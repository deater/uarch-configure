/* Read the RAPL registers on recent (>sandybridge) Intel processors	*/
/*									*/
/* There are currently three ways to do this:				*/
/*	1. Read the MSRs directly with /dev/cpu/??/msr			*/
/*	2. Use the perf_event_open() interface				*/
/*	3. Read the values from the sysfs powercap interface		*/
/*									*/
/* MSR Code originally based on a (never made it upstream) linux-kernel	*/
/*	RAPL driver by Zhang Rui <rui.zhang@intel.com>			*/
/*	https://lkml.org/lkml/2011/5/26/93				*/
/* Additional contributions by:						*/
/*	Romain Dolbeau -- romain @ dolbeau.org				*/
/*									*/
/* For raw MSR access the /dev/cpu/??/msr driver must be enabled and	*/
/*	permissions set to allow read access.				*/
/*	You might need to "modprobe msr" before it will work.		*/
/*									*/
/* perf_event_open() support requires at least Linux 3.14 and to have	*/
/*	/proc/sys/kernel/perf_event_paranoid < 1			*/
/*									*/
/* the sysfs powercap interface got into the kernel in 			*/
/*	2d281d8196e38dd (3.13)						*/
/*									*/
/* Compile with:   gcc -O2 -Wall -o rapl-read rapl-read.c -lm		*/
/*									*/
/* Vince Weaver -- vincent.weaver @ maine.edu -- 11 September 2015	*/
/*									*/

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
#include <linux/perf_event.h>

#define MSR_RAPL_POWER_UNIT		0x606

/*
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */
/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT	0x610
#define MSR_PKG_ENERGY_STATUS		0x611
#define MSR_PKG_PERF_STATUS		0x613
#define MSR_PKG_POWER_INFO		0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT		0x638
#define MSR_PP0_ENERGY_STATUS		0x639
#define MSR_PP0_POLICY			0x63A
#define MSR_PP0_PERF_STATUS		0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT		0x640
#define MSR_PP1_ENERGY_STATUS		0x641
#define MSR_PP1_POLICY			0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT		0x618
#define MSR_DRAM_ENERGY_STATUS		0x619
#define MSR_DRAM_PERF_STATUS		0x61B
#define MSR_DRAM_POWER_INFO		0x61C

/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET	0
#define POWER_UNIT_MASK		0x0F

#define ENERGY_UNIT_OFFSET	0x08
#define ENERGY_UNIT_MASK	0x1F00

#define TIME_UNIT_OFFSET	0x10
#define TIME_UNIT_MASK		0xF000

static int open_msr(int core) {

	char msr_filename[BUFSIZ];
	int fd;

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDONLY);
	if ( fd < 0 ) {
		if ( errno == ENXIO ) {
			fprintf(stderr, "rdmsr: No CPU %d\n", core);
			exit(2);
		} else if ( errno == EIO ) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
					core);
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

#define CPU_SANDYBRIDGE		42
#define CPU_SANDYBRIDGE_EP	45
#define CPU_IVYBRIDGE		58
#define CPU_IVYBRIDGE_EP	62
#define CPU_HASWELL		60	// 69,70 too?
#define CPU_HASWELL_EP		63
#define CPU_BROADWELL		61	// 71 too?
#define CPU_BROADWELL_EP	79
#define CPU_BROADWELL_DE	86
#define CPU_SKYLAKE		78	// 94 too?


static int detect_cpu(void) {

	FILE *fff;

	int family,model=-1;
	char buffer[BUFSIZ],*result;
	char vendor[BUFSIZ];

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

	printf("Found ");

	switch(model) {
		case CPU_SANDYBRIDGE:
			printf("Sandybridge");
			break;
		case CPU_SANDYBRIDGE_EP:
			printf("Sandybridge-EP");
			break;
		case CPU_IVYBRIDGE:
			printf("Ivybridge");
			break;
		case CPU_IVYBRIDGE_EP:
			printf("Ivybridge-EP");
			break;
		case CPU_HASWELL:
			printf("Haswell");
			break;
		case CPU_HASWELL_EP:
			printf("Haswell-EP");
			break;
		case CPU_BROADWELL:
			printf("Broadwell");
			break;
		default:
			printf("Unsupported model %d\n",model);
			model=-1;
			break;
	}

	printf(" Processor type\n");

	return model;
}

