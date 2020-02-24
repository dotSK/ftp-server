#pragma once
#define _POSIX_SOURCE
#define _GNU_SOURCE

#include "str_buf.h"

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

typedef struct conn_state {
    int fd;
    StrBuf_t path;
} ConnState;

typedef struct thrd_data {
    int sock_fd;
} ThrdData;