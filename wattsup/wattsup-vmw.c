/*
 *	wattsup-vmw - Program for controlling the Watts Up? Pro Device
 *
 *	Based on code example by Patrick Mochel
 *
 * All we want to do is print time followed by Watts, once per second
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>

#include <sys/stat.h>

static const char *wu_version = "0.3-vmw";


#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static int wu_count = 0;
static int wu_debug = 0;
static char *wu_delim = ", ";
static int wu_final = 0;
static int wu_interval = 1;
static int wu_label = 0;
static int wu_newline = 0;
static int wu_suppress = 0;

static int wu_localtime = 0;
static int wu_gmtime = 0;

static int wu_info_all = 0;
static int wu_no_data = 0;
static int wu_set_only = 0;

#define wu_strlen	256
#define wu_num_fields	18
#define wu_param_len	16

struct wu_packet {
	unsigned int	cmd;
	unsigned int	sub_cmd;
	unsigned int	count;
	char	buf[wu_strlen];
	int	len;
	char	* field[wu_num_fields];
	char	* label[wu_num_fields];
};


struct wu_data {
	unsigned int	watts;
	unsigned int	volts;
	unsigned int	amps;
	unsigned int	watt_hours;

	unsigned int	cost;
	unsigned int	mo_kWh;
	unsigned int	mo_cost;
	unsigned int	max_watts;

	unsigned int	max_volts;
	unsigned int	max_amps;
	unsigned int	min_watts;
	unsigned int	min_volts;

	unsigned int	min_amps;
	unsigned int	power_factor;
	unsigned int	duty_cycle;
	unsigned int	power_cycle;
};

struct wu_options {
	char	* longopt;
	int	shortopt;
	int	param;
	int	flag;
	char	* value;

	char	* descr;
	char	* option;
	char	* format;

	int	(*show)(int dev_fd);
	int	(*store)(int dev_fd);
};

enum {
	wu_option_help = 0,
	wu_option_version,

	wu_option_debug,

	wu_option_count,
	wu_option_final,

	wu_option_delim,
	wu_option_newline,
	wu_option_localtime,
	wu_option_gmtime,
	wu_option_label,

	wu_option_suppress,

	wu_option_cal,
	wu_option_header,

	wu_option_interval,
	wu_option_mode,
	wu_option_user,

	wu_option_info_all,
	wu_option_no_data,
	wu_option_set_only,
};


static char * wu_option_value(unsigned int index);


enum {
	wu_field_watts		= 0,
	wu_field_volts,
	wu_field_amps,

	wu_field_watt_hours,
	wu_field_cost,
	wu_field_mo_kwh,
	wu_field_mo_cost,

	wu_field_max_watts,
	wu_field_max_volts,
	wu_field_max_amps,

	wu_field_min_watts,
	wu_field_min_volts,
	wu_field_min_amps,

	wu_field_power_factor,
	wu_field_duty_cycle,
	wu_field_power_cycle,
};

struct wu_field {
	unsigned int	enable;
	char		* name;
	char		* descr;
};

static struct wu_field wu_fields[wu_num_fields] = {
	[wu_field_watts]	= {
		.name	= "watts",
		.descr	= "Watt Consumption",
	},

	[wu_field_min_watts]	= {
		.name	= "min-watts",
		.descr	= "Minimum Watts Consumed",
	},

	[wu_field_max_watts]	= {
		.name	= "max-watts",
		.descr	= "Maxium Watts Consumed",
	},

	[wu_field_volts]	= {
		.name	= "volts",
		.descr	= "Volts Consumption",
	},

	[wu_field_min_volts]	= {
		.name	= "max-volts",
		.descr	= "Minimum Volts Consumed",
	},

	[wu_field_max_volts]	= {
		.name	= "min-volts",
		.descr	= "Maximum Volts Consumed",
	},

	[wu_field_amps]		= {
		.name	= "amps",
		.descr	= "Amp Consumption",
	},

	[wu_field_min_amps]	= {
		.name	= "min-amps",
		.descr	= "Minimum Amps Consumed",
	},

	[wu_field_max_amps]	= {
		.name	= "max-amps",
		.descr	= "Maximum Amps Consumed",
	},

	[wu_field_watt_hours]	= {
		.name	= "kwh",
		.descr	= "Average KWH",
	},

	[wu_field_mo_kwh]	= {
		.name	= "mo-kwh",
		.descr	= "Average monthly KWH",
	},

	[wu_field_cost]		= {
		.name	= "cost",
		.descr	= "Cost per watt",
	},

	[wu_field_mo_cost]	= {
		.name	= "mo-cost",
		.descr	= "Monthly Cost",
	},

	[wu_field_power_factor]	= {
		.name	= "power-factor",
		.descr	= "Ratio of Watts vs. Volt Amps",
	},

	[wu_field_duty_cycle]	= {
		.name	= "duty-cycle",
		.descr	= "Percent of the Time On vs. Time Off",
	},

	[wu_field_power_cycle]	= {
		.name	= "power-cycle",
		.descr	= "Indication of power cycle",
	},

};



static void msg_start(const char * fmt, ...)
{
	va_list(ap);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static void msg_end(void)
{
	printf("\n");
}

static void msg(const char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static void dbg(const char * fmt, ...)
{
	va_list ap;

	if (wu_debug) {
		va_start(ap, fmt);
		msg_start("wattsup: [debug] ");
		vprintf(fmt, ap);
		msg_end();
		va_end(ap);
	}
}

static void err(const char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "wattsup: [error] ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static void perr(const char * fmt, ...)
{
	char buf[1024];
	int n;
	va_list ap;

	va_start(ap, fmt);
	n = sprintf(buf, "wattsup: [error] ");
	vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
	perror(buf);
	va_end(ap);
}

static int ret_err(int err)
{
	errno = err;
	return -1;
}


static void print_packet(struct wu_packet * p, char * str)
{
	int i;

	if (!wu_suppress)
		msg_start("Watts Up? %s\n", str);
	for (i = 0; i < p->count; i++) {
		if (i)
			msg("%s", wu_newline ? "\n" : wu_delim);
		if (wu_label)
			msg("[%s] ", p->label[i]);
		msg(p->field[i]);
	}
	msg_end();
}


static void print_time(void)
{
	time_t t;
	struct tm * tm;

	if (wu_localtime || wu_gmtime) {
		time(&t);

		if (wu_localtime)
			tm = localtime(&t);
		else
			tm = gmtime(&t);

		msg("[%02d:%02d:%02d] ", 
		    tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
}

static void print_packet_filter(struct wu_packet * p, 
				int (*filter_ok)(struct wu_packet * p, int i, char * str))
{
	char buf[256];
	int printed;
	int i;

	print_time();
	for (i = 0, printed = 0; i < p->count; i++) {
		if (!filter_ok(p, i, buf))
			continue;

		if (printed++)
			msg("%s", wu_newline ? "\n" : wu_delim);
		if (wu_label)
			msg("[%s] ", p->label[i]);
		msg(buf);
	}
	msg_end();
}

int debug=1;


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

	ret = open(full_device_name, O_RDWR | O_NONBLOCK);
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


static int wu_write(int fd, struct wu_packet * p)
{
	int ret;
	int n;
	int i;
	char * s = p->buf;

	memset(p->buf, 0, sizeof(p->buf));
	n = sprintf(p->buf, "#%c,%c,%d", p->cmd, p->sub_cmd, p->count);
	p->len = n;
	s = p->buf + n;

	for (i = 0; i < p->count; i++) {
		if ((p->len + strlen(p->field[i]) + 4) >= sizeof(p->buf)) {
			err("Overflowed command string");
			return ret_err(EOVERFLOW);
		}
		n = sprintf(s, ",%s", p->field[i]);
		s += n;
		p->len += n;
	}
	p->buf[p->len++] = ';';

	dbg("Writing '%s' (strlen = %d) (len = %d) to device", 
	    p->buf, strlen(p->buf), p->len);
	ret = write(fd, p->buf, p->len);
	if (ret != p->len)
		perr("Writing to device");
	
	return ret >= 0 ? 0 : ret;
}


static void dump_packet(struct wu_packet * p)
{
	int i;

	dbg("Packet - Command '%c' %d parameters", p->cmd, p->count);
	
	for (i = 0; i < p->count; i++) 
		dbg("[%2d] [%20s] = \"%s\"", i, p->label[i], p->field[i]);
}


static int parse_packet(struct wu_packet * p)
{
	char * s, *next;
	int i;

	p->buf[p->len] = '\0';

	dbg("Parsing Packet, Raw buffer is (%d bytes) [%s]", 
	    p->len, p->buf);

	s = p->buf;

	/*
	 * First character should be '#'
	 */
	if (s) {
		s = strchr(s, '#');
		if (s)
			s++;
		else {
			dbg("Invalid packet");
			return ret_err(EFAULT);
		}
	} else {
		dbg("Invalid packet");
		return ret_err(EFAULT);
	}

	/*
	 * Command character is first
	 */
	next = strchr(s, ',');
	if (next) {
		p->cmd = *s;
		s = ++next;
	} else {
		dbg("Invalid Command field [%s]", s);
		return ret_err(EFAULT);
	}

	/*
	 * Next character is the subcommand, and should be '-'
	 * Though, it doesn't matter, because we just 
	 * discard it anyway.
	 */
	next = strchr(s, ',');
	if (next) {
		p->sub_cmd = *s;
		s = ++next;
	} else {
		dbg("Invalid 2nd field");
		return ret_err(EFAULT);
	}

	/*
	 * Next is the number of parameters, 
	 * which should always be > 0.
	 */
	next = strchr(s, ',');
	if (next) {
		*next++ = '\0';
		p->count = atoi(s);
		s = next;
	} else {
		dbg("Couldn't determine number of parameters");
		return ret_err(EFAULT);
	}
	
	dbg("Have %d parameter%s (cmd = '%c')", 
	    p->count, p->count > 1 ? "s" : "", p->cmd);

	/*
	 * Now, we loop over the rest of the string,
	 * storing a pointer to each in p->field[].
	 *
	 * The last character was originally a ';', but may have been
	 * overwritten with a '\0', so we make sure to catch
	 * that when converting the last parameter. 
	 */
	for (i = 0; i < p->count; i++) {
		next = strpbrk(s, ",;");
		if (next) {
			*next++ = '\0';
		} else {
			if (i < (p->count - 1)) {
				dbg("Malformed parameter string [%s]", s);
				return ret_err(EFAULT);
			}
		}

		/*
		 * Skip leading white space in fields
		 */
		while (isspace(*s))
			s++;
		p->field[i] = s;
		s = next;
	}
	dump_packet(p);
	return 0;
}