#define MAX_CPUS	1024
#define MAX_PACKAGES	1024

static int total_cores=0,total_packages=0;
static int package_map[MAX_PACKAGES];

static int detect_packages(void) {

	char filename[BUFSIZ];
	FILE *fff;
	int package;
	int i;

	for(i=0;i<MAX_PACKAGES;i++) package_map[i]=-1;

	printf("\t");
	for(i=0;i<MAX_CPUS;i++) {
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fff=fopen(filename,"r");
		if (fff==NULL) break;
		fscanf(fff,"%d",&package);
		printf("%d (%d)",i,package);
		if (i%8==7) printf("\n\t"); else printf(", ");
		fclose(fff);

		if (package_map[package]==-1) {
			total_packages++;
			package_map[package]=i;
		}

	}

	printf("\n");

	total_cores=i;

	printf("\tDetected %d cores in %d packages\n\n",
		total_cores,total_packages);

	return 0;
}


/*******************************/
/* MSR code                    */
/*******************************/
static int rapl_msr(int core, int cpu_model) {

	int fd;
	long long result;
	double power_units,time_units;
	double cpu_energy_units,dram_energy_units;
	double package_before,package_after;
	double pp0_before,pp0_after;
	double pp1_before=0.0,pp1_after;
	double dram_before=0.0,dram_after;
	double thermal_spec_power,minimum_power,maximum_power,time_window;

	if (cpu_model<0) {
		printf("Unsupported CPU type\n");
		return -1;
	}

	printf("Checking core #%d\n",core);

	fd=open_msr(core);

	/* Calculate the units used */
	result=read_msr(fd,MSR_RAPL_POWER_UNIT);

	power_units=pow(0.5,(double)(result&0xf));
	cpu_energy_units=pow(0.5,(double)((result>>8)&0x1f));
	time_units=pow(0.5,(double)((result>>16)&0xf));

	/* On Haswell EP the DRAM units differ from the CPU ones */
	if (cpu_model==CPU_HASWELL_EP) {
		dram_energy_units=pow(0.5,(double)16);
	}
	else {
		dram_energy_units=cpu_energy_units;
	}

	printf("Power units = %.3fW\n",power_units);
	printf("CPU Energy units = %.8fJ\n",cpu_energy_units);
	printf("DRAM Energy units = %.8fJ\n",dram_energy_units);
	printf("Time units = %.8fs\n",time_units);
	printf("\n");

	/* Show package power info */
	result=read_msr(fd,MSR_PKG_POWER_INFO);
	thermal_spec_power=power_units*(double)(result&0x7fff);
	printf("Package thermal spec: %.3fW\n",thermal_spec_power);
	minimum_power=power_units*(double)((result>>16)&0x7fff);
	printf("Package minimum power: %.3fW\n",minimum_power);
	maximum_power=power_units*(double)((result>>32)&0x7fff);
	printf("Package maximum power: %.3fW\n",maximum_power);
	time_window=time_units*(double)((result>>48)&0x7fff);
	printf("Package maximum time window: %.6fs\n",time_window);

	/* Show package power limit */
	result=read_msr(fd,MSR_PKG_RAPL_POWER_LIMIT);
	printf("Package power limits are %s\n", (result >> 63) ? "locked" : "unlocked");
	double pkg_power_limit_1 = power_units*(double)((result>>0)&0x7FFF);
	double pkg_time_window_1 = time_units*(double)((result>>17)&0x007F);
	printf("Package power limit #1: %.3fW for %.6fs (%s, %s)\n",
		pkg_power_limit_1, pkg_time_window_1,
		(result & (1LL<<15)) ? "enabled" : "disabled",
		(result & (1LL<<16)) ? "clamped" : "not_clamped");
	double pkg_power_limit_2 = power_units*(double)((result>>32)&0x7FFF);
	double pkg_time_window_2 = time_units*(double)((result>>49)&0x007F);
	printf("Package power limit #2: %.3fW for %.6fs (%s, %s)\n", 
		pkg_power_limit_2, pkg_time_window_2,
		(result & (1LL<<47)) ? "enabled" : "disabled",
		(result & (1LL<<48)) ? "clamped" : "not_clamped");

	printf("\n");

	/* result=read_msr(fd,MSR_RAPL_POWER_UNIT); */

	result=read_msr(fd,MSR_PKG_ENERGY_STATUS);
	package_before=(double)result*cpu_energy_units;
	printf("Package energy before: %.6fJ\n",package_before);

	/* only available on *Bridge-EP */
	if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP)) {
		result=read_msr(fd,MSR_PKG_PERF_STATUS);
		double acc_pkg_throttled_time=(double)result*time_units;
		printf("Accumulated Package Throttled Time : %.6fs\n",
			acc_pkg_throttled_time);
	}

	result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
	pp0_before=(double)result*cpu_energy_units;
	printf("PowerPlane0 (core) for core %d energy before: %.6fJ\n",
		core,pp0_before);

	result=read_msr(fd,MSR_PP0_POLICY);
	int pp0_policy=(int)result&0x001f;
	printf("PowerPlane0 (core) for core %d policy: %d\n",core,pp0_policy);

	/* only available on *Bridge-EP */
	if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP)) {
		result=read_msr(fd,MSR_PP0_PERF_STATUS);
		double acc_pp0_throttled_time=(double)result*time_units;
		printf("PowerPlane0 (core) Accumulated Throttled Time "
		": %.6fs\n",acc_pp0_throttled_time);
	}

	/* not available on *Bridge-EP */
	if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
		(cpu_model==CPU_HASWELL)) {

 		result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
		pp1_before=(double)result*cpu_energy_units;
		printf("PowerPlane1 (on-core GPU if avail) before: %.6fJ\n",
			pp1_before);
		result=read_msr(fd,MSR_PP1_POLICY);
		int pp1_policy=(int)result&0x001f;
		printf("PowerPlane1 (on-core GPU if avail) %d policy: %d\n",
			core,pp1_policy);
	}


	/* Updated documentation (but not the Vol3B) says Haswell and	*/
	/* Broadwell have DRAM support too				*/
	if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP) ||
		(cpu_model==CPU_HASWELL_EP) ||
		(cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {

		result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
		dram_before=(double)result*dram_energy_units;
		printf("DRAM energy before: %.6fJ\n",dram_before);
	}

  	printf("\nSleeping 1 second\n\n");
	sleep(1);

	result=read_msr(fd,MSR_PKG_ENERGY_STATUS);
	package_after=(double)result*cpu_energy_units;
	printf("Package energy after: %.6f  (%.6fJ consumed)\n",
		package_after,package_after-package_before);

	result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
	pp0_after=(double)result*cpu_energy_units;
	printf("PowerPlane0 (core) for core %d energy after: %.6f  "
		"(%.6fJ consumed)\n",
		core,pp0_after,pp0_after-pp0_before);

	/* not available on SandyBridge-EP */
	if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
		(cpu_model==CPU_HASWELL)) {
		result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
		pp1_after=(double)result*cpu_energy_units;
		printf("PowerPlane1 (on-core GPU if avail) after: %.6f "
			" (%.6fJ consumed)\n",
			pp1_after,pp1_after-pp1_before);
	}

	if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP) ||
		(cpu_model==CPU_HASWELL_EP) ||
		(cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {

		result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
		dram_after=(double)result*dram_energy_units;
		printf("DRAM energy after: %.6f  (%.6fJ consumed)\n",
			dram_after,dram_after-dram_before);
	}

	printf("\n");
	printf("Note: the energy measurements can overflow in 60s or so\n");
	printf("      so try to sample the counters more often than that.\n\n");

	close(fd);

	return 0;
}

