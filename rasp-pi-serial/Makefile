CC = gcc
CFLAGS = -Wall -O2
LFLAGS =

all:	toggle_rts toggle_dtr rpirtscts pulse_rts


rpirtscts:	rpirtscts.o
	$(CC) $(LFLAGS) -o rpirtscts rpirtscts.o

rpirtscts.o:	rpirtscts.c
	$(CC) $(CFLAGS) -c rpirtscts.c


toggle_rts:	toggle_rts.o
	$(CC) $(LFLAGS) -o toggle_rts toggle_rts.o

toggle_rts.o:	toggle_rts.c
	$(CC) $(CFLAGS) -c toggle_rts.c


pulse_rts:	pulse_rts.o
	$(CC) $(LFLAGS) -o pulse_rts pulse_rts.o

pulse_rts.o:	pulse_rts.c
	$(CC) $(CFLAGS) -c pulse_rts.c


toggle_dtr:	toggle_dtr.o
	$(CC) $(LFLAGS) -o toggle_dtr toggle_dtr.o

toggle_dtr.o:	toggle_dtr.c
	$(CC) $(CFLAGS) -c toggle_dtr.c


clean:	
	rm -f *~ *.o toggle_rts toggle_dtr rpirtscts pulse_rts
