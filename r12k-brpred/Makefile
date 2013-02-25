CROSS_COMPILE=mips-linux-
LOC=/home/vince/linux-mips/kernel/hacked-linux-2.6.22/

obj-m += brpred_config.o

all:
	make -C $(LOC) M=$(PWD) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(LOC) M=$(PWD) clean
