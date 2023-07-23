CC = gcc
CFLAGS = -Wall

allocate: allocate.c memory.o scheduling.o
	$(CC) $(CFLAGS) -o allocate allocate.c memory.o scheduling.o -lm

process: process.c
	$(CC) $(CFLAGS) -o process process.c -lm

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c 

scheduling.o: scheduling.c scheduling.h
	$(CC) $(CFLAGS) -c scheduling.c

clean:
	rm -f memory.o scheduling.o allocate process

