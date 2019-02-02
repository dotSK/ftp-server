#ifndef FTP_FUNCTIONS
#define FTP_FUNCTIONS

#define send_status(NUM, MSG) ftp_sendline(sock_fd, #NUM MSG)

int functions_init(int initial_size);
void functions_destroy();
int ftp_acct(int sock_fd);
int ftp_cwd(const int sock_fd, const char *params);
int ftp_help(const int sock_fd);

#endif