CC = gcc
CFLAGS = -pthread

all: client server

# Собираем общий код в статическую библиотеку.
common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

libcommon.a: common.o
	ar rcs libcommon.a common.o

# Подключаем общую библиотеку к обеим программам.
client: client.c common.h libcommon.a
	$(CC) $(CFLAGS) client.c -L. -lcommon -o client

server: server.c common.h libcommon.a
	$(CC) $(CFLAGS) server.c -L. -lcommon -o server

clean:
	rm -f client server common.o libcommon.a

.PHONY: all clean
