CC = gcc
CFLAGS = -Wall -O2 -I/usr/include/libusb-1.0
LFLAGS = -L/usr/lib -lusb-1.0

all:	daqflex dump_binary dump_watts dump_watts_ddr3 debounce_serial dump_watts_ddr4

daqflex:	daqflex.o mccdevice.o poll_thread.o
	$(CC) -pthread -o daqflex \
		daqflex.o mccdevice.o poll_thread.o $(LFLAGS)

daqflex.o:	daqflex.c databuffer.h mccdevice.h poll_thread.h
	$(CC) $(CFLAGS) -c daqflex.c

mccdevice.o:	mccdevice.c mccdevice.h
	$(CC) $(CFLAGS) -c mccdevice.c

poll_thread.o:	poll_thread.c databuffer.h poll_thread.h
	$(CC) $(CFLAGS) -c poll_thread.c

dump_binary:	dump_binary.o
	$(CC) -o dump_binary dump_binary.o $(LFLAGS)

dump_binary.o:	dump_binary.c
	$(CC) $(CFLAGS) -c dump_binary.c

dump_watts:	dump_watts.o
	$(CC) -o dump_watts dump_watts.o $(LFLAGS)

dump_watts.o:	dump_watts.c
	$(CC) $(CFLAGS) -c dump_watts.c

dump_watts_ddr3:	dump_watts_ddr3.o
	$(CC) -o dump_watts_ddr3 dump_watts_ddr3.o $(LFLAGS)

dump_watts_ddr3.o:	dump_watts_ddr3.c
	$(CC) $(CFLAGS) -c dump_watts_ddr3.c


dump_watts_ddr4:	dump_watts_ddr4.o
	$(CC) -o dump_watts_ddr4 dump_watts_ddr4.o $(LFLAGS)

dump_watts_ddr4.o:	dump_watts_ddr4.c
	$(CC) $(CFLAGS) -c dump_watts_ddr4.c



debounce_serial:	debounce_serial.o
	$(CC) -o debounce_serial debounce_serial.o $(LFLAGS)

debounce_serial.o:	debounce_serial.c
	$(CC) $(CFLAGS) -c debounce_serial.c

clean:
	rm -f daqflex dump_binary dump_watts dump_watts_ddr3 dump_watts_ddr4 debounce_serial *.o *~
