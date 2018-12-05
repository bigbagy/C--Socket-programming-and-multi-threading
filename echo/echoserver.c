#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <getopt.h>

#if 0
/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/* 
 * Structs exported from netinet/in.h (for easy reference)
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif

#define BUFSIZE 2000

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 8803)\n"                                \
"  -m                  Maximum pending connections (default: 8)\n"            \
"  -h                  Show this help message\n"                              \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"maxnpending",   required_argument,      NULL,           'm'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};


int main(int argc, char **argv) {
  int option_char;
  int portno = 8803; /* port to listen on */
  int maxnpending = 8;
  
  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:m:h", gLongOptions, NULL)) != -1) {
   switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'p': // listen-port
        portno = atoi(optarg);
        break;                                        
      case 'm': // server
        maxnpending = atoi(optarg);
        break; 
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }

    if (maxnpending < 1) {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
    // socket --> bind --> listen --> accept
    struct sockaddr_in echo_serve_addr, echo_client_addr;
    unsigned int buffer_size = 16; 
    char buffer[buffer_size];
    //char hello[] = "Hello world!\n";
    int socket_fd, cli_fd;
    unsigned int len;
    int received; //, sent

    // 1. get socket()
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd < 0 ) {
      printf("socket call failed");
      exit(-1);
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
    if(bind(socket_fd, (struct sockaddr *) &echo_serve_addr, len) < 0) {
      printf("socket bind failed");
      exit(-1);
    }

    // 3. start to listen()
    if(listen(socket_fd, maxnpending) < 0) {
      printf("socket listen failed");
      exit(-1);
    }

    // 4. accept() request --> receive/send/close
    for(;;) {
      if ((cli_fd = accept(socket_fd, (struct sockaddr*)&echo_client_addr, &len)) == -1){
        printf("socket accept failed");
        exit(-1);
      }
      received = recv(cli_fd, buffer, buffer_size, 0);
      buffer[received] = '\0';
      printf("%s", buffer);
      send(cli_fd, buffer, received, 0);
      //printf("Sent %d bytes to client\n", sent);
      close(cli_fd);
    }
}
