#include "ftp_functions.h"

void ftp_acct(const int sock_fd) {
  send_status(220,
              "Permission already granted in response to PASS and/or USER");
}

void ftp_cwd(const int sock_fd, const StrBuf *restrict param,
             StrBuf *restrict curr_path, StrBuf *restrict buf) {
  StrBuf *new_path = NULL;

  if (*param->ptr != '\0') {
    new_path = validate_path(param, curr_path, buf);
    if (new_path != NULL && is_valid_dir(new_path->ptr)) {
      if (save_new_path(new_path, curr_path)) {
        send_status(250, "Okay");
      } else {
        send_status(451,
                    "Insufficient memory to process request, try again later");
      }
    } else {
      send_status(550, "Invalid path");
    }
    strbuf_destroy(new_path);
  } else {
    send_status(504, "Client passed empty parameter");
  }
}

void ftp_help(const int sock_fd) {
  ftp_sendline(
      sock_fd,
      "214-The following commands are recognized.\r\n"
      " ACCT CWD CDUP DELE HELP LIST MKD MODE MDTM STOR PWD QUIT\r\n"
      " PASV RETR RNFR RNTO RMD SIZE SYST USER NOOP PORT SITE TYPE\r\n");
  send_status(214, "Help OK.");
}

void ftp_cdup(const int sock_fd, const StrBuf *restrict param,
              StrBuf *restrict curr_path, StrBuf *restrict buf) {
  StrBuf *dir_up = strbuf_from_char("../");
  StrBuf *new_path = NULL;

  if (*param->ptr == '\0') {
    new_path = validate_path(&dir_up, curr_path, buf);
    if (new_path != NULL && is_valid_dir(new_path) &&
        save_new_path(new_path, curr_path)) {
      send_status(250, "Okay");
    } else {
      send_status(550, "Invalid path");
    }
    strbuf_destroy(new_path);
  } else {
    send_status(504, "Client passed non-empty parameter");
  }
}

void ftp_dele(const int sock_fd, const StrBuf *restrict param,
              const StrBuf *restrict curr_path, StrBuf *restrict buf) {
  StrBuf *new_path = NULL;
  int result = 0;

  if (*param->ptr != '\0') {
    new_path = validate_path(param, curr_path, buf);
    if (new_path != NULL && is_valid_file(new_path->ptr)) {
      result = unlink(new_path->ptr);
      if (result != 0) {
        if (errno == EACCES || errno == ENOENT || errno == EROFS) {
          send_status(550, "Cannot remove file");
        } else {
          send_status(450, "Cannot remove file right now, try again later");
        }
      } else {
        send_status(250, "File was removed successfully");
      }
    }
    strbuf_destroy(new_path);
  } else {
    send_status(504, "Client passed empty parameter");
  }
}

int ftp_list(const int sock_fd, const char *arg, int *conn_defined) {
  if (conn_defined) {
    if (change_path(cmd_argument, &path_buf, &path_buf_size,
                    &worker_data->dirinfo) == 1) {
      send_status(150, "Sending directory listing.");
      // ftp_sendline(sock_fd, "150 Sending directory listing.");
      pthread_mutex_lock(&worker_data->work_mtx);
      worker_data->data_op = LIST;
      pthread_cond_signal(&worker_data->work_ready);
      pthread_cond_wait(&worker_data->result_ready, &worker_data->work_mtx);
      if (worker_data->trans_ok) {
        send_status(226, "Directory send OK.");
        // ftp_sendline(sock_fd, "226 Directory send OK.");
      } else {
        send_status(425, "Transfer unsuccessfull.");
        // ftp_sendline(sock_fd, "425 Transfer unsuccessfull.");
      }
      pthread_mutex_unlock(&worker_data->work_mtx);
      *conn_defined = 0;
      return 1;
    } else {
      ftp_send_ascii(sock_fd, "550 Directory \"");
      ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
      ftp_sendline(sock_fd, "\" doesn't exist or is a file.");
      return 0;
    }
  } else {
    ftp_sendline(sock_fd, "425 Use PORT or PASV first.");
    return 0;
  }
}

