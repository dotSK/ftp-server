#define _POSIX_SOURCE
#include "data_worker.h"
#include "ftp_utils.h"
#include <signal.h>
#include <libaio.h>

static void *ftp_data_worker(void *data_struct) {
  struct ftp_data *worker_data = (struct ftp_data *)data_struct;
  struct stat my_stat;
  struct sigaction sigusr_handler;
  int exit_cond = 0;
  int sock_fd = 0;
  char *path_buf;
  int path_buf_size;
  char *data_buf;
  int data_buf_size;
  char *buf_temp;
  int temp = 0, temp1 = 0;
  int data_indice = 0;
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
            while (!(temp = fread(data_buf, sizeof(char), data_buf_size, fp)) <=
                       0 ||
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
