#define _POSIX_SOURCE 1
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "app.h"

//! for reference only, don't remove comment
/*
? struct addrinfo //defined in <netdb.h>
{
    int     ai_flags;
    int     ai_family;  // protocol family IPV4: AF_INET | IPV6: AF_INET6
    int     ai_socktype;
    int     ai_protocol;
    size_t  ai_addrlen;
    struct  sockaddr *ai_addr;
    char    *ai_canonname;     // canonical name
    struct  addrinfo *ai_next; // this struct can form a linked list
}
*/
int get_address(char* hostname, char* port, struct addrinfo** result) {
    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // AF_INET for IPV4, AF_INET6 for IPV6 or AF_UNSPEC for either
    hints.ai_socktype = SOCK_STREAM;  // Define Socket as a stream socket
    hints.ai_flags = AI_CANONNAME;

    if ((status = getaddrinfo(hostname, port, &hints, result)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    return 0;
}

void print_address(struct addrinfo* info) {
    char ipstr[INET6_ADDRSTRLEN];
    printf("IPv4 address for %s:\n", info->ai_canonname);

    // get the pointer to the address itself
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)info->ai_addr;

    // convert the IP to a string and print it:
    void* addr = &(ipv4->sin_addr);  // inet_ntop needs a void pointer as an argument
    inet_ntop(info->ai_family, addr, ipstr, sizeof ipstr);
    printf(" IPV4: %s\n", ipstr);
}

uint64_t recv_to_file(int socketfd, const char* filename) {
    FILE* file = fopen(filename, "a");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", filename);
        return -1;
    }

    char incoming_buffer[MAX_BUFFER_SIZE];
    int received_bytes = 0;

    printf("Receiving data from host\n");
    received_bytes = recv(socketfd, incoming_buffer, MAX_BUFFER_SIZE - 1, 0);
    if (received_bytes == -1) {
        fprintf(stderr, "Data reception error");
        return -1;
    }
    printf("Received %d bytes: %s\n", received_bytes, incoming_buffer);
    fprintf(file, "%s", incoming_buffer);

    fclose(file);
    return received_bytes;
}

int connection_manager(int socket) {
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 500000;
    // char buf1[256];
    // char buf2[256];

    FD_ZERO(&readfds);

    // add our descriptors to the set
    FD_SET(socket, &readfds);
    // FD_SET(s2, &readfds);
    // since we got s2 second, it's the "greater", so we use that for
    // the n param in select()
    // wait until either socket has data ready to be recv()d (timeout 10.5 secs)

    printf("Listening to socket %d\n", socket);
    fflush(stdout);

    while (1) {
        int rv = select(socket + 1, &readfds, NULL, NULL, NULL);
        if (rv == -1) {
            perror("select");  // error occurred in select()
            break;
        }

        // one or both of the descriptors have data
        if (FD_ISSET(socket, &readfds)) {
            recv_to_file(socket, "received.txt");
        }
        // if (FD_ISSET(s2, &readfds)) {
        //     recv(s1, buf2, sizeof buf2, 0);
        // }
    }
    return 0;
}

int run(char* hostname, char* port) {
    struct addrinfo* host_info = NULL;

    if (get_address(hostname, port, &host_info)) {
        perror("get address failed");
        return 2;
    }

    print_address(host_info);

    // create a connection socket file descriptor
    int con_socket = socket(host_info->ai_family, host_info->ai_socktype, host_info->ai_protocol);
    if (con_socket < 0) {
        perror("socket failed");
        return 2;
    }

    // establish connection to remote server using created socket
    printf("Establishing connection to %s\n on port %s\n\n", hostname, port);
    int con_status = connect(con_socket, host_info->ai_addr, host_info->ai_addrlen);
    if (con_status < 0) {
        perror("connection failed");
        return 2;
    }

    printf("Connection Succcessfull\n\n");
    freeaddrinfo(host_info);  // free the linked list

    connection_manager(con_socket);

    close(con_socket);
    return 0;
}
