all:	disable_core2_prefetch enable_core2_prefetch

CC = gcc
CFLAGS = -O2 -Wall -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

disable_core2_prefetch:	disable_core2_prefetch.o
	$(CC) $(LFLAGS) -o disable_core2_prefetch disable_core2_prefetch.o

disable_core2_prefetch.o:	disable_core2_prefetch.c
	$(CC) $(CFLAGS) -c disable_core2_prefetch.c


enable_core2_prefetch:	enable_core2_prefetch.o
	$(CC) $(LFLAGS) -o enable_core2_prefetch enable_core2_prefetch.o

enable_core2_prefetch.o:	enable_core2_prefetch.c
	$(CC) $(CFLAGS) -c enable_core2_prefetch.c

clean:	
	rm -f *~ *.o disable_core2_prefetch enable_core2_prefetch
