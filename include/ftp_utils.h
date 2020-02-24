#pragma once
#define _GNU_SOURCE

#include "str_buf.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// functions
bool save_new_path(const char* restrict new_path,
                   const StrBuf_t* restrict curr_rel_path);
StrBuf_t* validate_path(const StrBuf_t* restrict new_path,
                      const StrBuf_t* restrict curr_path,
                      StrBuf_t* restrict path_buf);
bool is_valid_dir(const char* path);
bool is_valid_file(const char* path);
int get_port(const char*, struct addrinfo**);
int ftp_sendline(int, const char*);
int ftp_send_ascii(int, const char*);
int ftp_send_binary(const int, const char*, const int);
char* ftp_cmd_get(const int sockfd, char**, int*);
int get_bound_sock(unsigned short int, uint32_t);
int get_sock(unsigned short int, uint32_t);
unsigned short int parse_ip_port(const char* str, uint32_t* ret_ip);
char* base_name_ptr(char*);
int est_pasv(int);
