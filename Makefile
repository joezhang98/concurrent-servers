# Makefile

CC = gcc
CCFLAGS = -std=gnu99 -Wall -O3 -g -DNDEBUG -pthread
LDFLAGS = -lpthread -pthread

EXECUTABLES = \
				sequential-server \
				threaded-server \
				threadspammer

all: $(EXECUTABLES)

sequential-server: utils.c sequential-server.c
				$(CC) $(CCFLAGS) $^ -o $@ $(LDFLAGS)

threaded-server: utils.c threaded-server.c
				$(CC) $(CCFLAGS) $^ -o $@ $(LDFLAGS)

threadspammer: threadspammer.c
				$(CC) $(CCFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean format

clean:
				rm -f $(EXECUTABLES) *.o

format:
				clang-format -style=file -i *.c *.h