static int wu_read(int fd, struct wu_packet * p)
{
	fd_set read_fd;
	struct timeval tv;
	int ret;

	FD_ZERO(&read_fd);
	FD_SET(fd, &read_fd);
	
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	ret = select(fd + 1, &read_fd, NULL, NULL, &tv);
	if (ret < 0) {
		perr("select on terminal device");
		return ret;
	} else if (ret > 0) {

		ret = read(fd, p->buf, wu_strlen);
		if (ret < 0) {
			perr("Reading from device");
			return ret;
		}
		p->len = ret;
	} else {
		dbg("Device timed out while reading");
		return ret_err(ETIME);
	}
	return parse_packet(p);
}


static int wu_show_header(int fd)
{
	struct wu_packet p = {
		.cmd		= 'H',
		.sub_cmd	= 'R',
		.count		= 0,
		.label = {
			[0] = "watts header",
			[1] = "volts header",
			[2] = "amps header",
			[3] = "kWh header",
			[4] = "cost header",
			[5] = "mo. kWh header",
			[6] = "mo. cost header",
			[7] = "max watts header",
			[8] = "max volts header",
			[9] = "max amps header",
			[10] = "min watts header",
			[11] = "min volts header",
			[12] = "min amps header",
			[13] = "power factor header",
			[14] = "duty cycle header",
			[15] = "power cycle header",
		}
	};
	int ret;

	ret = wu_write(fd, &p);
	if (ret) {
		perr("Requesting header strings");
		return ret;
	}
	sleep(1);

	ret = wu_read(fd, &p);
	if (ret) {
		perr("Reading header strings");
		return ret;
	}

	print_packet(&p, "Header Record");

	return 0;
}


