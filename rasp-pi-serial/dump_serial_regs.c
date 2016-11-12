/*
	Dump the rasp-pi current serial settings.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define VERSION "2.0"

/* See bcm2835 peripheral guide */

#define PI2_BASE 0x3F000000
#define PI_BASE  0x20000000


#define PI_GPIO_BASE (PI_BASE + 0x200000)
#define PI2_GPIO_BASE (PI2_BASE + 0x200000)

#define PI_SERIAL_BASE (PI_BASE + 0x201000)
#define PI2_SERIAL_BASE (PI2_BASE + 0x201000)


#define GFPSEL0 (0)
#define GFPSEL1 (1)
#define GFPSEL2 (2)
#define GFPSEL3 (3)


/* bits 18-23 */
#define GPIO1617MASK 0x00fc0000 /* GPIO 16 for CTS0 and 17 for RTS0 */
#define GPIO3031MASK 0x0000003f /* GPIO 30 for CTS0 and 31 for RTS0 */

#define GPIO_HEADER_26 0x00
#define GPIO_HEADER_40 0x01


#define BLOCK_SIZE (4*1024)

static uint32_t gpio_base = PI_GPIO_BASE;
static uint32_t serial_base = PI_SERIAL_BASE;
static int header_type = GPIO_HEADER_40;

static int rpi_version(void) {

	int result = -1;
	char string[256];

	FILE *fp;

	fp = fopen("/proc/cmdline", "r");
	if (fp) {
		while (fscanf(fp, "%255s", string) == 1) {
			if (sscanf(string, "bcm2708.boardrev=%i", &result))
				break;
		}

		if (result < 0) {
			rewind(fp);
			while (fscanf(fp, "%255s", string) == 1)
				if (sscanf(string, "bcm2709.boardrev=%i", &result))
				break;
		}

		fclose(fp);
	}
	if (result < 0) {
		fprintf(stderr, "can't parse /proc/cmdline\n");
		exit(EXIT_FAILURE);
	}
	return result;
}