int ftp_mkd(const int sock_fd, const char *arg) {
  if (*arg != '\0') {
    change_path(arg, &path_buf, &path_buf_size, &worker_data->dirinfo);
    if (mkdir(path_buf, 0755) == 0) {
      ftp_send_ascii(sock_fd, "250 Directory \"");
      ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
      ftp_sendline(sock_fd, "\" has been successfully created.");
    } else {
      ftp_send_ascii(sock_fd, "550 Directory \"");
      ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
      ftp_sendline(sock_fd, "\" couldn't be created.");
    }
  } else {
    ftp_sendline(sock_fd, "504 Client passed empty parameter");
  }
}

void ftp_mdtm(const int sock_fd, const StrBuf *restrict param,
              const StrBuf *restrict curr_path, StrBuf *restrict buf) {
  StrBuf *new_path = NULL;
  struct stat stat_buf;
  struct tm lt;

  if (*param->ptr != '\0') {
    new_path = validate_path(param, curr_path, buf);
    if (new_path != NULL &&
        (is_valid_file(new_path->ptr) || is_valid_dir(new_path->ptr))) {
      stat(new_path, &stat_buf);
      localtime_r(&stat_buf.st_mtime, &lt);
      strbuf_update(buf, "250 ", 0, sizeof("250 "));
      strftime(buf->ptr + 4, buf->size - 4, "%Y%m%d%H%M%S", &lt);
      ftp_sendline(sock_fd, buf->ptr);
    } else {
      send_status(550, "Invalid file specified");
    }
    strbuf_destroy(new_path);
  } else {
    send_status(504, "Client passed empty parameter.");
  }
}

int ftp_stor(const int sock_fd) {
  if (conn_defined) {
    if (*cmd_argument != '\0') {
      if (change_path(cmd_argument, &path_buf, &path_buf_size,
                      &worker_data->dirinfo) == 1) {
        ftp_send_ascii(sock_fd, "450 File \"");
        ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
        ftp_sendline(sock_fd, "\" is a directory.");
      } else {
        if ((temp_fjel = fopen(path_buf, "w")) != NULL) {
          ftp_sendline(sock_fd, "150 Ready to receive file");
          temp = 0;
          while (pthread_mutex_trylock(&worker_data->work_mtx) != 0 &&
                 temp < 3) {
            sleep(1);
            temp++;
          }
          if (temp < 3) {
            worker_data->file_w = temp_fjel;
            worker_data->data_op = STOR;
            pthread_cond_signal(&worker_data->work_ready);
            pthread_cond_wait(&worker_data->result_ready,
                              &worker_data->work_mtx);
            if (worker_data->trans_ok) {
              ftp_sendline(sock_fd, "226 File received OK.");
            } else {
              ftp_sendline(sock_fd, "451 Transfer unsuccessfull.");
            }
            conn_defined = 0;
            pthread_mutex_unlock(&worker_data->work_mtx);
          } else {
            ftp_sendline(sock_fd, "426 Connection timed out");
          }
        } else {
          ftp_send_ascii(sock_fd, "450 Cannot save file \"");
          ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
          ftp_sendline(sock_fd, "\".");
        }
      }
    } else {
      ftp_sendline(sock_fd, "504 Client passed empty parameter.");
    }
  } else {
    ftp_sendline(sock_fd, "425 Use PORT or PASV first.");
  }
  worker_data->start_pos = 0;
}

int ftp_pwd(const int sock_fd) {
  if (worker_data->dirinfo.c_dir[worker_data->dirinfo.base_dir_len] != '/') {
    ftp_sendline(sock_fd, "257 \"/\"");
  } else {
    ftp_send_ascii(sock_fd, "257 \"");
    ftp_send_ascii(
        sock_fd,
        &worker_data->dirinfo.c_dir[worker_data->dirinfo.base_dir_len]);
    ftp_send_ascii(sock_fd, "\"\r\n");
  }
}

