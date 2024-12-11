BASE = src/main.c src/parse.c src/log.c
CC = gcc
CFLAGS = -O2 -Wall -Wextra
BUILD = target

all:
	$(CC) $(BASE) $(CFLAGS) -o $(BUILD)/as65

debug:
	$(CC) $(BASE) $(CFLAGS) -ggdb3 -g -O0 -o $(BUILD)/as65-debug

valgrind:
	$(CC) $(BASE) $(CFLAGS) -ggdb3 -g -O0 -o $(BUILD)/as65-debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose $(BUILD)/as65-debug test.as bin
	