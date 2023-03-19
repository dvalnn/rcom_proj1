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
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", filename);
        return -1;
    }

    char incoming_buffer[RCV_BUFFER_SIZE];
    int received_bytes = 0;
    uint64_t total_received = 0;

    printf("Receiving data from host\n");
    while ((received_bytes = recv(socketfd, incoming_buffer, RCV_BUFFER_SIZE, 0)) != 0) {
        if (received_bytes == -1) {
            fprintf(stderr, "Data reception error at byte %ld", total_received);
            break;
        }
        total_received += received_bytes;
        fprintf(file, "%s", incoming_buffer);
        printf("received %d bytes for a total of %ld", received_bytes, total_received);
    }

    fclose(file);
    return total_received;
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

    char* buffer = "GET index.html";
    printf("Attempting to send %ld bytes: \"%s\"\n", strlen(buffer), buffer);

    int sent = send(con_socket, buffer, strlen(buffer), 0);
    if (sent < 0) {
        perror("send failed");
        return 2;
    }
    printf("Sent %d bytes\n", sent);

    uint64_t received = recv_to_file(con_socket, "received.html");
    if (received > 0)
        printf("Received %ld bytes\n", received);

    close(con_socket);
    printf("Connection closed\n");

    return 0;
}
