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

int run(char* hostname) {
    // printf("%s\n", argv1);
    // char* tcp_control_port = " 21";
    // char* tcp_data_port = "20";
    char* local_host_port = "3490";

    struct addrinfo* result = NULL;

    if (get_address(hostname, local_host_port, &result))
        perror("get address failed");

    print_address(result);

    // create a connection socket file descriptor
    int con_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (con_socket < 0)
        perror("socket failed");

    printf("socket descriptor number %d\n", con_socket);

    // establish connection to remote server using created socket
    int con_status = connect(con_socket, result->ai_addr, result->ai_addrlen);
    if (con_status < 0)
        perror("connection failed");

    printf("Connected to %s\n", result->ai_canonname);

    freeaddrinfo(result);  // free the linked list
    return 0;
}
