CC = gcc
CFLAGS = -O2 -Wall
LFLAGS =

all:	wattsup

wattsup:       wattsup.o
	       $(CC) $(LFLAGS) -o wattsup wattsup.o
	       
wattsup.o:     wattsup.c
	       $(CC) $(CFLAGS) -c wattsup.c
	       
clean:	       
	       rm -f *.o *~ wattsup
	       
