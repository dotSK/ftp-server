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

#define send_status(NUM, MSG) ftp_sendline(sock_fd, #NUM MSG)

enum FtpOp {
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
  Pwd, // print current workin directory
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

static const int BACKLOG_SIZE = 10;
static const int PORT_NUM = 4785;

StrBuf cwd = {.ptr = NULL, .size = 0, .len = 0};

static void *client_handler(void *socket_file_descriptor);
static void *ftp_data_worker(void *worker_data_struct);
int conn_handler(int socket_file_descriptor);
void start_session(int socket_file_descriptor, char *receive_buffer,
                   int buffer_size);
void sigusr_func(int signal_id);
void sigint_func(int signal_id);

#endif