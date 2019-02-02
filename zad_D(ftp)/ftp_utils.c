#include "ftp_utils.h"

int change_path(const char *new_dir, char **buf, int *buf_size,
                struct dir_info *dirinfo) {
  int new_indice = 0;
  int new_dir_len = strlen(new_dir);
  int c_dir_len = strlen(dirinfo->c_dir);
  int dot_occured = 0;
  char *temp = 0;
  int base_indice = 0;
  struct stat statbuf;

  if (*buf_size == 0) {
    *buf = malloc(100 * sizeof(char));
    if (*buf == NULL) {
      fputs("failed to allocate memory for temp_dir buffer", stderr);
      exit(1);
    }
    *buf_size = 100;
  }

  if (*buf_size < c_dir_len + new_dir_len) {
    temp = realloc(*buf,
                   ((2 * *buf_size) + c_dir_len + new_dir_len) * sizeof(char));
    if (temp == NULL) {
      fputs("failed to allocate more memory for temp_dir buffer", stderr);
      exit(1);
    }
    *buf = temp;
  }

  strncpy(*buf, dirinfo->c_dir, *buf_size);

  if (*new_dir == '/') {
    base_indice = dirinfo->base_dir_len;
    if ((*buf)[dirinfo->base_dir_len] == '/') {
      (*buf)[dirinfo->base_dir_len] = '\0';
    }
  } else if (c_dir_len - 1 >= 0 && c_dir_len + 1 < dirinfo->c_dir_size) {
    if ((*buf)[c_dir_len - 1] != '/') {
      (*buf)[c_dir_len++] = '/';
      (*buf)[c_dir_len] = '\0';
    }
    base_indice = c_dir_len;
  }

  while (new_indice < new_dir_len) {
    char curr_char = new_dir[new_indice];
    if (curr_char == '.') {
      if (dot_occured) {
        base_indice = rollback_dir(*buf, dirinfo->base_dir_len);
        dot_occured = 0;
      } else {
        dot_occured = 1;
      }
    } else if (curr_char == '/') {
      if (dot_occured) {
        dot_occured = 0;
      } else {
        (*buf)[base_indice++] = '/';
      }
    } else {
      if (dot_occured) {
        (*buf)[base_indice++] = '.';
        dot_occured = 0;
      }
      (*buf)[base_indice++] = curr_char;
    }
    new_indice++;
  }
  if (base_indice - 1 >= 0) {
    (*buf)[base_indice - 1] == '/' ? --base_indice : base_indice;
  }
  (*buf)[base_indice] = '\0';

  if (stat(*buf, &statbuf) == 0) {
    if (S_ISDIR(statbuf.st_mode)) {
      while (strlen(*buf) > dirinfo->c_dir_size) {
        temp = realloc(dirinfo->c_dir, dirinfo->c_dir_size * 2 * sizeof(char));
        if (temp == NULL) {
          fputs(
              "failed to allocate bigger buffer for current directory storage",
              stderr);
          return -3;
        }
        dirinfo->c_dir_size *= 2;
      }
      return 1; // file exists and is a directory
    } else if (S_ISREG(statbuf.st_mode)) {
      return 2; // file exists and is a regular file
    } else {
      return -2; // file exists but is a special file
    }
  } else {
    return -1; // file does not exist
  }
}

int rollback_dir(char *c_dir, int base_len) {
  int done = 0;
  int dir_len = strlen(c_dir);
  int curr_indice = dir_len;

  while (!(done && c_dir[curr_indice] == '/') && curr_indice > base_len) {
    if (c_dir[curr_indice] == '/') {
      done = 1;
    }
    curr_indice--;
  }
  c_dir[curr_indice] = '\0';
  return curr_indice;
}

int ftp_send_binary(const int sockfd, const char *bytestream, const int size) {
  int sent_len = 0, temp_ret = 0;

  while (sent_len < size) {
    temp_ret = send(sockfd, &bytestream[sent_len], size, 0);
    if (temp_ret == -1) {
      return -1;
    } else {
      sent_len += temp_ret;
    }
  }
  return sent_len;
}

int ftp_send_ascii(const int sockfd, const char *msg) {
  return ftp_send_binary(sockfd, msg, strlen(msg));
}

int ftp_sendline(const int sockfd, const char *msg) {
  if (ftp_send_ascii(sockfd, msg) < 0) {
    return -1;
  }
  return send(sockfd, "\r\n", 3, 0);
}

