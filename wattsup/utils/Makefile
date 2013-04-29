CC = gcc
CFLAGS = -Wall -O2
LFLAGS =

all:	make_plot

make_plot:	make_plot.o
	$(CC) $(LFLAGS) -o make_plot make_plot.o

make_plot.o:	make_plot.c
	$(CC) $(CFLAGS) -c make_plot.c

clean:	
	rm -f *~ *.o make_plot

