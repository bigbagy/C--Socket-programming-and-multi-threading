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

/* Be prepared accept a response of this length */
#define BUFSIZE 2000

#define USAGE                                                                       \
    "usage:\n"                                                                      \
    "  echoclient [options]\n"                                                      \
    "options:\n"                                                                    \
    "  -s                  Server (Default: localhost)\n"                           \
    "  -p                  Port (Default: 8803)\n"                                  \
    "  -m                  Message to send to server (Default: \"hello world.\")\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 8803;
    char *message = "hello world.";

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 'm': // message
            message = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == message)
    {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    struct sockaddr_in echo_serve_addr;
    int socket_fd;
    char buffer[16];
    int received;
    struct hostent *server;

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd < 0 ) {
      printf("socket call failed");
      exit(-1);
    }

    bcopy((char *)server->h_addr,  (char *)&echo_serve_addr.sin_addr.s_addr, server->h_length);
    echo_serve_addr.sin_family = AF_INET;
    //echo_serve_addr.sin_addr.s_addr = INADDR_ANY;
    echo_serve_addr.sin_port = htons(portno);
    bzero(&echo_serve_addr.sin_zero, 0);

    if (connect(socket_fd, (struct sockaddr*)&echo_serve_addr, sizeof(echo_serve_addr)) < 0) {
      printf("connect failed");
      exit(-1);
    }

    write(socket_fd, message, strlen(message));
    received = read(socket_fd, buffer, 16);
    buffer[received] = '\0';

    printf("%s", buffer);
}
