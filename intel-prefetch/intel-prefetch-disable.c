/* Disable the hardware prefetcher on:					   */
/*	Core2					  			   */
/*	Nehalem, Westmere, SandyBridge, IvyBridge, Haswell and Broadwell   */
/* See: https://software.intel.com/en-us/articles/disclosure-of-hw-prefetcher-control-on-some-intel-processors */
/*									   */
/* The key is MSR 0x1a4							   */
/* bit 0: L2 HW prefetcher						   */
/* bit 1: L2 adjacent line prefetcher					   */
/* bit 2: DCU (L1 Data Cache) next line prefetcher			   */
/* bit 3: DCU IP prefetcher (L1 Data Cache prefetch based on insn address) */
/*									   */
/* This code uses the /dev/msr interface, and you'll need to be root.      */
/*									   */
/* by Vince Weaver, vincent.weaver _at_ maine.edu -- 26 February 2016	   */

#define CORE2_PREFETCH_MSR	0x1a0
#define NHM_PREFETCH_MSR	0x1a4


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <sys/syscall.h>

static int open_msr(int core) {

	char msr_filename[BUFSIZ];
	int fd;

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDWR);
	if ( fd < 0 ) {
		if ( errno == ENXIO ) {
			fprintf(stderr, "rdmsr: No CPU %d\n", core);
			exit(2);
		} else if ( errno == EIO ) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n", core);
			exit(3);
		} else {
			perror("rdmsr:open");
			fprintf(stderr,"Trying to open %s\n",msr_filename);
			exit(127);
		}
	}

	return fd;
}

static long long read_msr(int fd, int which) {

	uint64_t data;

	if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
		perror("rdmsr:pread");
		exit(127);
	}

	return (long long)data;
}

static int write_msr(int fd, uint64_t which, uint64_t value) {

	if ( pwrite(fd, &value, sizeof(value), which) != sizeof value ) {
		perror("rdmsr:pwrite");
		exit(127);
	}

	return 0;
}

/* FIXME: should really error out if not an Intel CPU */
static int detect_cpu(void) {

	FILE *fff;

	int family,model=-1;
	char buffer[BUFSIZ],*result;
	char vendor[BUFSIZ];
	int is_core2=-1;

	fff=fopen("/proc/cpuinfo","r");
	if (fff==NULL) return -1;

	while(1) {
		result=fgets(buffer,BUFSIZ,fff);
		if (result==NULL) break;

		if (!strncmp(result,"vendor_id",8)) {
			sscanf(result,"%*s%*s%s",vendor);

			if (strncmp(vendor,"GenuineIntel",12)) {
				printf("%s not an Intel chip\n",vendor);
				return -1;
			}
		}

		if (!strncmp(result,"cpu family",10)) {
			sscanf(result,"%*s%*s%*s%d",&family);
			if (family!=6) {
				printf("Wrong CPU family %d\n",family);
				return -1;
			}
		}

		if (!strncmp(result,"model",5)) {
			sscanf(result,"%*s%*s%d",&model);
		}

	}

	fclose(fff);

	switch(model) {
		case 26: case 30: case 31: /* nhm */
			printf("Found Nehalem CPU\n");
			is_core2=0;
			break;

		case 46: /* nhm-ex */
			printf("Found Nehalem-EX CPU\n");
			is_core2=0;
			break;

		case 37: case 44: /* wsm */
			printf("Found Westmere CPU\n");
			is_core2=0;
			break;

		case 47: /* wsm-ex */
			printf("Found Westmere-EX CPU\n");
			is_core2=0;
			break;

		case 42: /* snb */
			printf("Found Sandybridge CPU\n");
			is_core2=0;
			break;

		case 45: /* snb-ep */
			printf("Found Sandybridge-EP CPU\n");
			is_core2=0;
			break;

		case 58: /* ivb */
			printf("Found Ivybridge CPU\n");
			is_core2=0;
			break;


		case 62: /* ivb-ep */
			printf("Found Ivybridge-EP CPU\n");
			is_core2=0;
			break;

		case 60: case 69: case 70: /* hsw */
			printf("Found Haswell CPU\n");
			is_core2=0;
			break;

		case 63: /* hsw-ep */
			printf("Found Haswell-EP CPU\n");
			is_core2=0;
			break;

		case 61: case 71: /* bdw */
			printf("Found Broadwell CPU\n");
			is_core2=0;
			break;

		case 86: case 79: /* bdw-? */
			printf("Found Broadwell-?? CPU\n");
			is_core2=0;
			break;

		case 15: case 22:
		case 23: case 29: /* core2 */
			printf("Found Core2 CPU\n");
			is_core2=1;
			break;
			break;
		default:
			printf("Unsupported model %d\n",model);
			is_core2=-1;
			break;
	}

	return is_core2;
}


/* Disable prefetch on nehalem and newer */
static int disable_prefetch_nhm(int core) {

	int fd;
	int result;
	int begin,end,c;

	printf("Disable all prefetch\n");

	if (core==-1) {
		begin=0;
		end=1024;
	}
	else {
		begin=core;
		end=core;
	}

	for(c=begin;c<=end;c++) {

		fd=open_msr(c);
		if (fd<0) break;

		/* Read original results */
		result=read_msr(fd,NHM_PREFETCH_MSR);

		printf("\tCore %d old : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&0x1?'N':'Y',
			result&0x2?'N':'Y',
			result&0x4?'N':'Y',
			result&0x8?'N':'Y'
			);

		/* Disable all */
		result=write_msr(fd,NHM_PREFETCH_MSR,0xf);

		/* Verify change */
		result=read_msr(fd,NHM_PREFETCH_MSR);

		printf("\tCore %d new : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&0x1?'N':'Y',
			result&0x2?'N':'Y',
			result&0x4?'N':'Y',
			result&0x8?'N':'Y'
			);

		close(fd);

	}

	return 0;
}

