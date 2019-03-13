#ifndef FTP_SERVER
#define FTP_SERVER
#define _POSIX_SOURCE
#define _GNU_SOURCE

#include "data_worker.h"
#include "ftp_functions.h"
#include "ftp_utils.h"

#include <assert.h>
#include <bits/sigaction.h>
#include <errno.h>
#include <linux/limits.h>
#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>

#define send_status(NUM, MSG) ftp_sendline(sock_fd, #NUM MSG)

enum FtpOp
{
  Acct,
  CdUp, // Change working directory to ../
  Cwd,  // Change working directory
  Dele, // Delete a file
  Help,
  List, // ls formated file list
  Mkd,  // Make a directory
  Mode, // Active/Passive mode?
  Mdtm,
  NoOp,
  Port,
  Pwd, // print current working directory
  Rest,
  Retr,
  RnFr, // Rename a file from
  RnTo, // Rename previously selected file to
  RmD,  // Remove a directory
  Site,
  Size, // Size of a file
  Syst,
  Stor, // Store a file
  Type,
  User,
  Quit
};

typedef struct conn_state
{
  int fd;
  StrBuf path;
} ConnState;

typedef struct thrd_data
{
  int sock_fd;
} ThrdData;

static const int PORT_NUM = 4785;
static const unsigned int NUM_OF_WORKERS = 10;
static const int EPOLL_MAX_EVENTS = 100;
static const int EPOLL_TIMEOUT = 100;

StrBuf cwd = {.ptr = NULL, .size = 0, .len = 0};

bool init_server(int sock_fd);
bool conn_arbiter(int sock_fd, pthread_t *thread_ids, ThrdData *thrd_data, size_t thrd_data_len);
void *server_thrd_start(const ThrdData *data);
static void *client_handler(void *socket_file_descriptor);
static void *ftp_data_worker(void *worker_data_struct);
int conn_handler(int socket_file_descriptor);
void start_session(int socket_file_descriptor, char *receive_buffer,
                   int buffer_size);
void sigusr_func(int signal_id);
void sigint_func(int signal_id);

#endif