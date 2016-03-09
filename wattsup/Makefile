CC = gcc
CFLAGS = -O2 -Wall
LFLAGS =

all:	wattsup wattsup-better wattsup-simple

wattsup-simple:       wattsup-simple.o
	       $(CC) $(LFLAGS) -o wattsup-simple wattsup-simple.o
	       
wattsup-simple.o:     wattsup-simple.c
	       $(CC) $(CFLAGS) -c wattsup-simple.c


wattsup:       wattsup.o
	       $(CC) $(LFLAGS) -o wattsup wattsup.o
	       
wattsup.o:     wattsup.c
	       $(CC) $(CFLAGS) -c wattsup.c
	       
wattsup-better:	wattsup-better.o
	$(CC) $(LFLAGS) -o wattsup-better wattsup-better.o

wattsup-better.o:	wattsup-better.c
	$(CC) $(CFLAGS) -c wattsup-better.c

clean:	       
	       rm -f *.o *~ wattsup wattsup-better wattsup-simple
