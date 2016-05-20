CROSS_COMPILE = arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld.bfd
OBJCOPY=$(CROSS_COMPILE)objcopy

all: bootblk.bin

pi2_secure_boot.o:
	$(CC) -O2 -march=armv7-a -nostartfiles -nostdlib pi2-secure-boot.s -c -o pi2-secure-boot.o

pi2_secure_boot: pi2_secure_boot.o
	$(LD) -pie --gc-sections -Bstatic -T pi2-secure-boot.lds -o pi2-secure-boot pi2-secure-boot.o

pi2_secure_boot.bin: pi2_secure_boot
	$(OBJCOPY) --gap-fill=0xff -O binary pi2-secure-boot pi2-secure-boot.bin

bootblk.bin: pi2_secure_boot.bin
	dd if=/dev/zero of=bootblk.bin bs=32k count=1
	dd if=pi2-secure-boot.bin of=bootblk.bin conv=notrunc
#	dd if=bcm2709-rpi-2-b.dtb of=bootblk.bin conv=notrunc bs=8k seek=1

clean:
	@rm -f *.o *.bin *~ pi2-secure-boot