static int perf_event_open(struct perf_event_attr *hw_event_uptr,
                    pid_t pid, int cpu, int group_fd, unsigned long flags) {

        return syscall(__NR_perf_event_open,hw_event_uptr, pid, cpu,
                        group_fd, flags);
}

#define NUM_RAPL_DOMAINS	4

char rapl_domain_names[NUM_RAPL_DOMAINS][30]= {
	"energy-cores",
	"energy-gpu",
	"energy-pkg",
	"energy-ram",
};


static int check_paranoid(void) {

	int paranoid_value;
	FILE *fff;

	fff=fopen("/proc/sys/kernel/perf_event_paranoid","r");
	if (fff==NULL) {
		fprintf(stderr,"Error! could not open /proc/sys/kernel/perf_event_paranoid %s\n",
			strerror(errno));

		/* We can't return a negative value as that implies no paranoia */
		return 500;
	}

	fscanf(fff,"%d",&paranoid_value);
	fclose(fff);

	return paranoid_value;

}

static int rapl_perf(int core) {

	FILE *fff;
	int type;
	int config=0;
	char units[BUFSIZ];
	char filename[BUFSIZ];
	int fd[NUM_RAPL_DOMAINS];
	double scale[NUM_RAPL_DOMAINS];
	struct perf_event_attr attr;
	long long value;
	int i;
	int paranoid_value;

	fff=fopen("/sys/bus/event_source/devices/power/type","r");
	if (fff==NULL) {
		printf("No perf_event rapl support found (requires Linux 3.14)\n");
		printf("Falling back to raw msr support\n\n");
		return -1;
	}
	fscanf(fff,"%d",&type);
	fclose(fff);

	printf("Using perf_event to gather RAPL results\n");
	printf("For raw msr results use the -m option\n\n");

	for(i=0;i<NUM_RAPL_DOMAINS;i++) {

		fd[i]=-1;

		sprintf(filename,"/sys/bus/event_source/devices/power/events/%s",
			rapl_domain_names[i]);

		fff=fopen(filename,"r");

		if (fff!=NULL) {
			fscanf(fff,"event=%x",&config);
			printf("Found config=%d\n",config);
			fclose(fff);
		} else {
			continue;
		}

		sprintf(filename,"/sys/bus/event_source/devices/power/events/%s.scale",
			rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			fscanf(fff,"%lf",&scale[i]);
			printf("Found scale=%g\n",scale[i]);
			fclose(fff);
		}

		sprintf(filename,"/sys/bus/event_source/devices/power/events/%s.unit",
			rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			fscanf(fff,"%s",units);
			printf("Found units=%s\n",units);
			fclose(fff);
		}

		memset(&attr,0x0,sizeof(attr));
		attr.type=type;
		attr.config=config;

		fd[i]=perf_event_open(&attr,-1,core,-1,0);
		if (fd[i]<0) {
			if (errno==EACCES) {
				paranoid_value=check_paranoid();
				if (paranoid_value>0) {
					printf("/proc/sys/kernel/perf_event_paranoid is %d\n",paranoid_value);
					printf("The value must be 0 or lower to read system-wide RAPL values\n");
				}

				printf("Permission denied; run as root or adjust paranoid value\n\n");
				return -1;
			}
			else {
				printf("error opening core %d: %s\n\n",
					i, strerror(errno));
				return -1;
			}
		}
	}

	printf("\nSleeping 1 second\n\n");
	sleep(1);

	for(i=0;i<NUM_RAPL_DOMAINS;i++) {
		if (fd[i]!=-1) {
			read(fd[i],&value,8);
			close(fd[i]);

			printf("%s Energy Consumed: %lf %s\n",
				rapl_domain_names[i],
				(double)value*scale[i],units);
		}
	}

	return 0;
}

