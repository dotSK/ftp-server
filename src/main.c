#include "main.h"

static bool shutdown = false;

int main(int argc, char *argv[]) {
  int sfd = 0;
  struct sigaction sigint_handler;
  bool status = false;

  // set signal handler for SIGINT
  memset(&sigint_handler, 0, sizeof(sigint_handler));
  sigint_handler.sa_handler = &sigint_func;
  if (sigaction(SIGINT, &sigint_handler, NULL) != 0) {
    perror("sigaction");
    pthread_exit(NULL);
  }

  // TODO: GNUism
  cwd.ptr = get_current_dir_name();
  cwd.len = strlen(cwd.ptr);
  cwd.size = cwd.len + 1;

  // print some informations
  fprintf(stdout, "listening on port: %d\n", PORT_NUM);
  fprintf(stdout, "current working directory: %s", cwd);

  // bind socket
  sfd = get_bound_sock(PORT_NUM, INADDR_ANY);
  if (sfd < 0) {
    perror('socket/bind');
    strbuf_free(&cwd);
    exit(1);
  } else {
    status = init_threads(sfd);
    // conn_handler(sfd);
    close(sfd);
  }
  strbuf_free(&cwd);
  exit(!status);
}

bool init_threads(int sock_fd) {
  unsigned int thread_count = 0;
  pthread_t *thread_ids = NULL;
  ThrdData *thrd_data = NULL;
  int tmp_sock_fd = 0;
  int epoll_fd = 0;

  // TODO: LINUXism
  long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  // TODO: get rid of upper limit
  assert(cpu_count > 0 && cpu_count < 1024);
  thread_count = ((unsigned int)cpu_count) - 1 + NUM_OF_WORKERS;

  if (listen(sock_fd, SOMAXCONN) != 0) {
    perror("listen");
    return false;
  }

  thread_ids = malloc(sizeof(pthread_t) * thread_count);
  thrd_data = malloc(sizeof(ThrdData));
  if (thread_ids == NULL || thrd_data == NULL) {
    perror("malloc");
    return false;
  }

  thrd_data->sock_fd = sock_fd;
  for (int i = 0; i < thread_count; i++) {
    if (pthread_create(thread_ids + i, NULL, &server_thrd_start, &thrd_data) !=
        0) {
      free(thread_ids);
      return false;
    }
  }

  server_thrd_start(&thrd_data);
}

void *server_thrd_start(const ThrdData *data) {
  int epoll_fd = 0;
  struct epoll_event epoll_cfg;
  ConnState *tmp_state = NULL;

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    // TODO: sensible return value
    pthread_exit(NULL);
  }

  tmp_state = malloc(sizeof(ConnState));
  if (tmp_state == NULL) {
    perror("malloc");
    // TODO: sensible return value
    pthread_exit(NULL);
  }
  tmp_state->fd = data->sock_fd;
  // TODO: do i really want edge-triggered polling?
  epoll_cfg.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
  epoll_cfg.data.ptr = tmp_state;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data->sock_fd, &epoll_cfg) != 0) {
    perror("epoll_ctl");
    // TODO: sensible return value
    pthread_exit(NULL);
  }

  const struct epoll_event *event_queue = NULL;
  while (!shutdown) {
    if (epoll_wait(epoll_fd, &event_queue, 1, -1) != 1) {
      perror("epoll_wait");
      shutdown = true;
    } else {
      switch (event_queue[0].events) {
      case EPOLLERR:
        fputs("epolerr happened...", stderr);
        shutdown = true;
        break;

      // socket closed - connection ended
      case EPOLLHUP:
        shutdown = true;
        break;

      // socket special exception (TODO: maybe handle later?)
      case EPOLLPRI:
        fputs("epollpri happened...", stderr);
        shutdown = true;
        break;

      // socket has data ready
      case EPOLLIN:
        // FIXME: lolwut?
        if (((ConnState *)event_queue[0].data.ptr)->fd == data->sock_fd) {
          // accept connection
          if (accept(sockfd, NULL, NULL) >= 0) {

          } else {
            perror("accept");
          }

        } else {
          // dispatch worker
        }
        break;

      default:
        fputs("epoll_wait returned non-selected signal, this should not happen",
              stderr);
        shutdown = 1;
        break;
      }
    }
  }

  tmp_sock_fd = accept(sockfd, NULL, NULL);
  if (tmp_sock_fd == -1) {
    if (errno == EINTR) {

    } else {
      perror("accept");
      exit(1);
    }
  }
  if ((temp_sfd = accept(sockfd, NULL, NULL)) == -1) {
    if (errno == EINTR) {
      continue;
    } else {
      perror("accept");
      exit(1);
    }
  }

  if (drop) {
    ftp_sendline(temp_sfd,
                 "421 Server run out of thread buffer, please try again later");
    close(temp_sfd);
    continue;
  } else {
    sock_fds[tp_indice] = temp_sfd;
  }

  if (pthread_create(&thread_pool[tp_indice], NULL, &client_handler,
                     &sock_fds[tp_indice]) != 0) {
    perror("pthread_create");
    exit(1);
  } else {
    thread_active[tp_indice] = 1;
  }
}

