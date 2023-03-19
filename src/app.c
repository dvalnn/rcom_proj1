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
#include "input_parser.h"

int get_address(char* hostname, char* port, struct addrinfo** result) {
    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // AF_INET for IPV4, AF_INET6 for IPV6 or AF_UNSPEC for either
    hints.ai_socktype = SOCK_STREAM;  // Define Socket as a stream socket
    hints.ai_flags = AI_CANONNAME;

    if ((status = getaddrinfo(hostname, port, &hints, result)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    return 0;
}

int connect_to_host(struct addrinfo* host, char* port) {
    // create a connection socket file descriptor
    int con_socket = socket(host->ai_family, host->ai_socktype, host->ai_protocol);
    if (con_socket < 0) {
        perror("socket failed");
        return -1;
    }

    // establish connection to remote server using created socket
    printf("Establishing connection to %s\n on port %s\n\n", host->ai_canonname, port);
    int con_status = connect(con_socket, host->ai_addr, host->ai_addrlen);
    if (con_status < 0) {
        perror("connection failed");
        return -1;
    }
    return con_socket;
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

    printf("open\n");

    char incoming_buffer[BUFFER_SIZE];
    int received_bytes = 0;

    printf("Receiving data from host\n");
    received_bytes = recv(socketfd, incoming_buffer, BUFFER_SIZE - 1, 0);

    if (received_bytes == -1) {
        fprintf(stderr, "Data reception error");
        return -1;
    }

    printf("Received %d bytes: %s\n", received_bytes, incoming_buffer);
    fflush(stdout);
    fprintf(file, "%s", incoming_buffer);
    fflush(file);

    fclose(file);
    printf("close\n");
    return received_bytes;
}

int run(char* url) {
    char username[256] = "anonymous", password[256] = "", host[256] = "",
         port[6] = "21", path[256] = "", passive_host[INET_ADDRSTRLEN],
         passive_port[6];

    // struct addrinfo* host_info = NULL;
    parse_input(url, username, password, host, port, path);
    printf("username %s\n", username);
    printf("password %s\n", password);
    printf("host %s\n", host);
    printf("port %s\n", port);
    printf("path %s\n", path);

    // if (get_address(host, port, &host_info)) {
    //     perror("get address failed");
    //     return 2;
    // }
    // print_address(host_info);

    // int socket = connect_to_host(host_info, port);

    // freeaddrinfo(host_info);

    // close(socket);
    return 0;
}