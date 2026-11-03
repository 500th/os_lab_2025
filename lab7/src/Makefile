CC = gcc

all: tcpserver tcpclient udpserver udpclient

tcpserver: tcpserver.c
	$(CC) tcpserver.c -o tcpserver

tcpclient: tcpclient.c
	$(CC) tcpclient.c -o tcpclient

udpserver: udpserver.c
	$(CC) udpserver.c -o udpserver

udpclient: udpclient.c
	$(CC) udpclient.c -o udpclient

clean:
	rm -f tcpserver tcpclient udpserver udpclient

.PHONY: all clean