// join and end threads

free(sock_fds);
free(thread_active);
free(thread_pool);

return 0;
}

static void *client_handler(void *sockfd) {
  int sock_fd = *(int *)sockfd;
  char *recv_buf = NULL;
  char *cmd_argument = NULL;
  int buf_size = 0;
  int end_of_session = 0;

  recv_buf = malloc(BUF_SIZE * sizeof(char));
  if (recv_buf == NULL) {
    fputs("failed to allocate memory for responese buffer", stderr);
    pthread_exit(thrd_nomem);
  }
  buf_size = BUF_SIZE;

  send_status(220, "beta_test_ftp_server");

  while (!end_of_session) {
    cmd_argument = ftp_cmd_get(sock_fd, &recv_buf, &buf_size);
    if (!strcmp(recv_buf, "user")) {
      if (!strcmp(cmd_argument, "anonymous")) {
        send_status(331, "Please specify password");
        cmd_argument = ftp_cmd_get(sock_fd, &recv_buf, &buf_size);
        puts(recv_buf);
        if (!strcmp(recv_buf, "pass")) {
          ftp_sendline(sock_fd, "230 Welcome!");
          start_session(sock_fd, recv_buf, buf_size);
          end_of_session = 1;
        } else {
          send_status(530, "Please login with USER and PASS");
        }
      } else {
        send_status(530, "This FTP server supports only anonymous login");
      }
    } else {
      send_status(530, "Please login with USER and PASS");
    }
  }

  close(sock_fd);
  free(recv_buf);
  thrd_exit(thrd_success);
}

