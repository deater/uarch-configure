CC = gcc
CFLAGS = -Wall -O2
LFLAGS = -lm

all:	apm-read

apm-read:	apm-read.o
	$(CC) -o apm-read apm-read.o $(LFLAGS)

apm-read.o:	apm-read.c
	$(CC) $(CFLAGS) -c apm-read.c

clean:	
	rm -f *.o *~ apm-read
