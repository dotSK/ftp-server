#pragma once

#include "ftp_utils.h"
#include <limits.h>
#include <malloc.h>

#define send_status(NUM, MSG) ftp_sendline(sock_fd, #NUM MSG)

extern StrBuf_t cwd;

int functions_init(int initial_size);
void functions_destroy();
void ftp_acct(int sock_fd);
void ftp_cwd(const int sock_fd, const StrBuf_t* restrict param,
             StrBuf_t* restrict curr_path, StrBuf_t* restrict buf);
void ftp_help(const int sock_fd);
void ftp_cdup(const int sock_fd, const StrBuf_t* restrict param,
              StrBuf_t* restrict curr_path, StrBuf_t* restrict buf);
void ftp_dele(const int sock_fd, const StrBuf_t* restrict param,
              const StrBuf_t* restrict curr_path, StrBuf_t* restrict buf);
void ftp_mdtm(const int sock_fd, const StrBuf_t* restrict param,
              const StrBuf_t* restrict curr_path, StrBuf_t* restrict buf);