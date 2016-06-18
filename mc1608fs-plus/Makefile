CC = gcc
CFLAGS = -Wall -O2 -I/usr/include/libusb-1.0
LFLAGS = -L/usr/lib -lusb-1.0

daqflex:	daqflex.o mccdevice.o poll_thread.o
	$(CC) $(LFLAGS) -pthread -o daqflex \
		daqflex.o mccdevice.o poll_thread.o

daqflex.o:	daqflex.c databuffer.h mccdevice.h poll_thread.h
	$(CC) $(CFLAGS) -c daqflex.c
	
mccdevice.o:	mccdevice.c mccdevice.h
	$(CC) $(CFLAGS) -c mccdevice.c

poll_thread.o:	poll_thread.c databuffer.h poll_thread.h
	$(CC) $(CFLAGS) -c poll_thread.c

clean:
	rm -f daqflex *.o *~