void start_session(int sock_fd, char *recv_buf, int buf_size) {
  struct ftp_data *worker_data;
  struct sockaddr_in temp_addr;
  struct stat stat_buf;
  unsigned int temp_addr_len = 0;

  int conn_defined = 0;
  char *cmd_argument;
  int exit_cond = 0;
  int temp = 0;
  unsigned short temp1 = 0, temp2 = 0;
  uint32_t remote_ip = 0;
  FILE *temp_fjel;

  char *path_buf = 0;
  char *ren_buf = 0;
  int ren_buf_size = 0;
  int path_buf_size = 0;
  int rnfr_acc = 0;

  worker_data = malloc(sizeof(struct ftp_data));
  if (worker_data == NULL) {
    fputs("failed to allocate memory for ftp data thread communication",
          stderr);
    thrd_exit(thrd_nomem);
  }
  memset(worker_data, 0, sizeof(struct ftp_data));

  path_buf = malloc(100 * sizeof(char));
  if (path_buf == NULL) {
    fputs("failed to allocate memory for ftp temp directory buffer", stderr);
    pthread_exit(NULL);
  }
  path_buf_size = 100;

  ren_buf = malloc(100 * sizeof(char));
  if (path_buf == NULL) {
    fputs("failed to allocate memory for file rename path buffer", stderr);
    pthread_exit(NULL);
  }
  ren_buf_size = 100;

  // init current directory buffer
  worker_data->dirinfo.c_dir = malloc(100 * sizeof(char));
  if (worker_data->dirinfo.c_dir == NULL) {
    fputs("failed to allocate memory for ftp directory buffer", stderr);
    pthread_exit(NULL);
  }
  worker_data->dirinfo.c_dir_size = 100;
  worker_data->dirinfo.c_dir =
      getcwd(worker_data->dirinfo.c_dir, worker_data->dirinfo.c_dir_size);
  worker_data->dirinfo.base_dir_len = strlen(worker_data->dirinfo.c_dir);
  worker_data->start_pos = 0;

  // init ftp data worker thread data struct
  pthread_cond_init(&worker_data->work_ready, NULL);
  pthread_cond_init(&worker_data->result_ready, NULL);
  pthread_mutex_init(&worker_data->work_mtx, NULL);

  if (thrd_create(&worker_data->thread_info, ftp_data_worker, worker_data) !=
      0) {
    fputs("failed to create ftp data thread", stderr);
    exit(1);
  }

  while (!exit_cond && !shutdown) {
    if ((cmd_argument = ftp_cmd_get(sock_fd, recv_buf, buf_size)) == NULL) {
      exit_cond = 1;
      continue;
    }

    if (!strcmp(*recv_buf, "acct"))
      ftp_acct(sock_fd);
    else if (!strcmp(*recv_buf, "cwd"))
      ftp_cwd(sock_fd, cmd_argument);
    else if (!strcmp(*recv_buf, "cdup"))
      ftp_cdup(sock_fd);
    else if (!strcmp(*recv_buf, "dele"))
      ftp_dele(sock_fd);
    else if (!strcmp(*recv_buf, "help"))
      ftp_help(sock_fd);
    else if (!strcmp(*recv_buf, "list"))
      ftp_list(sock_fd);
    else if (!strcmp(*recv_buf, "mkd") || !strcmp(*recv_buf, "xmkd"))
      ftp_mkd(sock_fd);
    else if (!strcmp(*recv_buf, "mode"))
      ftp_mode(sock_fd);
    else if (!strcmp(*recv_buf, "mdtm"))
      ftp_mdtm(sock_fd);
    else if (!strcmp(*recv_buf, "stor"))
      ftp_stor(sock_fd);
    else if (!strcmp(*recv_buf, "pwd"))
      ftp_pwd(sock_fd);
    else if (!strcmp(*recv_buf, "quit")) {
      ftp_sendline(sock_fd, "221 Goodbye!");
      exit_cond = 1;
    } else if (!strcmp(*recv_buf, "pasv")) {
      if (*cmd_argument != '\0') {
        ftp_sendline(sock_fd, "504 Command parameters are prohibited");
      } else {
        if ((temp = get_bound_sock(0, INADDR_ANY)) > 0) {
          temp_addr_len = sizeof(temp_addr);
          getsockname(temp, (struct sockaddr *)&temp_addr, &temp_addr_len);
          if (pthread_mutex_trylock(&worker_data->work_mtx) != 0) {
            pthread_kill(worker_data->t_info, SIGUSR1);
            pthread_mutex_lock(&worker_data->work_mtx);
          }
          worker_data->sock_fd = temp;
          worker_data->pasv = 1;
          worker_data->data_op = PASV;
          pthread_cond_signal(&worker_data->work_ready);
          pthread_mutex_unlock(&worker_data->work_mtx);
          ftp_send_ascii(sock_fd, "227 =(127,0,0,1,");
          temp1 = ntohs(temp_addr.sin_port) % 256;
          temp2 = ntohs(temp_addr.sin_port) / 256;
          sprintf(*recv_buf, "%d,%d)", temp2, temp1);
          ftp_sendline(sock_fd, *recv_buf);
          conn_defined = 1;
        }
      }
    } else if (!strcmp(*recv_buf, "retr"))
      ftp_retr(sock_fd);
    else if (!strcmp(*recv_buf, "rnfr"))
      ftp_rnfr(sock_fd);
    else if (!strcmp(*recv_buf, "rnto"))
      ftp_rnto(sock_fd);
    else if (!strcmp(*recv_buf, "rmd"))
      ftp_rmd(sock_fd);
    else if (!strcmp(*recv_buf, "size"))
      ftp_size(sock_fd);
    else if (!strcmp(*recv_buf, "syst")) {
      ftp_sendline(sock_fd, "215 UNIX Type: L8");
    } else if (!strcmp(*recv_buf, "user")) {
      ftp_sendline(sock_fd, "530 Can't change from guest user");
    } else if (!strcmp(*recv_buf, "noop")) {
      ftp_sendline(sock_fd, "200 NOOP ok.");
    } else if (!strcmp(*recv_buf, "port")) {
      if (*cmd_argument == '\0') {
        ftp_sendline(sock_fd, "501 Received empty parameter");
      } else {
        if ((temp1 = parse_ip_port(cmd_argument, &remote_ip)) > 0) {
          ftp_sendline(sock_fd, "200 Accepted");
          if ((temp = get_sock(temp1, remote_ip)) > 0) {
            if (pthread_mutex_trylock(&worker_data->work_mtx) != 0) {
              pthread_kill(worker_data->t_info, SIGUSR1);
              pthread_mutex_lock(&worker_data->work_mtx);
            }
            worker_data->sock_fd = temp;
            memset(&worker_data->remote, 0, sizeof(worker_data->remote));
            worker_data->remote.sin_family = AF_INET;
            worker_data->remote.sin_port = htons(temp1);
            worker_data->remote.sin_addr.s_addr = htonl(remote_ip);
            worker_data->pasv = 0;
            worker_data->conn_defined = 1;
            pthread_mutex_unlock(&worker_data->work_mtx);
            conn_defined = 1;
          }
        } else {
          ftp_sendline(sock_fd, "501 Syntax error");
        }
      }
    } else if (!strcmp(*recv_buf, "site")) {
      ftp_sendline(sock_fd,
                   "200 This server has no site specific commands available");
    } else if (!strcmp(*recv_buf, "type")) {
      if (!strcmp(cmd_argument, "a") || !strcmp(cmd_argument, "a n") ||
          !strcmp(cmd_argument, "A")) {
        worker_data->b_flag = 0;
        ftp_sendline(sock_fd, "200 Switched to ASCII mode");
      } else if (!strcmp(cmd_argument, "i") || !strcmp(cmd_argument, "l 8") ||
                 !strcmp(cmd_argument, "I") || !strcmp(cmd_argument, "L 8")) {
        worker_data->b_flag = 1;
        ftp_sendline(sock_fd, "200 Switched to Binary mode");
      } else {
        ftp_sendline(sock_fd, "501 Syntax error");
      }
    } else if (!strcmp(*recv_buf, "rest")) {
      if (*cmd_argument != '\0') {
        if (sscanf(cmd_argument, "%d", &temp) == 1) {
          worker_data->start_pos = temp;
          ftp_sendline(sock_fd, "350 REST accepted");
        } else {
          ftp_sendline(sock_fd, "501 Syntax error");
        }
      } else {
        ftp_sendline(sock_fd, "501 Received empty parameter");
      }
    } else {
      ftp_sendline(sock_fd, "500 Unknown command.");
    }
  }

  if (pthread_mutex_trylock(&worker_data->work_mtx) != 0) {
    pthread_kill(worker_data->t_info, SIGUSR1);
    pthread_mutex_lock(&worker_data->work_mtx);
  }
  worker_data->data_op = QUIT;
  pthread_cond_signal(&worker_data->work_ready);
  pthread_mutex_unlock(&worker_data->work_mtx);
  pthread_join(worker_data->t_info, NULL);
  free(path_buf);
  free(ren_buf);
  free(worker_data->dirinfo.c_dir);
  pthread_mutex_destroy(&worker_data->work_mtx);
  pthread_cond_destroy(&worker_data->work_ready);
  pthread_cond_destroy(&worker_data->result_ready);
  free(worker_data);
  return;
}