int ftp_retr(const int sock_fd) {
  if (*cmd_argument != '\0') {
    temp = 0;
    while (pthread_mutex_trylock(&worker_data->work_mtx) != 0 && temp < 3) {
      sleep(1);
      temp++;
    }
    if (temp < 3) {
      if (worker_data->conn_defined) {
        if (change_path(cmd_argument, &path_buf, &path_buf_size,
                        &worker_data->dirinfo) == 2) {
          worker_data->fjel = path_buf;
          worker_data->data_op = RETR;
          pthread_cond_signal(&worker_data->work_ready);
          pthread_cond_wait(&worker_data->result_ready, &worker_data->work_mtx);
          if (worker_data->trans_ok) {
            ftp_sendline(sock_fd, "226 File send OK.");
          } else {
            ftp_sendline(sock_fd, "451 Transfer unsuccessfull.");
          }
          conn_defined = 0;
        } else {
          ftp_send_ascii(sock_fd, "550 File\"");
          ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
          ftp_sendline(sock_fd, "\" doesn't exist.");
        }
      } else {
        ftp_sendline(sock_fd, "425 Use PORT or PASV first.");
      }
      pthread_mutex_unlock(&worker_data->work_mtx);
    } else {
      ftp_sendline(sock_fd, "426 Connection timed out");
    }
  } else {
    ftp_sendline(sock_fd, "504 Client passed empty parameter.");
  }
  worker_data->start_pos = 0;
}

int ftp_rnfr(const int sock_fd) {
  if (*cmd_argument != '\0') {
    if (change_path(cmd_argument, &ren_buf, &ren_buf_size,
                    &worker_data->dirinfo) > 0) {
      ftp_sendline(sock_fd, "350 RNFR OK, waiting for RNTO.");
      rnfr_acc = 1;
    } else {
      ftp_send_ascii(sock_fd, "550 File\"");
      ftp_send_ascii(sock_fd, base_name_ptr(ren_buf));
      ftp_sendline(sock_fd, "\" doesn't exist.");
    }
  } else {
    ftp_sendline(sock_fd, "504 Client passed empty parameter.");
  }
}

int ftp_rnto(const int sock_fd) {
  if (*cmd_argument != '\0') {
    if (rnfr_acc) {
      if (change_path(cmd_argument, &path_buf, &path_buf_size,
                      &worker_data->dirinfo) > 0) {
        ftp_sendline(sock_fd, "550 File exists.");
      } else {
        if (rename(ren_buf, path_buf) == 0) {
          ftp_sendline(sock_fd, "250 File renamed succesfully");
        } else {
          ftp_sendline(sock_fd, "550 Failed to rename file.");
        }
      }
    } else {
      ftp_sendline(sock_fd, "550 You must use RNFR before using RNTO.");
    }
  } else {
    ftp_sendline(sock_fd, "504 Client passed empty parameter.");
  }
  rnfr_acc = 0;
}

int ftp_rmd(const int sock_fd) {
  if (*cmd_argument != '\0') {
    if (change_path(cmd_argument, &path_buf, &path_buf_size,
                    &worker_data->dirinfo) == 1) {
      if (rmdir(path_buf) == 0) {
        ftp_send_ascii(sock_fd, "250 Directory \"");
        ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
        ftp_sendline(sock_fd, "\" was removed successfully.");
      } else {
        ftp_send_ascii(sock_fd, "450 Couldn't remove directory \"");
        ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
        ftp_sendline(sock_fd, "\".");
      }
    } else {
      ftp_send_ascii(sock_fd, "550 Directory \"");
      ftp_send_ascii(sock_fd, base_name_ptr(path_buf));
      ftp_sendline(sock_fd, "\" does not exist.");
    }
  } else {
    ftp_sendline(sock_fd, "504 Client passed empty parameter");
  }
}

int ftp_size(const int sock_fd, const char *arg, const char *path) {
  char buf[255 + 10 + 17];
  struct stat stat_buf;
  int bytes_written = 0;

  if (*arg != '\0') {
    // TODO: rewrite me pl0x
    if (change_path(arg, &path_buf, &path_buf_size, &worker_data->dirinfo) ==
        2) {
      stat(path_buf, &stat_buf);
      snprintf(buf, 5, "220 ");
      bytes_written =
          snprintf(buf[4], sizeof(buf) - 4 - 6 - 1, "%ld", stat_buf.st_size);
      bytes_written = snprintf(buf[bytes_written + 4],
                               sizeof(buf) - bytes_written - 4 - 1, " bytes");
      ftp_sendline(sock_fd, buf);
      return 1;
    } else {
      snprintf(buf, 11, "550 File \"");
      bytes_written =
          snprintf(buf, sizeof(buf) - 10 - 17 - 1, "%s", base_name_ptr(path));
      snprintf(buf, sizeof(buf) - bytes_written - 10 - 1, "\" does not exist.");
      ftp_sendline(sock_fd, buf);
      return 0;
    }
  } else {
    ftp_sendline(sock_fd, "504 Client passed empty parameter.");
    return 0;
  }
}