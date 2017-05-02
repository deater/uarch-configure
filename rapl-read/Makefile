CC = gcc
CFLAGS = -O2 -Wall
LFLAGS = -lm

all:	rapl-read rapl-plot

rapl-read:	rapl-read.o
	$(CC) $(LFLAGS) -o rapl-read rapl-read.o

rapl-read.o:	rapl-read.c
	$(CC) $(CFLAGS) -c rapl-read.c


rapl-plot:	rapl-plot.o
	$(CC) $(LFLAGS) -o rapl-plot rapl-plot.o

rapl-plot.o:	rapl-plot.c
	$(CC) $(CFLAGS) -c rapl-plot.c


clean:	
	rm -f *.o *~ rapl-read rapl-plot

install:
	scp rapl-read.c vweaver@sasquatch.eece.maine.edu:public_html/projects/rapl