static int wu_show_cal(int fd)
{
	struct wu_packet p = {
		.cmd		= 'F',
		.sub_cmd	= 'R',
		.count		= 0,
		.label = {
			[0] = "flags",
			[1] = "sample count",
			[2] = "volts gain",
			[3] = "volts bias",
			[4] = "amps gain",
			[5] = "amps bias",
			[6] = "amps offset",
			[7] = "low amps gain",
			[8] = "low amps bias",
			[9] = "low amps offset",
			[10] = "watts gain",
			[11] = "watts offset",
			[12] = "low watts gain",
			[13] = "low watts offset",
		},
	};
	int ret;

	ret = wu_write(fd, &p);
	if (ret) {
		perr("Requesting calibration parameters");
		return ret;
	}
	sleep(1);

	ret = wu_read(fd, &p);
	if (ret) {
		perr("Reading header strings");
		return ret;
	}
	print_packet(&p, "Calibration Settings");

	return 0;
}

static int wu_start_log(int wu_fd) {

	struct wu_packet p = {
		.cmd		= 'L',
		.sub_cmd	= 'W',
		.count		= 3,
		.field		= {
			[0] = "E",
			[1] = "1",
			[2] = "1",
		},
	};
	int ret;

	/*
	 * Start up logging
	 */
	ret = wu_write(wu_fd, &p);
	if (!ret)
		sleep(1);
	else {
		perr("Starting External Logging");
		return ret;
	}
	return ret;
}

