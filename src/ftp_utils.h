#ifndef FTP_UTILS
#define FTP_UTILS
#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

typedef struct StrBuf {
  char *ptr;
  size_t size;
  size_t len;
} StrBuf;

extern StrBuf cwd;

void strbuf_free(StrBuf *buf);
int try_size_change(StrBuf *restrict buf, const size_t size);
char *validate_path(const StrBuf *restrict new_path,
                    const StrBuf *restrict curr_path,
                    StrBuf *restrict path_buf);
int is_valid_dir(const char *path);
int get_port(const char *, struct addrinfo **);
int ftp_sendline(int, const char *);
int ftp_send_ascii(int, const char *);
int ftp_send_binary(const int, const char *, const int);
char *ftp_cmd_get(const int sockfd, char **, int *);
int get_bound_sock(unsigned short int, uint32_t);
int get_sock(unsigned short int, uint32_t);
int rollback_dir(char *, int);
unsigned short int parse_ip_port(const char *str, uint32_t *ret_ip);
char *base_name_ptr(char *);
int est_pasv(int);
#endif
