CC = gcc
CFLAGS = -O2 -Wall
LFLAGS = -lm

all:	rapl-read rapl-plot rapl-csv

rapl-read:	rapl-read.o
	$(CC) -o rapl-read rapl-read.o $(LFLAGS)

rapl-read.o:	rapl-read.c
	$(CC) $(CFLAGS) -c rapl-read.c


rapl-plot:	rapl-plot.o
	$(CC) -o rapl-plot rapl-plot.o $(LFLAGS)

rapl-plot.o:	rapl-plot.c
	$(CC) $(CFLAGS) -c rapl-plot.c

rapl-csv:      rapl-csv.o
	$(CC) -o rapl-csv rapl-csv.o $(LFLAGS)

rapl-csv.o:    rapl-csv.c
	$(CC) $(CFLAGS) -c rapl-csv.c


clean:	
	rm -f *.o *~ rapl-read rapl-plot rapl-csv

install:
	scp rapl-read.c vweaver@sasquatch.eece.maine.edu:public_html/projects/rapl
