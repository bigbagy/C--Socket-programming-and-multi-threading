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
/*
 * Structs exported from netinet/in.h (for easy reference)
 */

/* Internet address */
struct in_addr {
    unsigned int s_addr;
};

/* Internet style socket address */
struct sockaddr_in  {
    unsigned short int sin_family; /* Address family */
    unsigned short int sin_port;   /* Port number */
    struct in_addr sin_addr;   /* IP address */
    unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
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

#define BUFSIZE 8803

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: cs8803.txt)\n" \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 8803)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}
};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char **argv)
{
    int option_char;
    int portno = 8803;             /* port to listen on */
    char *filename = "cs8803.txt"; /* file to transfer */

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:hf:", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'f': // listen-port
            filename = optarg;
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

    /* Socket Code Here */
    struct sockaddr_in echo_serve_addr, echo_client_addr;
    unsigned int buffer_size = 256;
    char buffer[buffer_size];
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
    if (listen(socket_fd, 10) < 0) {
        error("socket listen failed");
    }

    // 4. accept() request --> receive/send/close
    for (;;) {
        if ((cli_fd = accept(socket_fd, (struct sockaddr*)&echo_client_addr, &len)) == -1) {
            error("socket accept failed");
        }

        // 5. start to send file data
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
            error("File opern error");
        }

        for (;;) {
            int nread = fread(buffer, 1, buffer_size, fp);
            if (nread > 0) {
                int sent = 0;
                while(sent != nread) {
                    sent += send(cli_fd, &buffer[sent], nread-sent, 0);    
                }
            }

            if (nread < buffer_size) {
                if (ferror(fp)) {
                    error("Error reading\n");
                }
                // else: finish reading
                break;
            }
        }

        close(cli_fd);
    }
}
