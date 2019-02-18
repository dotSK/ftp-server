#ifndef FTP_FUNCTIONS
#define FTP_FUNCTIONS

#include "ftp_utils.h"
#include <limits.h>
#include <malloc.h>

#define send_status(NUM, MSG) ftp_sendline(sock_fd, #NUM MSG)

extern StrBuf cwd;

int functions_init(int initial_size);
void functions_destroy();
void ftp_acct(int sock_fd);
void ftp_cwd(const int sock_fd, const StrBuf *restrict param,
             StrBuf *restrict curr_path, StrBuf *restrict buf);
void ftp_help(const int sock_fd);
void ftp_cdup(const int sock_fd, const StrBuf *restrict param,
              StrBuf *restrict curr_path, StrBuf *restrict buf);
void ftp_dele(const int sock_fd, const StrBuf *restrict param,
              const StrBuf *restrict curr_path, StrBuf *restrict buf);
void ftp_mdtm(const int sock_fd, const StrBuf *restrict param,
              const StrBuf *restrict curr_path, StrBuf *restrict buf);
#endif