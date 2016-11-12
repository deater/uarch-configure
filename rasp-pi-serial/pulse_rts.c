/* Code to toggle RTS pin on Raspberry Pi

	On Pi	TX is GPIO14
		RX is GPIO15
	You need to configure ALT3 for GPIO16/17
		CTS is GPIO16
		RTS is GPIO17

	On Pi3 the amba-pl011 adapter (/dev/ttyAMA) is used by
	bluetooth so they re-rout the stripped down
	mini-uart to those pins.
		Run ./rpirtscts to enable this

	To disable this try putting
		pi3-miniuart-bt
	In /boot/config.txt

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <termios.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int set_RTS(int fd, int value) {

	int status;
	int result;

	/* Get current status */
	result=ioctl(fd, TIOCMGET, &status);
	if (result<0) {
		fprintf(stderr,"Error TIOCMGET: %s\n",
			strerror(errno));
		return -1;
	}

	if (value) {
		/* Set RTS value (makes it -9V) */
		status |= TIOCM_DTR;
	}
	else {
		/* clear RTS value (makes it +9V) */
		status &= ~TIOCM_DTR;
	}

	/* Write out new status */
	result=ioctl(fd, TIOCMSET, &status);
	if (result<0) {
		fprintf(stderr,"Error TIOCMSET %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}


int main(int argc, char **argv) {

	int fd;
	char serial_name[BUFSIZ];
	struct termios attr;
	struct termios oldterminfo;
	int result;
	int length=1000000;

	/* Get serial port name */
	if (argc>1) {
		strncpy(serial_name,argv[1],BUFSIZ);
	}
	else {
		strcpy(serial_name,"/dev/ttyAMA0");
	}

	if (argc>2) {
		length=atoi(argv[2]);
	}



	/* Open serial port */
	fd = open(serial_name, O_RDWR);
	if (fd<0) {
		fprintf(stderr,"Error opening %s: %s\n",
			serial_name,strerror(errno));
		return -1;
	}

	/* Save existing terminfo */
	result = tcgetattr(fd, &oldterminfo);
	if (result<0) {
		fprintf(stderr,"Error tcgetattr(): %s\n",
			strerror(errno));
		return -1;
	}

	/* Set to use hardware flow control */
	attr = oldterminfo;
	attr.c_cflag |= CRTSCTS | CLOCAL;
	attr.c_oflag = 0;

	/* Flush */
	result = tcflush(fd, TCIOFLUSH);
	if (result<0) {
		fprintf(stderr,"ERROR tcflush() :%s\n",
			strerror(errno));
		return -1;
	}

	/* Push out change immediately */
	result=tcsetattr(fd, TCSANOW, &attr);
	if (result<0) {
		fprintf(stderr,"ERROR: tcsetattr() :%s\n",
			strerror(errno));
		return -1;
	}

	while(1) {
		set_RTS(fd, 0);
		sleep(1);
		set_RTS(fd, 1);
		sleep(1);
	}

	/* Restore termio settings */
	tcsetattr(fd, TCSANOW, &oldterminfo);

	/* Close the port */
	close(fd);

	return 0;
}

