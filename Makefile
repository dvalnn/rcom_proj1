# -*- Makefile -*-
# Makefile to build the project
# NOTE: This file must not be changed.

# Parameters
CC = gcc

DEBUG_LEVEL=2

# _DEBUG is used to include internal logging of errors and general information. Levels go from 1 to 3, highest to lowest priority respectively
# _PRINT_PACKET_DATA is used to print the packet data that is received by RX
CFLAGS = -Wall -g -D _DEBUG=$(DEBUG_LEVEL)

SRC = src/
INCLUDE = include/
BIN = bin/

FILE=/files/crab.mp4
HOST=rcom:rcom@netlab1.fe.up.pt
LOCAL_FILE_NAME = files/crab.mp4

# Targets
.PHONY: all
all: $(BIN)/main

$(BIN)/main: main.c $(SRC)/*.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCLUDE) -lrt

.PHONY: run
run: $(BIN)/main
	./$(BIN)/main ftp://$(HOST)/$(FILE) $(LOCAL_FILE_NAME)

docs: $(BIN)/main
	doxygen Doxyfile

.PHONY: clean
clean:
	rm -f $(BIN)/main
