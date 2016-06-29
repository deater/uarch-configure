CC = gcc
CFLAGS = -Wall -O2 -I/usr/include/libusb-1.0
LFLAGS = -L/usr/lib -lusb-1.0

all:	daqflex dump_binary dump_watts dump_watts_multiple debounce_serial

daqflex:	daqflex.o mccdevice.o poll_thread.o
	$(CC) $(LFLAGS) -pthread -o daqflex \
		daqflex.o mccdevice.o poll_thread.o

daqflex.o:	daqflex.c databuffer.h mccdevice.h poll_thread.h
	$(CC) $(CFLAGS) -c daqflex.c
	
mccdevice.o:	mccdevice.c mccdevice.h
	$(CC) $(CFLAGS) -c mccdevice.c

poll_thread.o:	poll_thread.c databuffer.h poll_thread.h
	$(CC) $(CFLAGS) -c poll_thread.c

dump_binary:	dump_binary.o
	$(CC) $(LFLAGS) -o dump_binary dump_binary.o

dump_binary.o:	dump_binary.c
	$(CC) $(CFLAGS) -c dump_binary.c

dump_watts:	dump_watts.o
	$(CC) $(LFLAGS) -o dump_watts dump_watts.o

dump_watts.o:	dump_watts.c
	$(CC) $(CFLAGS) -c dump_watts.c

dump_watts_multiple:	dump_watts_multiple.o
	$(CC) $(LFLAGS) -o dump_watts_multiple dump_watts_multiple.o

dump_watts_multiple.o:	dump_watts_multiple.c
	$(CC) $(CFLAGS) -c dump_watts_multiple.c

debounce_serial:	debounce_serial.o
	$(CC) $(LFLAGS) -o debounce_serial debounce_serial.o

debounce_serial.o:	debounce_serial.c
	$(CC) $(CFLAGS) -c debounce_serial.c

clean:
	rm -f daqflex dump_binary dump_watts dump_watts_multiple debounce_serial *.o *~