static int wu_stop_log(int fd)
{
	struct wu_packet p = {
		.cmd		= 'L',
		.sub_cmd	= 'R',
		.count		= 0,
		.label = {
			[0] = "time stamp",
			[1] = "interval",
		},
	};
	int ret;

	/*
	 * Stop logging and read time stamp.
	 */
	ret = wu_write(fd, &p);
	if (ret) {
		perr("Stopping External Logging");
		return ret;
	}
	sleep(1);

	ret = wu_read(fd, &p);
	if (ret) {
		perr("Reading final time stamp");
		return ret;
	}
	if (wu_final)
		print_packet(&p, "Final Time Stamp and Interval");
	return ret;
}

static int filter_data(struct wu_packet * p, int i, char * buf)
{
	if (i < wu_num_fields) {
		if (wu_fields[i].enable) {
			double val = strtod(p->field[i], NULL);
			snprintf(buf, 256, "%.1f", val / 10.0);
			return 1;
		}
	}
	return 0;
}

static int wu_clear(int fd) {

   int ret;

   char abort[]={0x18,0x0};
   char hard_reset_packet[]="#V,W,0;";
   char reset_packet[]="#R,W,0;";
   char read_packet[BUFSIZ];

   /* Clear the memory */
   ret = write(fd, abort,strlen(abort));
   if (ret!=strlen(abort)) {
      fprintf(stderr,"Error Clearing memory!\n");
   } else {
     sleep(1);
   }	

   /* Clear the memory */
   ret = write(fd, hard_reset_packet,strlen(hard_reset_packet));
   if (ret!=strlen(hard_reset_packet)) {
      fprintf(stderr,"Error Clearing memory!\n");
   } else {
     sleep(1);
   }	

   while(1) {

   /* Clear the memory */
   ret = write(fd, reset_packet,strlen(reset_packet));
   if (ret!=strlen(reset_packet)) {
      fprintf(stderr,"Error Clearing memory!\n");
   } else {
     sleep(1);
   }	

   /* Dummy read */

   read(fd, read_packet,BUFSIZ);
   sleep(1);
   }
   return ret;

}

