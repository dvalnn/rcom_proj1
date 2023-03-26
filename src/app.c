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

ssize_t recv_all(int sock, char* buffer, size_t len) {
    ssize_t bytes_read_total = 0, bytes_read = 0;

    usleep(100000);

    while ((bytes_read = recv(sock, buffer + bytes_read_total,
                              len - bytes_read_total, MSG_DONTWAIT)) > 0) {
        bytes_read_total += bytes_read;
        usleep(100000);
    }

    buffer[bytes_read_total] = '\0';
    printf("Received %ld bytes: %s", bytes_read_total, buffer);

    return bytes_read_total;
}

int parse_code(int socket) {
    char buffer[BUFFER_SIZE] = "";
    int code = 0;

    size_t received_bytes = recv_all(socket, buffer, sizeof buffer);
    if (received_bytes < 0) {
        return -1;
    }

    sscanf(buffer, "%d", &code);
    printf("Received code %d\n", code);
    return code;
}

ssize_t send_all(int socket, const char* buffer, size_t len) {
    size_t total = 0;        // how many bytes we've sent
    size_t bytesleft = len;  // how many we have left to send
    int n;
    while (total < len) {
        n = send(socket, buffer + total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    return bytesleft;
}

int ftp_login(int socket, char* username, char* password) {
    int code = 0;

    code = parse_code(socket);
    if (code == -1)
        return -1;  // did not receive welcome message
    if (code == 230) {
        printf("login succesfull");
        return 0;
    }

    char user_buff[BUFFER_SIZE] = "USER ";
    strcat(user_buff, username);
    strcat(user_buff, "\n");
    printf("sending username: %s", user_buff);
    size_t not_sent = send_all(socket, user_buff, strlen(user_buff));
    if (not_sent) {
        perror("ftp_login failed - username not sent correctly\n");
        return -1;
    }

    code = parse_code(socket);
    if (code == -1)
        return -1;

    char pass_buff[BUFFER_SIZE] = "PASS ";
    strcat(pass_buff, password);
    strcat(pass_buff, "\n");
    printf("sending password: %s", pass_buff);
    not_sent = send_all(socket, pass_buff, strlen(pass_buff));
    if (not_sent) {
        perror("ftp_login failed - password not sent correctly\n");
        return -1;
    }

    code = parse_code(socket);
    if (code == -1)
        return -1;

    printf("Login successful\n");
    return 0;
}

int passive_mode(int socket) {
    char* buff[BUFFER_SIZE];
    size_t not_sent = send_all(socket, "PASV", strlen("PASV"));
    recv_all(socket, buff, BUFFER_SIZE);
    return 0;
}

int recv_to_file(int socket) {
    FILE* file = fopen("files/downloaded.txt", "w");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", "files/downloaded.hmtl");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read_total = 0, bytes_read = 0;

    usleep(100000);

    while ((bytes_read = recv(socket, buffer, sizeof buffer, MSG_DONTWAIT)) > 0) {
        bytes_read_total += bytes_read;
        fprintf(file, "%s", buffer);
        usleep(100000);
    }

    fclose(file);
    printf("close\n");
    return 0;
}

void retrieve_file(int sock, char* path) {
    char format[BUFFER_SIZE] = "retr ";
    strcat(format, path);
    strcat(format, "\n");
    printf("sending %s", format);
    send_all(sock, format, strlen(format));
    recv_to_file(sock);
}

int run(char* url) {
    char username[256] = "anonymous", password[256] = "", host[256] = "",
         port[6] = "21", path[256] = "", passive_host[INET_ADDRSTRLEN],
         passive_port[6];

    parse_input(url, username, password, host, port, path);
    printf("username %s\n", username);
    printf("password %s\n", password);
    printf("host %s\n", host);
    printf("port %s\n", port);
    printf("path %s\n", path);

    struct addrinfo* host_info;

    if (get_address(host, port, &host_info)) {
        perror("get address failed");
        return 2;
    }
    print_address(host_info);

    int socket = connect_to_host(host_info, port);

    ftp_login(socket, username, password);
    // retrieve_file(socket, path);
    //  sleep(5);

    freeaddrinfo(host_info);

    close(socket);
    return 0;
}