static int rpi_gpio_header_type(int version) {


	switch (version) {
		case 0x000002:
			printf("Model B Rev 1.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000003:
			printf("Model B Rev 1.0+ with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000004:
			printf("Model B Rev 2.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000005:
			printf("Model B Rev 2.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000006:
			printf("Model B Rev 2.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000007:
			printf("Model A with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000008:
			printf("Model A with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000009:
			printf("Model A with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x00000d:
			printf("Model B Rev 2.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x00000e:
			printf("Model B Rev 2.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x00000f:
			printf("Model B Rev 2.0 with 26 pin\n");
			header_type = GPIO_HEADER_26;
			break;
		case 0x000010:
			printf("Model B+ Rev 1.0 with 40 pin\n");
			break;
		case 0x000014:
		case 0x000011:
			printf("Compute Module is not supported\n");
			exit(EXIT_FAILURE);
		case 0x000015:
		case 0x000012:
			printf("Model A+ Rev 1.1 with 40 pin\n");
			break;
		case 0x000013:
			printf("Model B+ Rev 1.2 with 40 pin\n");
			break;
		case 0xa01040:
			printf("Pi 2 Model B Rev 1.0 with 40 pin\n");
			gpio_base=PI2_GPIO_BASE;
			serial_base=PI2_SERIAL_BASE;
			break;
		case 0xa01041:
		case 0xa21041:
			printf("Pi 2 Model B Rev 1.1 with 40 pin\n");
			gpio_base=PI2_GPIO_BASE;
			serial_base=PI2_SERIAL_BASE;
			break;
		case 0x900092:
			printf("Pi Zero Rev 1.2 with 40 pin\n");
			break;
		case 0x900093:
			printf("Pi Zero Rev 1.3 with 40 pin\n");
			break;
		case 0xa22082:
		case 0xa02082:
			printf("Pi 3 Model B Rev 1.2 with 40 pin\n");
			gpio_base=PI2_GPIO_BASE;
			serial_base=PI2_SERIAL_BASE;
			break;
		default:
			printf("Unknown Raspberry Pi - assuming 40 pin\n");
	}
	return header_type;
}


static void dump_serial_regs(int header_type) {

	int fd;
	void *gpio_map,*serial_map;
	uint32_t temp;
	uint32_t tx_gpio,rx_gpio,cts_gpio,rts_gpio;

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "can't open /dev/mem (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Map GPIO region */
	gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd, gpio_base);


	/* Map Serial region */
	serial_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd, serial_base);

	close(fd);

	if (gpio_map == MAP_FAILED) {
		fprintf(stderr, "gpio mmap error (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (serial_map == MAP_FAILED) {
		fprintf(stderr, "serial mmap error (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


	volatile uint32_t *gpio = (volatile uint32_t *)gpio_map;
	volatile uint32_t *serial = (volatile uint32_t *)serial_map;

	printf("*** Raspberry Pi Curent Serial Settings ***\n\n");

	printf("GPIO pin settings:\n");

	temp=gpio[GFPSEL1];
	tx_gpio=(temp>>12)&0x7;
	rx_gpio=(temp>>15)&0x7;

	/* 0 = input */
	/* 1 = output */
	/* 4 = ALT0 */
	/* 5 = ALT1 */
	/* 6 = ALT2 */
	/* 7 = ALT3 */
	/* 3 = ALT4 */
	/* 2 = ALT5 */

	printf("\tPIN14/TX = ");
	if (tx_gpio==4) printf("AMBA (ALT0)\n");
	else if (tx_gpio==2) printf("mini-uart (ALT5)\n");
	else printf("Unknown (%d)\n",tx_gpio);

	printf("\tPIN15/RX = ");
	if (rx_gpio==4) printf("AMBA (ALT0)\n");
	else if (rx_gpio==2) printf("mini-uart (ALT5)\n");
	else printf("Unknown (%d)\n",rx_gpio);


	if (header_type == GPIO_HEADER_40) {
		temp=gpio[GFPSEL1];
		cts_gpio=(temp>>18)&0x7;
		rts_gpio=(temp>>21)&0x7;

		printf("\tPIN16/CTS = ");
		if (cts_gpio==0) printf("not configured\n");
		else if (cts_gpio==7) printf("AMBA (ALT3)\n");
		else if (cts_gpio==2) printf("mini-uart (ALT5)\n");
		else printf("Unknown (%d)\n",cts_gpio);

		printf("\tPIN17/RTS = ");
		if (rts_gpio==0) printf("not configured\n");
		else if (rts_gpio==7) printf("AMBA (ALT3)\n");
		else if (rts_gpio==2) printf("mini-uart (ALT5)\n");
		else printf("Unknown (%d)\n",rts_gpio);

	}
	else {
		printf("\tPIN30/CTS = \n");
		printf("\tPIN31/RTS = \n");
	}

	printf("\nAMBA Serial Port Settings:\n");

#define UART_FR		6
	temp=serial[UART_FR];
	printf("\tFR register: %x\n",temp);

	/* TXFE */
	if (temp&(1<<7)) printf("\t\tTX FIFO Empty\n");
	else printf("\t\tTX FIFO not empty\n");

	/* RXFF */
	if (temp&(1<<6)) printf("\t\tRX FIFO Full\n");
	else printf("\t\tRX FIFO not full\n");

	/* TXFF */
	if (temp&(1<<5)) printf("\t\tTX FIFO Full\n");
	else printf("\t\tTX FIFO not full\n");

	/* RXFE */
	if (temp&(1<<4)) printf("\t\tRX FIFO Empty\n");
	else printf("\t\tRX FIFO not empty\n");

	/* BUSY */
	if (temp&(1<<3)) printf("\t\tUART BUSY\n");
	else printf("\t\tUART not busy\n");

	/* CTS */
	if (temp&(1<<0)) printf("\t\tCTS pin is low\n");
	else printf("\t\tCTS pin is high\n");


#define UART_IBRD	9
#define UART_FBRD	10

	int ip,fp;
	double divider;

	ip=serial[UART_IBRD];
	fp=serial[UART_FBRD];
	divider=(double)ip+((double)fp-0.5)/64.0;

	printf("\tIBRD register: %x, FBRD register: %x, Divider=%lf\n",
		ip,fp,divider);
	printf("\t\tBaud=%.0lf\n",3000000.0/(divider));

#define UART_LCRH	11
	temp=serial[UART_LCRH];
	printf("\tLCRH register: %x\n",temp);

	printf("\t\tWord size: %d bits, ",((temp>>5)&0x3)+5);

	printf("Parity: ");
	if (temp&(1<<7)) {
		printf("stick, ");
	}
	if (temp&(1<<1)) {
		if (temp&(1<<2)) {
			printf("even, ");
		}
		else {
			printf("odd, ");
		}
	}
	else {
		printf("none, ");
	}

	/* STP2 */
	if (temp&(1<<3)) printf("Stop Bits: 2\n");
	else printf("Stop Bits: 1\n");



	/* FEN */
	if (temp&(1<<4)) printf("\t\tFIFO Enabled\n");
	else printf("\t\tFIFO disabled\n");


#define UART_CR		12
	temp=serial[UART_CR];
	printf("\tCR register: %x\n",temp);

	/* CTSEN */
	if (temp&(1<<15)) printf("\t\tCTS Enabled\n");
	else printf("\t\tCTS Disabled\n");

	/* RTSEN */
	if (temp&(1<<14)) printf("\t\tRTS Enabled\n");
	else printf("\t\tRTS Disabled\n");

	/* RTS */
	if (temp&(1<<11)) printf("\t\tRTS Pin is 0\n");
	else printf("\t\tRTS Pin is 1\n");

	/* RXE */
	if (temp&(1<<9)) printf("\t\tRX enabled\n");
	else printf("\t\tRX disabled\n");

	/* TXE */
	if (temp&(1<<8)) printf("\t\tTX enabled\n");
	else printf("\t\tTX disabled\n");

	/* LBE */
	if (temp&(1<<7)) printf("\t\tLoopback enabled\n");
	else printf("\t\tLoopback disabled\n");

	/* LBE */
	if (temp&(1<<0)) printf("\t\tUART enabled\n");
	else printf("\t\tUART disabled\n");


#define UART_IMSC	14
	temp=serial[UART_IMSC];
	printf("\tIMSC register: %x\n",temp);

}

int main(int argc, char **argv) {

	int version,header_type;

	version=rpi_version();
	header_type=rpi_gpio_header_type(version);


	dump_serial_regs(header_type);

	return EXIT_SUCCESS;
}
