#include "ftp_utils.h"

void strbuf_free(StrBuf *buf) {
  free(buf->ptr);
  buf->size = 0;
  buf->len = 0;
}

void strbuf_destroy(StrBuf *buf) {
  strbuf_free(buf->ptr);
  free(buf);
}

/**
 * Creates new StrBuf.
 */
StrBuf *strbuf_new(void) {
  StrBuf *temp = malloc(sizeof(StrBuf));

  if (temp != NULL) {
    temp->ptr = NULL;
    temp->len = 0;
    temp->size = 0;
  }
  return temp;
}

/**
 * Create new StrBuf from existing string.
 */
StrBuf *strbuf_from_char(const char *restrict str) {
  StrBuf *temp = malloc(sizeof(StrBuf));

  if (temp != NULL) {
    temp->ptr = str;
    temp->len = strlen(str);
    temp->size = temp->len + 1;
  }
  return temp;
}

/**
 * Tries to reallocate buf if smaller than size,
 * or if it fails, tries to allocate new buffer using malloc.
 */
bool try_size_change(StrBuf *restrict buf, const size_t size) {
  char *tmp_ptr = NULL;

  if (buf->size < size) {
    if ((tmp_ptr = realloc(buf, size)) != NULL) {
      buf->size = size;
    } else if ((tmp_ptr = malloc(size)) != NULL) {
      strbuf_free(buf);
      buf->ptr = tmp_ptr;
      buf->size = size;
    } else {
      return false;
    }
  }
  return true;
}

/**
 * Tries to change path.
 *
 * @param new_path path to be copied to buffer
 * @param curr_rel_path current relative path buffer (copy destination)
 *
 * @return Boolean according to result.
 */
bool try_path_copy(const StrBuf *restrict new_path,
                   const StrBuf *restrict curr_rel_path) {
  size_t rel_segment_size = new_path->len - cwd.len + 1;

  if (try_size_change(curr_rel_path, rel_segment_size)) {
    memcpy(curr_rel_path->ptr, new_path->ptr + cwd.len, rel_segment_size);
    return true;
  } else {
    return false;
  }
}

/**
 * Checks if path is confined to a given starting folder.
 *
 * @return *ptr if path is valid
 * @return NULL if path is invalid or there was an allocation error
 */
char *validate_path(const StrBuf *restrict new_path,
                    const StrBuf *restrict curr_path,
                    StrBuf *restrict path_buf) {
  size_t desired_size = 0;
  char *canonical_path = NULL;

  if (new_path->len > 0 && new_path->ptr[0] == '/') {
    desired_size = cwd.len + new_path->len + 1;
    if (try_size_change(path_buf, desired_size)) {
      memcpy(path_buf->ptr, cwd.ptr, cwd.len);
      memcpy(path_buf->ptr + cwd.len, new_path->ptr, new_path->len + 1);
    } else {
      return NULL;
    }
  } else {
    desired_size = cwd.len + curr_path->len + new_path->len + 1;
    if (try_size_change(path_buf, desired_size)) {
      memcpy(path_buf->ptr, cwd.ptr, cwd.len);
      memcpy(path_buf->ptr + cwd.len, curr_path->ptr, curr_path->len);
      memcpy(path_buf->ptr + cwd.len + curr_path->len, new_path->ptr,
             new_path->len + 1);
    } else {
      return NULL;
    }
  }

  // TODO: GNUism
  canonical_path = canonicalize_file_name(path_buf->ptr);
  if (canonical_path != NULL) {
    if (!path_confined(canonical_path)) {
      free(canonical_path);
      canonical_path = NULL;
    }
  }
  return canonical_path;
}

// TODO: how to define FTP bounds?
/**
 * Checks if given path is confined withing FTP bounds.
 */
static bool path_confined(const char *restrict path) {
  size_t path_len = strlen(path);
  if (path_len < cwd.len) {
    return false;
  } else {
    return strncmp(cwd.ptr, path, cwd.len) == 0;
  }
}

/**
 * Checks if path is valid dir.
 *
 * @param path Preferrably absolute canonical path.
 * @return Boolean according to result.
 */
bool is_valid_dir(const char *path) {
  struct stat result;

  if (lstat(path, &result) != 0) {
    return false;
  } else {
    return S_ISDIR(result.st_mode);
  }
}

static int ftp_send_binary(const int sockfd, const char *bytestream,
                           const int size) {
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
