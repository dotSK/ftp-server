#ifndef FTP_FUNCTIONS
#define FTP_FUNCTIONS

#define send_status(NUM, MSG) ftp_sendline(sock_fd, #NUM MSG)

extern StrBuf cwd;

int functions_init(int initial_size);
void functions_destroy();
int ftp_acct(int sock_fd);
void ftp_cwd(const int sock_fd, const StrBuf *restrict param,
            StrBuf *restrict curr_path, StrBuf *restrict path_buf);
int ftp_help(const int sock_fd);

#endif