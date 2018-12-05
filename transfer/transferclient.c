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

#define BUFSIZE 2000

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferclient [options]\n"                           \
    "options:\n"                                             \
    "  -s                  Server (Default: localhost)\n"    \
    "  -p                  Port (Default: 8803)\n"           \
    "  -o                  Output file (Default gios.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 8803;
    char *filename = "gios.txt";

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:h", gLongOptions, NULL)) != -1)
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
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
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
    char buffer[256];
    int received;
    struct hostent *server;

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd < 0 ) {
        error("socket call failed");
    }

    bcopy((char *)server->h_addr,  (char *)&echo_serve_addr.sin_addr.s_addr, server->h_length);
    echo_serve_addr.sin_family = AF_INET;
    echo_serve_addr.sin_port = htons(portno);
    bzero(&echo_serve_addr.sin_zero, 0);

    if (connect(socket_fd, (struct sockaddr*)&echo_serve_addr, sizeof(echo_serve_addr)) < 0) {
        error("connect failed");
    }

    FILE *file_ptr;
    file_ptr = fopen(filename, "ab");
    if (NULL == file_ptr) {
        error("opening file error");
    }

    while ((received = read(socket_fd, buffer, 256)) > 0) {
        fwrite(buffer, 1, received, file_ptr);
    }

    if (received < 0) {
        error("read error");
    }
}