/* Enable prefetch on nehalem and newer */
static int enable_prefetch_nhm(int core) {

	int fd;
	int result;
	int begin,end,c;

	printf("Enable all prefetch\n");

	if (core==-1) {
		begin=0;
		end=1024;
	}
	else {
		begin=core;
		end=core;
	}

	for(c=begin;c<=end;c++) {

		fd=open_msr(c);
		if (fd<0) break;

		/* Read original results */
		result=read_msr(fd,NHM_PREFETCH_MSR);

		printf("\tCore %d old : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&0x1?'N':'Y',
			result&0x2?'N':'Y',
			result&0x4?'N':'Y',
			result&0x8?'N':'Y'
			);

		/* Enable all */
		result=write_msr(fd,NHM_PREFETCH_MSR,0x0);

		/* Verify change */
		result=read_msr(fd,NHM_PREFETCH_MSR);

		printf("\tCore %d new : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&0x1?'N':'Y',
			result&0x2?'N':'Y',
			result&0x4?'N':'Y',
			result&0x8?'N':'Y'
			);

		close(fd);

	}

	return 0;
}



/* Disable prefetch on core2 */
static int disable_prefetch_core2(int core) {

	int fd;
	uint64_t result;
	int begin,end,c;

	printf("Disable all prefetch\n");

	if (core==-1) {
		begin=0;
		end=1024;
	}
	else {
		begin=core;
		end=core;
	}

	for(c=begin;c<=end;c++) {

		fd=open_msr(c);
		if (fd<0) break;

		/* Read original results */
		result=read_msr(fd,CORE2_PREFETCH_MSR);

		printf("\tCore %d old : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&(1ULL<<9)?'N':'Y',
			result&(1ULL<<19)?'N':'Y',
			result&(1ULL<<37)?'N':'Y',
			result&(1ULL<<39)?'N':'Y'
			);

		/* Disable all */
		result|=(1ULL<<9)|(1ULL<<19)|(1ULL<<37)|(1ULL<<39);
		result=write_msr(fd,CORE2_PREFETCH_MSR,result);

		/* Verify change */
		result=read_msr(fd,CORE2_PREFETCH_MSR);

		printf("\tCore %d new : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&(1ULL<<9)?'N':'Y',
			result&(1ULL<<19)?'N':'Y',
			result&(1ULL<<37)?'N':'Y',
			result&(1ULL<<39)?'N':'Y'
			);

		close(fd);

	}

	return 0;
}

/* Enable prefetch on core2 */
static int enable_prefetch_core2(int core) {

	int fd;
	uint64_t result;
	int begin,end,c;

	printf("Enable all prefetch\n");

	if (core==-1) {
		begin=0;
		end=1024;
	}
	else {
		begin=core;
		end=core;
	}

	for(c=begin;c<=end;c++) {

		fd=open_msr(c);
		if (fd<0) break;

		/* Read original results */
		result=read_msr(fd,CORE2_PREFETCH_MSR);

		printf("\tCore %d old : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&(1ULL<<9)?'N':'Y',
			result&(1ULL<<19)?'N':'Y',
			result&(1ULL<<37)?'N':'Y',
			result&(1ULL<<39)?'N':'Y'
			);

		/* Enable all */
		result &= ~((1ULL<<9)|(1ULL<<19)|(1ULL<<37)|(1ULL<<39));
		result=write_msr(fd,CORE2_PREFETCH_MSR,result);

		/* Verify change */
		result=read_msr(fd,CORE2_PREFETCH_MSR);

		printf("\tCore %d new : L2HW=%c L2ADJ=%c DCU=%c DCUIP=%c\n",
			c,
			result&(1ULL<<9)?'N':'Y',
			result&(1ULL<<19)?'N':'Y',
			result&(1ULL<<37)?'N':'Y',
			result&(1ULL<<39)?'N':'Y'
			);

		close(fd);

	}

	return 0;
}


int main(int argc, char **argv) {

	int c;
	int core=-1;
	int result=-1;
	int disable=1;
	int core2=0;

	printf("\n");

	opterr=0;

	while ((c = getopt (argc, argv, "c:deh")) != -1) {
		switch (c) {
		case 'c':
			core = atoi(optarg);
			break;
		case 'd':
			disable=1;
			break;
		case 'e':
			disable=0;
			break;

		case 'h':
			printf("Usage: %s [-c core] [-d] [-e] [-h]\n\n",argv[0]);
			exit(0);
		default:
			exit(-1);
		}
	}

	core2=detect_cpu();
	if (core2<0) {
		printf("Unsupported CPU type\n");
		return -1;
	}

	if (disable) {
		if (core2) {
			result=disable_prefetch_core2(core);
		}
		else {
			result=disable_prefetch_nhm(core);
		}
	}
	else {
		if (core2) {
			result=enable_prefetch_core2(core);
		}
		else {
			result=enable_prefetch_nhm(core);
		}
	}

	if (result<0) {

		printf("Unable to access prefetch MSR.\n");
		printf("* Verify you have an Intel Nehalem or newer processor\n");
		printf("* You will probably need to run as root\n");
		printf("* Make sure the msr module is installed\n");
		printf("\n");

		return -1;

	}

	return 0;
}