static int wu_read_data(int fd) {

	struct wu_packet p = {
		.label = {
			[0] = "watts",
			[1] = "volts",
			[2] = "amps",
			[3] = "watt hours",
			[4] = "cost",
			[5] = "mo. kWh",
			[6] = "mo. cost",
			[7] = "max watts",
			[8] = "max volts",
			[9] = "max amps",
			[10] = "min watts",
			[11] = "min volts",
			[12] = "min amps",
			[13] = "power factor",
			[14] = "duty cycle",
			[15] = "power cycle",
		        [16] = "Hertz",
		        [17] = "VA",
		},
	};
	int num_read = 0;
	int retry = 0;
	int ret;
	int i;

	static const int wu_max_retry = 2;

	i = 0;
	while (1) {
		
		ret = wu_read(fd, &p);
		if (ret) {
			if (++retry < wu_max_retry) {
				dbg("Bad record back, retrying\n");
				sleep(wu_interval);
				continue;
			} else if (retry == wu_max_retry) {
				dbg("Still couldn't get a good record, resetting\n");
				wu_stop_log(fd);
				wu_clear(fd);
				wu_start_log(fd);
				num_read = 0;
				sleep(wu_interval);
				continue;
			}
			perr("Blech. Giving up on read");
			break;
		} else if (retry)
			retry = 0;

		dbg("[%d] ", num_read);
		num_read++;
		print_packet_filter(&p, filter_data);

		if (wu_count && (++i == wu_count)) 
			break;
		
		sleep(wu_interval);
	}
	return 0;

}


static int wu_show_interval(int fd)
{
	struct wu_packet p = {
		.cmd		= 'S',
		.sub_cmd	= 'R',
		.count		= 0,
		.label = {
			[0] = "reserved",
			[1] = "interval",
		},
	};
	int ret;
	
	ret = wu_write(fd, &p);
	if (ret) {
		perr("Requesting interval");
		return ret;
	}
	sleep(1);

	ret = wu_read(fd, &p);
	if (ret) {
		perr("Reading interval");
		return ret;
	}
	print_packet(&p, "Interval Settings");

	return 0;
}

static int wu_write_interval(int fd, unsigned int seconds, 
			     unsigned int interval)
{
	char str_seconds[wu_param_len];
	char str_interval[wu_param_len];
	struct wu_packet p = {
		.cmd		= 'S',
		.sub_cmd	= 'W',
		.count		= 2,
		.field		= {
			[0] = str_seconds,
			[1] = str_interval,
		},
	};
	int ret;

	snprintf(str_seconds, wu_param_len, "%ud", seconds);
	snprintf(str_interval, wu_param_len, "%ud", interval);

	ret = wu_write(fd, &p);
	if (ret) {
		perr("Setting Sampling Interval");
		return ret;
	}
	sleep(1);
	return 0;
}

static int wu_store_interval(int fd)
{
	char * s = wu_option_value(wu_option_interval);
	char * end;

	wu_interval = strtol(s, &end, 0);
	if (*end) {
		err("Invalid interval: %s", s);
		return ret_err(EINVAL);
	}
	return wu_write_interval(fd, 1, wu_interval);
}

static int wu_show_mode(int fd)
{
	struct wu_packet p = {
		.cmd		= 'M',
		.sub_cmd	= 'R',
		.count		= 0,
		.label		= {
			[0] = "display mode",
		},
	};
	int ret;

	ret = wu_write(fd, &p);
	if (ret) {
		perr("Requesting device display mode");
		return ret;
	}

	ret = wu_read(fd, &p);
	if (ret) {
		perr("Reaing device display mode");
		return ret;
	}
	dump_packet(&p);
	return ret;
}