static int rapl_sysfs(int core) {

	char event_names[NUM_RAPL_DOMAINS][256];
	char filenames[NUM_RAPL_DOMAINS][256];
	char basename[256];
	char tempfile[256];
	long long before[NUM_RAPL_DOMAINS];
	long long after[NUM_RAPL_DOMAINS];
	int valid[NUM_RAPL_DOMAINS];
	int i;
	FILE *fff;

	printf("Using sysfs powercap interface to gather results\n\n");

	/* /sys/class/powercap/intel-rapl/intel-rapl:0/ */
	/* name has name */
	/* energy_uj has energy */
	/* subdirectories intel-rapl:0:0 intel-rapl:0:1 intel-rapl:0:2 */

	i=0;
	sprintf(basename,"/sys/class/powercap/intel-rapl/intel-rapl:%d",
		core);
	sprintf(tempfile,"%s/name",basename);
	fff=fopen(tempfile,"r");
	if (fff==NULL) {
		fprintf(stderr,"Could not open %s\n",tempfile);
		return -1;
	}
	fscanf(fff,"%s",event_names[i]);
	valid[i]=1;
	fclose(fff);
	sprintf(filenames[i],"%s/energy_uj",basename);

	/* Handle subdomains */
	for(i=1;i<NUM_RAPL_DOMAINS;i++) {
		sprintf(tempfile,"%s/intel-rapl:%d:%d/name",
			basename,core,i-1);
		fff=fopen(tempfile,"r");
		if (fff==NULL) {
			fprintf(stderr,"Could not open %s\n",tempfile);
			valid[i]=0;
			continue;
		}
		valid[i]=1;
		fscanf(fff,"%s",event_names[i]);
		fclose(fff);
		sprintf(filenames[i],"%s/intel-rapl:%d:%d/energy_uj",
			basename,core,i-1);

	}

	/* Gather before values */
	for(i=0;i<NUM_RAPL_DOMAINS;i++) {
		if (valid[i]) {
			fff=fopen(filenames[i],"r");
			if (fff==NULL) {
				fprintf(stderr,"Error opening %s!\n",filenames[i]);
			}
			else {
				fscanf(fff,"%lld",&before[i]);
			}
			fclose(fff);
		}
	}

	printf("\nSleeping 1 second\n\n");
	sleep(1);

	/* Gather after values */
	for(i=0;i<NUM_RAPL_DOMAINS;i++) {
		if (valid[i]) {
			fff=fopen(filenames[i],"r");
			if (fff==NULL) {
				fprintf(stderr,"Error opening %s!\n",filenames[i]);
			}
			else {
				fscanf(fff,"%lld",&after[i]);
			}
			fclose(fff);
		}
	}


	for(i=0;i<NUM_RAPL_DOMAINS;i++) {
		if (valid[i]) {
			printf("%s\t: %lfJ\n",event_names[i],
				((double)after[i]-(double)before[i])/1000000.0);
		}
	}

	return 0;

}

