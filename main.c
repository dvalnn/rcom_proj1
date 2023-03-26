#include <stdio.h>
#include <stdlib.h>

#include "app.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s url\n", argv[0]);
        exit(1);
    }

    return run(argv[1], argv[2]);
}