/*
 *	wattsup-simple - Program for controlling the Watts Up? Pro Device
 *
 *	Based on code example by Patrick Mochel
 *
 * All we want to do is print time followed by Watts, once per second
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/time.h>

static int debug=1;


/* start the external logging of power info */
/* #L,W,3,E,<Reserved>,<Interval>; */
static int wu_start_external_log(int wu_fd, int interval) {

	char command[BUFSIZ];
	int ret,length;

	if (debug) fprintf(stderr,"Enabling logging...\n");

	sprintf(command,"#L,W,3,E,1,%d;",interval);
	if (debug) fprintf(stderr,"%s\n",command);

	length=strlen(command);

	ret=write(wu_fd,command,length);
	if (ret!=length) {
		fprintf(stderr,"Error starting logging %s!\n",
			strerror(errno));
		return -1;
	}

	sleep(1);

	return 0;
}


/* Open our device, probably ttyUSB0 */
static int open_device(char *device_name) {

	struct stat s;
	int ret;
	char full_device_name[BUFSIZ];

	sprintf(full_device_name,"/dev/%s",device_name);

	ret = stat(full_device_name, &s);
	if (ret < 0) {
		fprintf(stderr,"Problem statting %s, %s\n",
			full_device_name,strerror(errno));
		return -1;
	}

	if (!S_ISCHR(s.st_mode)) {
		fprintf(stderr,"Error: %s is not a TTY character device.",
			full_device_name);
		return -1;
	}

	ret = access(full_device_name, R_OK | W_OK);
	if (ret) {
		fprintf(stderr,"Error: %s is not writable, %s.",
			full_device_name,strerror(errno));
		return -1;
	}

	/* Not NONBLOCK */
	ret = open(full_device_name, O_RDWR);
	if (ret < 0) {
		fprintf(stderr,"Error! Could not open %s, %s",
			full_device_name, strerror(errno));
		return -1;
	}

	return ret;
}


/* Do the annoying Linux serial setup */
static int setup_serial_device(int fd) {

	struct termios t;
	int ret;

	/* get the current attributes */
	ret = tcgetattr(fd, &t);
	if (ret) {
		fprintf(stderr,"tcgetattr failed, %s\n",strerror(errno));
		return ret;
	}

	/* set terminal to "raw" mode */
	cfmakeraw(&t);

	/* set input speed to 115200 */
	/* (original code did B9600 ??? */
	cfsetispeed(&t, B115200);

	/* set output speed to 115200 */
	cfsetospeed(&t, B115200);

	/* discard any data received but not read */
	tcflush(fd, TCIFLUSH);

	/* 8N1 */
	t.c_cflag &= ~PARENB;
	t.c_cflag &= ~CSTOPB;
	t.c_cflag &= ~CSIZE;
	t.c_cflag |= CS8;

	/* set the new attributes */
	ret = tcsetattr(fd, TCSANOW, &t);

	if (ret) {
		fprintf(stderr,"ERROR: setting terminal attributes, %s\n",
			strerror(errno));
		return ret;
	}

	return 0;
}

/* Read from the meter */
static int wu_read(int fd) {

	int ret=-1;
	int offset=0;

	struct timeval sample_time;
	double double_time=0.0;
	double prev_time=0.0;

#define STRING_SIZE 256
	char string[STRING_SIZE];

	memset(string,0,STRING_SIZE);

	while(ret<0) {
		ret = read(fd, string, STRING_SIZE);
		if ((ret<0) && (ret!=EAGAIN)) {
			fprintf(stderr,"error reading from device %s\n",
				strerror(errno));
		}
	}

	if (string[0]!='#') {
		fprintf(stderr,"Protocol error with string %s\n",string);
		return ret;
	}

	prev_time=double_time;

	gettimeofday(&sample_time,NULL);
	double_time=(double)sample_time.tv_sec+
		((double)sample_time.tv_usec)/1000000.0;

	if ((double_time-prev_time>1.2) && (prev_time!=0.0)) {
		fprintf(stderr,"Error!  More than 1s between times!\n");
	}


	offset=ret;

	/* Typically ends in ;\r\n */
	while(string[offset-1]!='\n') {
//		printf("Offset is %d char %d\n",offset,string[offset-1]);
		ret = read(fd, string+offset, STRING_SIZE-ret);
		offset+=ret;
	}

//	printf("String: %lf %s\n",double_time,string);

	char watts_string[BUFSIZ];
	double watts;
	int i=0,j=0,commas=0;
	while(i<strlen(string)) {
		if (string[i]==',') commas++;
		if (commas==3){
			i++;
			while(string[i]!=',') {
				watts_string[j]=string[i];
				i++;
				j++;
			}
			watts_string[j]=0;
			break;
		}
		i++;
	}

	watts=atof(watts_string);
	watts/=10.0;

	printf("%lf %.1lf\n",double_time,watts);


	return 0;
}


int main(int argc, char ** argv) {

	int ret;
	char device_name[BUFSIZ];
	int wu_fd = 0;


	/* Check command line */
	if (argc>1) {
		strncpy(device_name,argv[1],BUFSIZ);
	}
	else {
		strncpy(device_name,"ttyUSB0",BUFSIZ);
	}

	/* Print help? */
	/* Print version? */

	/*************************/
	/* Open device           */
	/*************************/
	wu_fd = open_device(device_name);
	if (wu_fd<0) {
		return wu_fd;
	}

	if (debug) {
		fprintf(stderr,"DEBUG: %s is opened\n", device_name);
	}

	ret = setup_serial_device(wu_fd);
	if (ret) {
		close(wu_fd);
		return -1;
	}

	/* Enable logging */
	ret = wu_start_external_log(wu_fd,1);
	if (ret) {
		fprintf(stderr,"Error enabling logging\n");
		close(wu_fd);
		return 0;
	}

	/* Read data */
	while (1) {
		ret = wu_read(wu_fd);
		usleep(500000);
	}

	close(wu_fd);
	return 0;
}