int main(int argc, char **argv) {

	int c;
	int force_msr=0,force_perf_event=0,force_sysfs=0;
	int core=0;
	int result=-1;
	int cpu_model;

	printf("\n");

	opterr=0;

	while ((c = getopt (argc, argv, "c:hmps")) != -1) {
		switch (c) {
		case 'c':
			core = atoi(optarg);
			break;
		case 'h':
			printf("Usage: %s [-c core] [-h] [-m]\n\n",argv[0]);
			printf("\t-c core : specifies which core to measure\n");
			printf("\t-h      : displays this help\n");
			printf("\t-m      : forces use of MSR mode\n");
			printf("\t-p      : forces use of perf_event mode\n");
			printf("\t-s      : forces use of sysfs mode\n");
			exit(0);
		case 'm':
			force_msr = 1;
			break;
		case 'p':
			force_perf_event = 1;
			break;
		case 's':
			force_sysfs = 1;
			break;
		default:
			fprintf(stderr,"Unknown option %c\n",c);
			exit(-1);
		}
	}

	(void)force_sysfs;

	cpu_model=detect_cpu();
	detect_packages();

	if ((!force_msr) && (!force_perf_event)) {
		result=rapl_sysfs(core);
	}

	if (result<0) {
		if ((force_perf_event) && (!force_msr)) {
			result=rapl_perf(core);
		}
	}

	if (result<0) {
		result=rapl_msr(core,cpu_model);
	}

	if (result<0) {

		printf("Unable to read RAPL counters.\n");
		printf("* Verify you have an Intel Sandybridge or newer processor\n");
		printf("* You may need to run as root or have /proc/sys/kernel/perf_event_paranoid set properly\n");
		printf("* If using raw msr access, make sure msr module is installed\n");
		printf("\n");

		return -1;

	}

	return 0;
}
