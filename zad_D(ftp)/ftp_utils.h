#ifndef FTP_UTILS
#define FTP_UTILS
#include <pthread.h>
#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>

enum d_op {LIST, PASV, QUIT, RETR, STOR};

struct dir_info {
    char *c_dir;
    int base_dir_len;
    int c_dir_size;
};

struct ftp_data {
    int pasv;
    int conn_defined;
    int b_flag;
    int sock_fd;
    int start_pos;
    char *fjel;
    FILE *file_w;
    struct sockaddr_in remote;
    int trans_ok;
    enum d_op data_op;
    struct dir_info dirinfo;
    thrd_t thread_info;
    pthread_cond_t work_ready;
    pthread_cond_t result_ready;
    pthread_mutex_t work_mtx;
};

int change_path(const char *, char **, int *, struct dir_info *);
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
