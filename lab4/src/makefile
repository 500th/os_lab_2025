
CC = gcc
CFLAGS = -I.
AR = ar

all: parallel_min_max process_memory psum

parallel_min_max: parallel_min_max.c find_min_max.c utils.c
	$(CC) $(CFLAGS) parallel_min_max.c find_min_max.c utils.c -o parallel_min_max

process_memory: process_memory.c
	$(CC) $(CFLAGS) process_memory.c -o process_memory

sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c sum.c -o sum.o

libsum.a: sum.o
	$(AR) rcs libsum.a sum.o

psum: parallel_sum.c utils.c sum.h libsum.a
	$(CC) $(CFLAGS) -pthread parallel_sum.c utils.c -L. -lsum -o psum

clean:
	rm -f parallel_min_max process_memory psum sum.o libsum.a
