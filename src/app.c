#define _POSIX_SOURCE 1
#define _GNU_SOURCE

#include "app.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "input_parser.h"

unsigned long file_size = 0L;
unsigned long current_progress = 0L;

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

int parse_code(int fd) {
    char buff[BUFFER_SIZE] = "";
    int code = -1;

    if (recv_all(fd, buff, BUFFER_SIZE) < 0)
        return -1;

    sscanf(buff, "%d", &code);

    int n = 0;
    unsigned long temp;
    if (sscanf(buff, " %*[^(](%lu bytes)%n", &temp, &n) && n > 0) {
        file_size = temp;
        printf("File size: %lu bytes\n", file_size);
    }

    return code;
}

int ftp_login(int socket, char* username, char* password) {
    int code = 0;
    int len;

    code = parse_code(socket);
    if (code == -1)
        return -1;  // did not receive welcome message
    if (code == 230) {
        printf("login succesfull");
        return 0;
    }

    char user_buff[BUFFER_SIZE];

    sprintf(user_buff, "USER %s\n%n", username, &len);
    printf("sending username: %s", user_buff);

    size_t sent = send(socket, user_buff, len, 0);
    if (!sent) {
        perror("ftp_login failed - username not sent correctly\n");
        return -1;
    }

    code = parse_code(socket);
    if (code == -1)
        return -1;

    char pass_buff[BUFFER_SIZE];

    sprintf(pass_buff, "PASS %s\n%n", password, &len);
    printf("sending password: %s", pass_buff);

    sent = send(socket, pass_buff, len, 0);
    if (!sent) {
        perror("ftp_login failed - password not sent correctly\n");
        return -1;
    }

    code = parse_code(socket);
    if (code == -1)
        return -1;

    printf("Login successful\n");
    return 0;
}

int passive_mode(int socket, char* passive_host, char* passive_port) {
    char buffer[BUFFER_SIZE];
    char* msg = "pasv\n";

    send(socket, msg, strlen(msg), 0);
    recv_all(socket, buffer, BUFFER_SIZE);

    int buff[6];
    sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &buff[0], &buff[1], &buff[2], &buff[3], &buff[4], &buff[5]);
    sprintf(passive_host, "%d.%d.%d.%d", buff[0], buff[1], buff[2], buff[3]);
    sprintf(passive_port, "%d", buff[4] * 256 + buff[5]);

    printf("Passive Mode Data:\n\tHost - %s\n\tPort - %s\n", passive_host, passive_port);

    return 0;
}

ssize_t recvall(int fd, char* buff, size_t len) {
    ssize_t bytes_read_total = 0, bytes_read = 0;

    usleep(100000);

    while ((bytes_read = recv(fd, buff + bytes_read_total,
                              len - bytes_read_total, MSG_DONTWAIT)) > 0) {
        bytes_read_total += bytes_read;
        usleep(100000);
    }

    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        printf("%s\n", strerror(errno));
        return -1;
    }

    buff[bytes_read_total] = '\0';
    if (bytes_read_total > 0)
        printf("%s", buff);

    return bytes_read_total;
}

// void retrieve_file(int socket, char* path, char* local_path) {
//     char format[BUFFER_SIZE];
//     int len;

//     snprintf(format, "retr %s\n%n", path, &len);

//     int sent = send(socket, format, len, 0);
//     if (sent < 0) {
//         printf("Error sending command: %s\n", strerror(errno));
//         return;
//     }

//     printf("sent %s", format);

//     int code = parse_code(socket);
//     if (code < 0) return;

//     ssize_t bytes_read;
//     uint8_t buff[BUFFER_SIZE];

//     int out_fd = open(local_path, O_CREAT, 0744);
//     if (!out_fd) {
//         printf("Could not open %s\n", local_path);
//         return;
//     }

//     printf("Starting transfer\n");
//     while ((bytes_read = recv(socket, buff, BUFFER_SIZE, 0)) > 0) {
//         printf("hello\n");
//         ssize_t bytes_written = write(out_fd, buff, bytes_read);
//         printf("Written %lu bytes to file", bytes_written);
//     }

//     close(out_fd);
// }

int get_code(int fd) {
    char buf[2048] = "";
    int code = -1;

    if (recvall(fd, buf, 2048) < 0)
        return -1;

    sscanf(buf, "%d", &code);

    int n = 0;
    unsigned long temp;
    if (sscanf(buf, " %*[^(](%lu bytes)%n", &temp, &n) && n > 0) {
        file_size = temp;
        printf("File size: %lu bytes\n", file_size);
    }

    return code;
}

int start_transfer(int fd, const char* path) {
    char buf[512];
    int len;

    snprintf(buf, sizeof buf, "retr %s\n%n", path, &len);
    if (send(fd, buf, len, 0) < 0) {
        printf("Error sending 'retr' command: %s\n", strerror(errno));
        return -1;
    }

    int code = get_code(fd);

    printf("%s",code);
    if (code != 150 && code != 226) {
        printf("Could not retrieve file\n");
        return -1;
    }

    printf("Retrieving file\n");
    return 0;
}

void retrieve_file(int socket, char* path, char* local_path) {
    if (start_transfer(socket, path) < 0) return;

    int out_fd = creat(local_path, 0744);
    uint8_t buf[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = recv(socket, buf, BUFFER_SIZE, 0)) > 0) {
        ssize_t bytes_written = write(out_fd, buf, bytes_read);

        current_progress += bytes_written;

        printf("Written %lu bytes (%lf%%) to file\n", bytes_written, (double)(current_progress * 100.0 / file_size));
        printf("%.*s\n", (int)bytes_read, buf);
    }

    close(out_fd);

    printf("Transfer complete\n");

    return;
}

int run(char* url, char* local_file_name) {
    char username[256] = "anonymous", password[256] = "", host[256] = "",
         port[6] = "21", path[256] = "", passive_host[INET_ADDRSTRLEN] = "",
         passive_port[6] = "";

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

    freeaddrinfo(host_info);

    ftp_login(socket, username, password);
    passive_mode(socket, passive_host, passive_port);

    if (get_address(passive_host, passive_port, &host_info)) {
        perror("get address failed");
        return 2;
    }

    int data_socket = connect_to_host(host_info, passive_port);
    freeaddrinfo(host_info);

    retrieve_file(data_socket, path, local_file_name);

    close(socket);
    close(data_socket);

    return 0;
}