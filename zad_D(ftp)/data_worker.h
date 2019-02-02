#ifndef DATA_WORKER
#define DATA_WORKER

static void *ftp_data_worker(void *);
void sigusr_func(int);

enum WorkerOp { Init, Op };

struct worker_pool {
  struct worker_state state;
  struct worker_data data;
  struct worker_pool *next_worker;
};

struct worker_state {
  int data_updated;
  int done;
  int result;
};

struct worker_data {
  char *curr_path;
  int sock_fd;
  enum WorkerOp op;
  int pipe_fd;
};

#endif
