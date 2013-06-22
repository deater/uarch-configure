CC = gcc
CFLAGS = -O2 -Wall
LFLAGS = -lm

all:	rapl-read

rapl-read:	rapl-read.o
	$(CC) $(LFLAGS) -o rapl-read rapl-read.o

rapl-read.o:	rapl-read.c
	$(CC) $(CFLAGS) -c rapl-read.c

clean:	
	rm -f *.o *~ rapl-read

install:
	scp rapl-read.c vweaver@sasquatch.eece.maine.edu:public_html/projects/rapl
