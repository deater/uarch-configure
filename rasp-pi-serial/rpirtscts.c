/*
    A command-line utility for enabling hardware flow control on the
    Raspberry Pi serial port.

    Copyright (C) 2013 Matthew Hollingworth.

    40 pin header support for newer Raspberry Pis
    Copyright (C) 2016 Brendan Traw.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

static int header_type = GPIO_HEADER_40;
static int gpio_base = PI_GPIO_BASE;

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
			break;
		case 0xa01041:
		case 0xa21041:
			printf("Pi 2 Model B Rev 1.1 with 40 pin\n");
			gpio_base=PI2_GPIO_BASE;
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
			break;
		default:
			printf("Unknown Raspberry Pi - assuming 40 pin\n");
	}
	return header_type;
}


static void set_rts_cts(int enable, int header_type) {

	uint32_t gfpsel, gpiomask;
	int fd;
	void *gpio_map;
	uint32_t temp;

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "can't open /dev/mem (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd, gpio_base);
	close(fd);

	if (gpio_map == MAP_FAILED) {
		fprintf(stderr, "mmap error (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	volatile uint32_t *gpio = (volatile uint32_t *)gpio_map;

	if (enable) printf("Enabling ");
	else printf("Disabling ");

	/* newer 40 pin GPIO header */
	if (header_type == GPIO_HEADER_40) {
		gfpsel = GFPSEL1;
		gpiomask = GPIO1617MASK;
		printf("CTS0 and RTS0 on GPIOs 16 and 17\n");
	}
	else { /* 26 pin GPIO header */
		gfpsel = GFPSEL3;
		gpiomask = GPIO3031MASK;
		printf("CTS0 and RTS0 on GPIOs 30 and 31\n");
	}

	if (enable) {
		temp=gpio[gfpsel];
		printf("Before: %x mask %x\n",temp,gpiomask);

		/* clear out old value */
		temp &= ~gpiomask;

		/* Set to ALT3 = 111 */
		temp |= gpiomask;

		gpio[gfpsel] = temp;

		printf("After: %x\n",gpio[gfpsel]);

		/* GPIO14/15 = ALT0/ALT0 for pl101 uart */
		/* 10 0100 = 0x24 */
		/* GPIO14/15 = ALT5/ALT5 for mini-uart */
		/* 01 0010 = 0x12 */

		printf("GPIO14/15=%x Expect ALT0/ALT0 0x24\n",
			(temp>>12)&0x3f);

	}
	else {
		/* clear to 0.  Input? */
		printf("Before: %x\n",gpio[gfpsel]);
		gpio[gfpsel] &= ~gpiomask;
		printf("After: %x\n",gpio[gfpsel]);

	}
}

static void print_usage(void) {

	printf( "Version: " VERSION "\n");
	printf("Usage: rpirtscts on|off\n");
	printf("Enable or disable hardware flow control pins on ttyAMA0.\n");
	printf("\nFor 26 pin GPIO header boards:\n");
	printf("P5 header pins remap as follows:\n");
	printf("    P5-05 (GPIO30) -> CTS (input)\n");
	printf("    P5-06 (GPIO31) -> RTS (output)\n");
	printf("\nFor 40 pin GPIO header boards:\n");
	printf("    P1-36 (GPIO16) -> CTS (input)\n");
	printf("    P1-11 (GPIO17) -> RTS (output)\n");
	printf("\nYou may also need to enable flow control in the driver:\n");
	printf("    stty -F /dev/ttyAMA0 crtscts\n");
}

int main(int argc, char **argv) {

	int enable,disable,version,header_type;

	version=rpi_version();
	header_type=rpi_gpio_header_type(version);

	if (argc != 2) {
		print_usage();
	} else {
		enable = !strcmp(argv[1],  "on");
		disable = !strcmp(argv[1], "off");

		if (enable) set_rts_cts(1,header_type);
		else if (disable) set_rts_cts(0,header_type);
		else print_usage();
	}
	return EXIT_SUCCESS;
}
