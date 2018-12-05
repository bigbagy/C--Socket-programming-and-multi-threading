#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

#include "steque.h"
#include "gfserver.h"
#include "content.h"

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  gfserver_main [options]\n"                                                 \
"options:\n"                                                                  \
"  -t [nthreads]       Number of threads (Default: 3)\n"                      \
"  -p [listen_port]    Listen port (Default: 8803)\n"                         \
"  -m [content_file]   Content file mapping keys to content files\n"          \
"  -h                  Show this help message.\n"                             \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"nthreads",      required_argument,      NULL,           't'},
  {"content",       required_argument,      NULL,           'm'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};

typedef struct req_item {
  int cli_fd;
} req_item;

steque_t* q;
pthread_mutex_t mx;
pthread_cond_t cv;
pthread_t *threads;

extern ssize_t handler_get(gfcontext_t *ctx, char *path, void* arg);

static void _sig_handler(int signo) {
  if (signo == SIGINT || signo == SIGTERM) {
    exit(signo);
  }
}

struct gfcontext_t {
  unsigned int bytes_sent;
  unsigned int file_size;
  int cli_fd;
  char *path;
};

void error(char *msg) {
  perror(msg);
  exit(1);
}

char* read_path(int cli_fd) {
  //<scheme> <method> <path>\r\n\r\n
  char buffer[256];
  buffer[255] = '\0';
  int received;
  int current_size = 0;
  while (current_size < 4 || buffer[current_size - 3] != '\r') {
    received = recv(cli_fd, &buffer[current_size], 1, 0);
    if (received <= 0) {
      perror("connection failed while reading header");
      return "";
    }
    current_size += received;
  }
  char *path = (char*)malloc(512 * sizeof(char));
  sscanf(buffer, "GETFILE GET %s", path);

  return path;
}

void *worker(void *t) {
  
  //gfserver_t *gfs = (gfserver_t *)t;
  while (1) {
    pthread_mutex_lock(&mx);
    while (steque_size(q) == 0) {
      pthread_cond_wait(&cv, &mx);
    }
    // 1. deque a request from queue
    req_item *item = (req_item*) steque_pop(q);
    int cli_fd = item->cli_fd;
    free(item);
    pthread_mutex_unlock(&mx);
    // 2. get a socket
    gfcontext_t *ctx = malloc(sizeof(gfcontext_t));
    ctx->cli_fd = cli_fd;
    
    char *path = read_path(cli_fd);
    handler_get(ctx, path, 0);
    free(ctx);
    free(path);
  }
}


void gfserver_serve_mt(gfserver_t *gfs, int portno) {

  fprintf(stdout, "gfserver_serve start!\n");

  struct sockaddr_in echo_serve_addr, echo_client_addr;

  int socket_fd, cli_fd;
  unsigned int len;

  // 1. get socket()
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if ( socket_fd < 0 ) {
    error("socket call failed");
  }
  //The bzero() function erases the data in the n bytes of the memory
  // starting at the location pointed to by s, by writing zeroes (bytes
  // containing '\0') to that area.

  echo_serve_addr.sin_family = AF_INET;
  echo_serve_addr.sin_addr.s_addr = INADDR_ANY;
  echo_serve_addr.sin_port = htons(portno);
  bzero(&echo_serve_addr.sin_zero, 0);


  // 2. bind() to port
  int optval = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
  len = sizeof(struct sockaddr_in);
  if (bind(socket_fd, (struct sockaddr *) &echo_serve_addr, len) < 0) {
    error("socket bind failed");
  }

  // 3. start to listen()
  if (listen(socket_fd, 100) < 0) {
    error("socket listen failed");
  }

  // 4. accept() request --> receive/send/close
  for (;;) {
    if ((cli_fd = accept(socket_fd, (struct sockaddr*)&echo_client_addr, &len)) == -1) {
      error("socket accept failed");
    }

    req_item item = {cli_fd};

    pthread_mutex_lock(&mx);
    steque_item new_item = (steque_item) &item;
    steque_enqueue(q, new_item);
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mx);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  int option_char = 0;
  unsigned short port = 8803;
  char *content_map = "content.txt";
  gfserver_t *gfs = NULL;
  int nthreads = 3;

  if (signal(SIGINT, _sig_handler) == SIG_ERR) {
    fprintf(stderr, "Can't catch SIGINT...exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR) {
    fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
    exit(EXIT_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "m:p:t:h", gLongOptions, NULL)) != -1) {
    switch (option_char) {
    default:
      fprintf(stderr, "%s", USAGE);
      exit(1);
    case 'p': // listen-port
      port = atoi(optarg);
      break;
    case 't': // nthreads
      nthreads = atoi(optarg);
      break;
    case 'h': // help
      fprintf(stdout, "%s", USAGE);
      exit(0);
      break;
    case 'm': // file-path
      content_map = optarg;
      break;
    }
  }

  /* not useful, but it ensures the initial code builds without warnings */
  if (nthreads < 1) {
    nthreads = 1;
  }

  content_init(content_map);

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_port(gfs, port);
  gfserver_set_maxpending(gfs, 64);
  gfserver_set_handler(gfs, handler_get);
  gfserver_set_handlerarg(gfs, NULL);


  // initialize:
  // 1. request queue
  steque_init(q);
  // 2. mutexes & condition variable
  pthread_mutex_init(&mx, NULL);
  pthread_cond_init (&cv, NULL);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // start worker threads
  threads = malloc(nthreads * sizeof(pthread_t));
  for (int i = 0; i < nthreads; i++) {
    pthread_create(&threads[i], &attr, worker, (void *)gfs);
  }
  /*Loops forever*/
  gfserver_serve_mt(gfs, port);

  // clean up
  for (int i = 0; i < nthreads; i++) {
    pthread_join(threads[i], NULL);
  }

  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&mx);
  pthread_cond_destroy(&cv);
  steque_destroy(q);
  pthread_exit (NULL);
}