static int wu_write_mode(int fd, int mode)
{
	char str_mode[wu_param_len];
	struct wu_packet p = {
		.cmd		= 'M',
		.sub_cmd	= 'W',
		.count		= 1,
		.field		= {
			[0] = str_mode,
		},
	};
	int ret;
	
	snprintf(str_mode, wu_param_len, "%ud", mode);
	ret = wu_write(fd, &p);
	if (ret)
		perr("Setting device display mode");
	else
		sleep(1);
	return ret;
}

static int wu_store_mode(int fd)
{
	char * s = wu_option_value(wu_option_mode);
	char * end;
	unsigned int mode;

	mode = strtol(s, &end, 0);
	if (*end) {
		err("Invalid mode: %s", s);
		return ret_err(EINVAL);
	}
	return wu_write_mode(fd, mode);
}



static int wu_show_user(int fd)
{
	struct wu_packet p = {
		.cmd		= 'U',
		.sub_cmd	= 'R',
		.count		= 0,
		.label		= {
			[0] = "cost per kWh",
			[1] = "2nd tier cost",
			[2] = "2nd tier threshold",
			[3] = "duty cycle threshold",
		},
	};
	int ret;

	ret = wu_write(fd, &p);
	if (ret) {
		perr("Requesting user parameters");
		return ret;
	}
	sleep(1);

	ret = wu_read(fd, &p);
	if (ret) {
		perr("Reading user parameters");
		return ret;
	}
	print_packet(&p, "User Settings");
	return 0;
}


static int wu_write_user(int fd, unsigned int kwh_cost, 
			 unsigned int second_tier_cost,
			 unsigned int second_tier_threshold,
			 unsigned int duty_cycle_threshold)
{
	char str_kwh_cost[wu_param_len];
	char str_2nd_tier_cost[wu_param_len];
	char str_2nd_tier_threshold[wu_param_len];
	char str_duty_cycle_threshold[wu_param_len];

	struct wu_packet p = {
		.cmd		= 'U',
		.sub_cmd	= 'R',
		.count		= 0,
		.label		= {
			[0] = str_kwh_cost,
			[1] = str_2nd_tier_cost,
			[2] = str_2nd_tier_threshold,
			[3] = str_duty_cycle_threshold,
		},
	};
	int ret;

	snprintf(str_kwh_cost, wu_param_len, "%ud", kwh_cost);
	snprintf(str_2nd_tier_cost, wu_param_len, "%ud", 
		 second_tier_cost);
	snprintf(str_2nd_tier_threshold, wu_param_len, "%ud", 
		 second_tier_threshold);
	snprintf(str_duty_cycle_threshold, wu_param_len, "%ud", 
		 duty_cycle_threshold);

	ret = wu_write(fd, &p);
	if (ret)
		perr("Writing user parameters");
	else
		sleep(1);
	return ret;
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

#if 0

	/*
	 * Check for options to print device info
	 */
	if (wu_info_all) {
		wu_show(wu_option_cal, wu_fd);
		wu_show(wu_option_header, wu_fd);
		wu_show(wu_option_interval, wu_fd);
		wu_show(wu_option_mode, wu_fd);
		wu_show(wu_option_user, wu_fd);
	} else {
		wu_check_show(wu_option_cal, wu_fd);
		wu_check_show(wu_option_header, wu_fd);

		if (!wu_set_only) {
			wu_check_show(wu_option_interval, wu_fd);
			wu_check_show(wu_option_mode, wu_fd);
			wu_check_show(wu_option_user, wu_fd);
		}
	}
#endif

	if (!wu_no_data) {

		if ((ret = wu_start_log(wu_fd)))
			goto Close;
	    
		wu_read_data(wu_fd);
		
		wu_stop_log(wu_fd);
	}

Close:
	close(wu_fd);
	return 0;
}


