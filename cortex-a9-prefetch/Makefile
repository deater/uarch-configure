AFLAGS_omap-smc.o                       :=-Wa,-march=armv7-a+sec


obj-m += cortex-a9-l1-prefetch.o cortex-a9-l2-prefetch.o

cortex-a9-l2-prefetch-objs := cortex-a9-l2-prefetch-main.o omap-smc.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