static void *ftp_data_worker(void *data_struct) {
  struct ftp_data *worker_data = (struct ftp_data *)data_struct;
  struct sigaction sigusr_handler;
  int exit_cond = 0;
  int sock_fd = 0;
  char *path_buf;
  int path_buf_size;
  char *data_buf;
  int data_buf_size;
  char *buf_temp;
  int temp = 0, temp1 = 0;
  FILE *fp;

  memset(&sigusr_handler, 0, sizeof(sigusr_handler));
  sigusr_handler.sa_handler = &sigusr_func;
  if (sigaction(SIGUSR1, &sigusr_handler, NULL) != 0) {
    perror("sigaction");
    pthread_exit(NULL);
  }

  path_buf = malloc(300 * sizeof(char));
  if (path_buf == NULL) {
    fputs("failed to allocate data worker file path buffer", stderr);
    pthread_exit(NULL);
  }
  path_buf_size = 300 * sizeof(char);

  data_buf = malloc(1024 * sizeof(char));
  if (data_buf == NULL) {
    fputs("failed to allocate data worker file buffer", stderr);
    pthread_exit(NULL);
  }
  data_buf_size = 1024 * sizeof(char);

  pthread_mutex_lock(&worker_data->work_mtx);
  while (!exit_cond) {
    pthread_cond_wait(&worker_data->work_ready, &worker_data->work_mtx);

    switch (worker_data->data_op) {
    case PASV:
      if ((sock_fd = est_pasv(worker_data->sock_fd)) < 0) {
        worker_data->conn_defined = 0;
      } else {
        worker_data->conn_defined = 1;
      }
      break;
    case LIST:
      if (worker_data->conn_defined) {
        if (!worker_data->pasv) {
          if ((connect(worker_data->sock_fd,
                       (struct sockaddr *)&worker_data->remote,
                       sizeof(worker_data->remote))) == -1) {
            fputs("active socket connection to client failed", stderr);
            break;
          } else {
            sock_fd = worker_data->sock_fd;
          }
        }
        while (strlen(worker_data->dirinfo.c_dir) > path_buf_size) {
          buf_temp = realloc(path_buf, 2 * path_buf_size * sizeof(char));
          if (buf_temp == NULL) {
            fputs("cannot allocate more memory to ftp data worker path buffer, "
                  "skipping operation",
                  stderr);
            close(sock_fd);
            worker_data->conn_defined = 0;
            break;
          } else {
            path_buf = buf_temp;
            path_buf_size *= 2;
          }
        }
        sprintf(path_buf, "/bin/ls -l \"%s\"", worker_data->dirinfo.c_dir);
        fp = popen(path_buf, "r");
        while (fgets(data_buf, data_buf_size, fp) != NULL) {
          if (ftp_send_ascii(sock_fd, data_buf) < 0) {
            fputs("failed to send data to client", stderr);
            worker_data->trans_ok = 0;
            break;
          }
          worker_data->trans_ok = 1;
        }
        pclose(fp);
        close(sock_fd);
        worker_data->conn_defined = 0;
      } else {
        fputs("ftp thread requested action without making a connection first",
              stderr);
        worker_data->trans_ok = 0;
      }
      close(sock_fd);
      worker_data->conn_defined = 0;
      pthread_cond_signal(&worker_data->result_ready);
      break;
    case RETR:
      if (worker_data->conn_defined) {
        if (!worker_data->pasv) {
          if ((connect(worker_data->sock_fd,
                       (struct sockaddr *)&worker_data->remote,
                       sizeof(worker_data->remote))) == -1) {
            fputs("active socket connection to client failed", stderr);
            break;
          } else {
            sock_fd = worker_data->sock_fd;
          }
        }
        if (worker_data->b_flag) {
          if ((fp = fopen(worker_data->fjel, "rb")) != NULL) {
            if (worker_data->start_pos != 0)
              fseek(fp, worker_data->start_pos, SEEK_SET);
            while (!((temp = fread(data_buf, sizeof(char), data_buf_size,
                                   fp)) <= 0) ||
                   !feof(fp)) {
              temp1 = ftp_send_binary(sock_fd, data_buf, temp);
              if (temp1 == -1) {
                fputs("failed to send data", stderr);
                worker_data->trans_ok = 0;
                break;
              }
              worker_data->trans_ok = 1;
            }
            fclose(fp);
          } else {
            fputs("file from worker data does not exist, lolwut?", stderr);
          }
        } else {
          if ((fp = fopen(worker_data->fjel, "r")) != NULL) {
            if (worker_data->start_pos != 0)
              fseek(fp, worker_data->start_pos, SEEK_SET);
            while ((buf_temp = fgets(data_buf, data_buf_size, fp)) != NULL &&
                   !(feof(fp) || ferror(fp))) {
              ftp_send_ascii(sock_fd, data_buf);
            }
            if (ferror(fp)) {
              worker_data->trans_ok = 0;
            } else {
              worker_data->trans_ok = 1;
            }
            fclose(fp);
          } else {
            fputs("file from worker data does not exist, lolwut?", stderr);
          }
        }
        close(sock_fd);
        worker_data->conn_defined = 0;
        worker_data->start_pos = 0;

      } else {
        fputs("connection is not defined", stderr);
        worker_data->trans_ok = 0;
      }
      close(sock_fd);
      worker_data->conn_defined = 0;
      pthread_cond_signal(&worker_data->result_ready);
      break;
    case STOR:
      if (worker_data->conn_defined) {
        if (!worker_data->pasv) {
          if ((connect(worker_data->sock_fd,
                       (struct sockaddr *)&worker_data->remote,
                       sizeof(worker_data->remote))) == -1) {
            fputs("active socket connection to client failed", stderr);
            break;
          } else {
            sock_fd = worker_data->sock_fd;
          }
        }
      }
      if (worker_data->file_w != NULL) {
        while ((temp = recv(sock_fd, data_buf, data_buf_size, 0)) > 0) {
          if (fwrite(data_buf, sizeof(*data_buf), temp, worker_data->file_w) !=
              temp) {
            fputs("failed to write file", stderr);
            worker_data->trans_ok = 0;
            break;
          }
        }
        fflush(worker_data->file_w);
        fclose(worker_data->file_w);
        worker_data->trans_ok = 1;
      } else {
        fputs("file from worker_data is NULL, lolwut?", stderr);
        worker_data->trans_ok = 1;
      }
      close(sock_fd);
      worker_data->conn_defined = 0;
      pthread_cond_signal(&worker_data->result_ready);
      break;
    case QUIT:
      close(sock_fd);
      worker_data->conn_defined = 0;
      exit_cond = 1;
      break;
    default:
      fputs("data worker received wrong op, lolwut?", stderr);
      break;
    }
  }

  close(sock_fd);
  free(path_buf);
  free(data_buf);
  pthread_mutex_unlock(&worker_data->work_mtx);
  pthread_exit(NULL);
}

void sigusr_func(int sigid) {
  fputs("aborted listen", stderr);
  return;
}

void sigint_func(int sigid) {
  fputs("\nFTP Server is shutting down gracefully...\n", stderr);
  shutdown = 1;
  return;
}
