CC = gcc
CFLAGS = -Wall -O2
LFLAGS =

all:	apm-read

apm-read:	apm-read.o
	$(CC) $(LFLAGS) -o apm-read apm-read.c

apm-read.o:	apm-read.c
	$(CC) $(CFLAGS) -c apm-read.c

clean:	
	rm -f *.o *~ apm-read
