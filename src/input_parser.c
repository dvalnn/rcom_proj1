#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_loginfo(char** input, char* username, char* password) {
    int bytes_read = 0;
    int scan_state = 0;

    // check for "username:password@host" format
    scan_state = sscanf(*input, "%255[^:@/]:%255[^:@/]@%n", username, password, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        return;
    }
    // check for "username@host" format
    scan_state = sscanf(*input, "%255[^:@/]@%n", username, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        password[0] = '\0';
        return;
    }
    // default case
    username[0] = '\0';
    password[0] = '\0';
}

int parse_host(char** input, char* host) {
    int bytes_read = 0;
    int scan_state = 0;

    scan_state = sscanf(*input, "%255[^:@/]:%n", host, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        return 2;  // read port next
    }

    scan_state = sscanf(*input, "%255[^:@/]/%n", host, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        return 1;  // read path next
    }

    scan_state = sscanf(*input, "%255[^:@/]%n", host, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        return 0;  // end of url
    }
    return -1;  // error
}

int parse_port(char** input, char* port) {
    int bytes_read = 0;
    int scan_state = 0;

    scan_state = sscanf(*input, "%6[0123456789]/%n", port, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        return 1;  // read path next
    }
    scan_state = sscanf(*input, "%6[0123456789]%n", port, &bytes_read);
    if (scan_state && bytes_read > 0) {
        *input += bytes_read;
        return 0;  // end of url
    }
    return -1;  // error
}

int parse_input(char* input, char* username, char* password, char* host, char* port, char* path) {
    int bytes_read = 0;
    char* pos = input;

    // remove ftp:// prefix
    sscanf(input, "ftp://%n", &bytes_read);
    pos += bytes_read;

    // check for username and password
    parse_loginfo(&pos, username, password);

    // parse for host
    int status = parse_host(&pos, host);
    switch (status) {
        case 2:
            status = parse_port(&pos, port);
            break;
            
        case 1:
            break;

        case 0:
            return 0;

        default:
            return -1;
    }

    // parse for file path
    sscanf(pos, "%256s", path);

    return 0;
}