// TODO: set fixed buffer size, so it won't grow indefinitely
// TODO: rewrite so it returns err if buffer is not big enough
char *ftp_cmd_get(const int sockfd, char *buf, int *buf_size) {
  int buf_indice = 0, recv_bytes = 0;
  int op_indice = 0;
  char *temp = 0;

  // if (*buf_size == 0) {
  //   temp = (malloc(100 * sizeof(char)));
  //   if (temp == NULL) {
  //     fputs("cannot allocate memory for ftp request buffer", stderr);
  //     exit(1);
  //   } else {
  //     *buf = temp;
  //     *buf_size = 100;
  //   }
  // }

  while ((recv_bytes =
              recv(sockfd, &(*buf)[buf_indice], (*buf_size - buf_indice), 0)) >=
             (*buf_size - buf_indice) &&
         (*buf)[recv_bytes + buf_indice - 1] != '\n') {
    if (recv_bytes == -1) {
      return NULL;
    }
    buf_indice += recv_bytes;
    *buf_size *= 2;
    temp = realloc(*buf, *buf_size * sizeof(char));
    if (temp == NULL) {
      fputs("cannot allocate more memory for ftp request buffer", stderr);
      exit(1);
    } else {
      *buf = temp;
    }
  }
  buf_indice += recv_bytes;

  if (buf_indice - 2 >= 0 && (*buf)[buf_indice - 2] == '\r') {
    (*buf)[buf_indice - 2] = '\0';
  } else if (buf_indice - 1 >= 0 && (*buf)[buf_indice - 1] == '\n') {
    (*buf)[buf_indice - 1] = '\0';
  } else {
    (*buf)[buf_indice] = '\0';
  }

  while (op_indice < *buf_size - 1 && (*buf)[op_indice] != ' ' &&
         (*buf)[op_indice] != '\0') {
    (*buf)[op_indice] = tolower((*buf)[op_indice]);
    op_indice++;
  }
  (*buf)[op_indice] = '\0';

  if (op_indice >= buf_indice - 2) {
    (*buf)[op_indice + 1] = '\0';
  }

  if (buf_indice > 0) {
    return *buf + op_indice + 1;
  } else {
    return NULL;
  }
}

int get_bound_sock(unsigned short int portnum, uint32_t ip_address) {
  struct sockaddr_in sock_addr;
  int sfd = 0;

  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons(portnum);
  sock_addr.sin_addr.s_addr = htonl(ip_address);
  sfd = socket(AF_INET, SOCK_STREAM,
               6); // ipv4 sock, tcp sock, tcp protocol number
  if (sfd == -1) {
    perror("socket");
    return -1; // socket cannot be created
  }

  if (bind(sfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
    perror("bind");
    return -2; // socket cannot be bound
  }

  return sfd;
}

int get_sock(unsigned short int portnum, uint32_t ip_address) {
  struct sockaddr_in sock_addr;
  int sfd = 0;

  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons(portnum);
  sock_addr.sin_addr.s_addr = htonl(ip_address);
  sfd = socket(AF_INET, SOCK_STREAM,
               6); // ipv4 sock, tcp sock, tcp protocol number
  if (sfd == -1) {
    perror("socket");
    return -1; // socket cannot be created
  }

  return sfd;
}

char *base_name_ptr(char *path) {
  int indice = strlen(path);

  if (indice > 1 && path[indice - 1] == '/') {
    indice--;
  }
  while (path[--indice] != '/')
    ;

  return &path[indice + 1];
}

unsigned short int parse_ip_port(const char *str, uint32_t *ret_ip) {
  uint32_t ip[3];
  unsigned short int temp, temp1;

  if (sscanf(str, "%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%hu,%hu",
             ret_ip, &ip[0], &ip[1], &ip[2], &temp, &temp1) == 6) {
    for (int i = 0; i < 3; i++) {
      *ret_ip = *ret_ip << 8;
      *ret_ip |= ip[i];
    }
    temp *= 256;
    temp += temp1;
    return temp;
  } else {
    return -1;
  }
}

int est_pasv(int sockfd) {
  int temp = 0;

  if (listen(sockfd, 1) != 0) {
    perror("data listen");
    return -1;
  } else {
    if ((temp = accept(sockfd, NULL, NULL)) > 0) {
      close(sockfd);
      return temp;
    } else if (errno == EINTR) {
      close(sockfd);
      return -2;
    } else {
      perror("pasv accept");
      return -3;
    }
  }
}
