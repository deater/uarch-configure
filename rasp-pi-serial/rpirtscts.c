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

#define GPIO_BASE (0x20200000)
#define BLOCK_SIZE (4*1024)
#define GFPSEL3 (3)
#define GPIO3031mask 0x0000003f /* GPIO 30 for CTS0 and 31 for RTS0 */
#define GFPSEL1 (1)
#define GPIO1617mask 0x00fc0000 /* GPIO 16 for CTS0 and 17 for RTS0 */

#define GPIO_header_26 0x00
#define GPIO_header_40 0x01

#define VERSION "1.2"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int rpi_version(void) {

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

int rpi_gpio_header_type(void) {
	int header_type = GPIO_header_40;
	switch (rpi_version()) { /* Adapted from http://www.raspberrypi-spy.co.uk/2012/09/checking-your-raspberry-pi-board-version/ */
	case 0x000002: printf("Model B Rev 1.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000003: printf("Model B Rev 1.0+ with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000004: printf("Model B Rev 2.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000005: printf("Model B Rev 2.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000006: printf("Model B Rev 2.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000007: printf("Model A with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000008: printf("Model A with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000009: printf("Model A with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x00000d: printf("Model B Rev 2.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x00000e: printf("Model B Rev 2.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x00000f: printf("Model B Rev 2.0 with 26 pin GPIO header detected\n"); header_type = GPIO_header_26; break;
	case 0x000010: printf("Model B+ Rev 1.0 with 40 pin GPIO header detected\n"); break;
	case 0x000014:
	case 0x000011: printf("Compute Module is not supported\n"); exit(EXIT_FAILURE);
	case 0x000015:
	case 0x000012: printf("Model A+ Rev 1.1 with 40 pin GPIO header detected\n"); break;
	case 0x000013: printf("Model B+ Rev 1.2 with 40 pin GPIO header detected\n"); break;
	case 0xa01040: printf("Pi 2 Model B Rev 1.0 with 40 pin GPIO header detected\n"); break;
	case 0xa01041:
	case 0xa21041: printf("Pi 2 Model B Rev 1.1 with 40 pin GPIO header detected\n"); break;
	case 0x900092: printf("Pi Zero Rev 1.2 with 40 pin GPIO header detected\n"); break;
	case 0x900093: printf("Pi Zero Rev 1.3 with 40 pin GPIO header detected\n"); break;
	case 0xa22082:
	case 0xa02082: printf("Pi 3 Model B Rev 1.2 with 40 pin GPIO header detected\n"); break;
	default: printf("Unknown Raspberry Pi - assuming 40 pin GPIO header\n");
	}
	return header_type;
}


void set_rts_cts(int enable) {
	int gfpsel, gpiomask;
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "can't open /dev/mem (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	void *gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);
	close(fd);
	if (gpio_map == MAP_FAILED) {
		fprintf(stderr, "mmap error (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	volatile unsigned *gpio = (volatile unsigned *)gpio_map;

	if (rpi_gpio_header_type() == GPIO_header_40) { /* newer 40 pin GPIO header */
		gfpsel = GFPSEL1;
		gpiomask = GPIO1617mask;
		printf("Enabling CTS0 and RTS0 on GPIOs 16 and 17\n");
	}
	else { /* 26 pin GPIO header */
		gfpsel = GFPSEL3;
		gpiomask = GPIO3031mask;
		printf("Enabling CTS0 and RTS0 on GPIOs 30 and 31\n");
	}

	enable ? (gpio[gfpsel] |= gpiomask) : (gpio[gfpsel] &= ~gpiomask);
}

void print_usage(void) {
	printf( \
	"Version: " VERSION "\n" \
	"Usage: rpirtscts on|off\n" \
	"Enable or disable hardware flow control pins on ttyAMA0.\n" \
	"\nFor 26 pin GPIO header boards:\n"    \
	"P5 header pins remap as follows:\n"	\
	"    P5-05 (GPIO30) -> CTS (input)\n" \
	"    P5-06 (GPIO31) -> RTS (output)\n" \
	"\nFor 40 pin GPIO header boards:\n"    \
	"    P1-36 (GPIO16) -> CTS (input)\n" \
	"    P1-11 (GPIO17) -> RTS (output)\n" \
	"\nYou may also need to enable flow control in the driver:\n" \
	"    stty -F /dev/ttyAMA0 crtscts\n" \
	);
}

int main(int argc, char **argv) {

	if (argc != 2) {
		print_usage();
	} else {
		int  enable = strcmp(argv[1],  "on") == 0;
		int disable = strcmp(argv[1], "off") == 0;
		enable || disable ? set_rts_cts(enable) : print_usage();
	}
	return EXIT_SUCCESS;
}
