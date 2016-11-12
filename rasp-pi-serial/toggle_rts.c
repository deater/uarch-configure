/* Code to toggle RTS pin on serial port.

	_____________
	\ 1 2 3 4 5 /
	 \ 6 7 8 9 /
	  ---------

	5 is ground and 7 is RTS

	When toggled the pin will go to -9V to 9V and back
	(should be -12V but this is a pl2303 USB serial port I am using

Code based roughly on a reference found here by HKo:
	http://www.linuxquestions.org/questions/programming-9/manually-controlling-rts-cts-326590/
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
		status |= TIOCM_RTS;
	}
	else {
		/* clear RTS value (makes it +9V) */
		status &= ~TIOCM_RTS;
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

	set_RTS(fd, 0);
//	sleep(1);
	usleep(length);       /* pause 1 second */
	set_RTS(fd, 1);
//	sleep(1);
//}

	/* Restore termio settings */
	tcsetattr(fd, TCSANOW, &oldterminfo);

	/* Close the port */
	close(fd);

	return 0;
}

