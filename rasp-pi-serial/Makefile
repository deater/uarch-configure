CC = gcc
CFLAGS = -Wall -O2
LFLAGS =

all:	toggle_rts toggle_dtr rpirtscts pulse_rts dump_serial_regs \
		dump_rts_cts monitor_rts_cts 


dump_rts_cts:	dump_rts_cts.o
	$(CC) $(LFLAGS) -o dump_rts_cts dump_rts_cts.o

dump_rts_cts.o:	dump_rts_cts.c
	$(CC) $(CFLAGS) -c dump_rts_cts.c


monitor_rts_cts:	monitor_rts_cts.o
	$(CC) $(LFLAGS) -o monitor_rts_cts monitor_rts_cts.o

monitor_rts_cts.o:	monitor_rts_cts.c
	$(CC) $(CFLAGS) -c monitor_rts_cts.c



dump_serial_regs:	dump_serial_regs.o
	$(CC) $(LFLAGS) -o dump_serial_regs dump_serial_regs.o

dump_serial_regs.o:	dump_serial_regs.c
	$(CC) $(CFLAGS) -c dump_serial_regs.c



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
	rm -f *~ *.o toggle_rts toggle_dtr rpirtscts pulse_rts dump_serial_regs dump_rts_cts monitor_rts_cts
