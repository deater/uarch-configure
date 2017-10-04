/* Plot the rapl results */
/*									*/
/* Vince Weaver -- vincent.weaver @ maine.edu -- 13 April 2017		*/
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

#include <sys/time.h>
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

/* PSYS RAPL Domain */
#define MSR_PLATFORM_ENERGY_STATUS	0x64d

/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET	0
#define POWER_UNIT_MASK		0x0F

#define ENERGY_UNIT_OFFSET	0x08
#define ENERGY_UNIT_MASK	0x1F00

#define TIME_UNIT_OFFSET	0x10
#define TIME_UNIT_MASK		0xF000


#define MAX_CPUS	1024
#define MAX_PACKAGES	16

double package_energy[MAX_PACKAGES],last_package[MAX_PACKAGES];
double cores_energy[MAX_PACKAGES],last_cores[MAX_PACKAGES];
double uncore_energy[MAX_PACKAGES],last_uncore[MAX_PACKAGES];
double dram_energy[MAX_PACKAGES],last_dram[MAX_PACKAGES];
double psys_energy[MAX_PACKAGES],last_psys[MAX_PACKAGES];

#define PACKAGE 1
#define CORES	2
#define UNCORE	4
#define DRAM	8
#define PSYS	16

static int available=PACKAGE|CORES|UNCORE|DRAM|PSYS;

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
#define CPU_SKYLAKE		78
#define CPU_SKYLAKE_HS		94
#define CPU_SKYLAKE_X		85
#define CPU_KNIGHTS_LANDING	87
#define CPU_KABYLAKE		142
#define CPU_KABYLAKE_2		158


/* TODO: on Skylake, also may support  PSys "platform" domain,	*/
/* the whole SoC not just the package.				*/
/* see dcee75b3b7f025cc6765e6c92ba0a4e59a4d25f4			*/

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
			available=PACKAGE|CORES|UNCORE;
			break;
		case CPU_IVYBRIDGE_EP:
			printf("Ivybridge-EP");
			break;
		case CPU_HASWELL:
			printf("Haswell");
			break;
		case CPU_HASWELL_EP:
			printf("Haswell-EP");
			available=PACKAGE|DRAM;
			break;
		case CPU_BROADWELL_EP:
			printf("Broadwell-EP");
			break;
		case CPU_BROADWELL:
			printf("Broadwell");
			break;
		case CPU_SKYLAKE:
		case CPU_SKYLAKE_HS:
			printf("Skylake");
			break;
		case CPU_SKYLAKE_X:
			printf("Skylake-X");
			break;
		case CPU_KABYLAKE:
		case CPU_KABYLAKE_2:
			printf("Kaby Lake");
			break;
		case CPU_KNIGHTS_LANDING:
			printf("Knight's Landing");
			break;
		default:
			printf("Unsupported model %d\n",model);
			model=-1;
			break;
	}

	printf(" Processor type\n");

	return model;
}


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


static double cpu_energy_units[MAX_PACKAGES],dram_energy_units[MAX_PACKAGES];

/*******************************/
/* MSR code                    */
/*******************************/
static int rapl_detect_msr(int core, int cpu_model) {

	int fd;
	long long result;
	double power_units,time_units;


	double thermal_spec_power,minimum_power,maximum_power,time_window;
	int j;

	if (cpu_model<0) {
		printf("\tUnsupported CPU model %d\n",cpu_model);
		return -1;
	}

	for(j=0;j<total_packages;j++) {

		fd=open_msr(package_map[j]);

		/* Calculate the units used */
		result=read_msr(fd,MSR_RAPL_POWER_UNIT);

		power_units=pow(0.5,(double)(result&0xf));
		cpu_energy_units[j]=pow(0.5,(double)((result>>8)&0x1f));
		time_units=pow(0.5,(double)((result>>16)&0xf));

		/* On Haswell EP and Knights Landing */
		/* The DRAM units differ from the CPU ones */
		if ((cpu_model==CPU_HASWELL_EP) ||
				(cpu_model==CPU_BROADWELL_EP) ||
				(cpu_model==CPU_SKYLAKE_X) ||
				(cpu_model==CPU_KNIGHTS_LANDING)) {
			dram_energy_units[j]=pow(0.5,(double)16);
			printf("DRAM: Using %lf instead of %lf\n",
				dram_energy_units[j],cpu_energy_units[j]);
		}
		else {
			dram_energy_units[j]=cpu_energy_units[j];
		}




		printf("\t\tPower units = %.3fW\n",power_units);
		printf("\t\tCPU Energy units = %.8fJ\n",cpu_energy_units[j]);
		printf("\t\tDRAM Energy units = %.8fJ\n",dram_energy_units[j]);
		printf("\t\tTime units = %.8fs\n",time_units);
		printf("\n");

		/* Show package power info */
		result=read_msr(fd,MSR_PKG_POWER_INFO);
		thermal_spec_power=power_units*(double)(result&0x7fff);
		printf("\t\tPackage thermal spec: %.3fW\n",thermal_spec_power);
		minimum_power=power_units*(double)((result>>16)&0x7fff);
		printf("\t\tPackage minimum power: %.3fW\n",minimum_power);
		maximum_power=power_units*(double)((result>>32)&0x7fff);
		printf("\t\tPackage maximum power: %.3fW\n",maximum_power);
		time_window=time_units*(double)((result>>48)&0x7fff);
		printf("\t\tPackage maximum time window: %.6fs\n",time_window);

		/* Show package power limit */
		result=read_msr(fd,MSR_PKG_RAPL_POWER_LIMIT);
		printf("\t\tPackage power limits are %s\n", (result >> 63) ? "locked" : "unlocked");
		double pkg_power_limit_1 = power_units*(double)((result>>0)&0x7FFF);
		double pkg_time_window_1 = time_units*(double)((result>>17)&0x007F);
		printf("\t\tPackage power limit #1: %.3fW for %.6fs (%s, %s)\n",
			pkg_power_limit_1, pkg_time_window_1,
			(result & (1LL<<15)) ? "enabled" : "disabled",
			(result & (1LL<<16)) ? "clamped" : "not_clamped");
		double pkg_power_limit_2 = power_units*(double)((result>>32)&0x7FFF);
		double pkg_time_window_2 = time_units*(double)((result>>49)&0x007F);
		printf("\t\tPackage power limit #2: %.3fW for %.6fs (%s, %s)\n", 
			pkg_power_limit_2, pkg_time_window_2,
			(result & (1LL<<47)) ? "enabled" : "disabled",
			(result & (1LL<<48)) ? "clamped" : "not_clamped");

		/* only available on *Bridge-EP */
		if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP)) {
			result=read_msr(fd,MSR_PKG_PERF_STATUS);
			double acc_pkg_throttled_time=(double)result*time_units;
			printf("\tAccumulated Package Throttled Time : %.6fs\n",
				acc_pkg_throttled_time);
		}

		/* only available on *Bridge-EP */
		if ((cpu_model==CPU_SANDYBRIDGE_EP) || (cpu_model==CPU_IVYBRIDGE_EP)) {
			result=read_msr(fd,MSR_PP0_PERF_STATUS);
			double acc_pp0_throttled_time=(double)result*time_units;
			printf("\tPowerPlane0 (core) Accumulated Throttled Time "
				": %.6fs\n",acc_pp0_throttled_time);

			result=read_msr(fd,MSR_PP0_POLICY);
			int pp0_policy=(int)result&0x001f;
			printf("\tPowerPlane0 (core) for core %d policy: %d\n",core,pp0_policy);

		}


		if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
			(cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL)) {

			result=read_msr(fd,MSR_PP1_POLICY);
			int pp1_policy=(int)result&0x001f;
			printf("\tPowerPlane1 (on-core GPU if avail) %d policy: %d\n",
				core,pp1_policy);
		}
		close(fd);

	}
	printf("\n");

	return 0;
}


static int rapl_msr(int core, int cpu_model) {

	int fd;
	long long result;

	int j;

	for(j=0;j<total_packages;j++) {

		fd=open_msr(package_map[j]);

		/* Package Energy */
		result=read_msr(fd,MSR_PKG_ENERGY_STATUS);
		package_energy[j]=(double)result*cpu_energy_units[j];

		/* PP0 energy */
		/* Not available on Haswell-EP? */
		result=read_msr(fd,MSR_PP0_ENERGY_STATUS);
		cores_energy[j]=(double)result*cpu_energy_units[j];

		/* PP1 energy */
		/* not available on *Bridge-EP */
		if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE) ||
			(cpu_model==CPU_HASWELL) || (cpu_model==CPU_BROADWELL) ||
			(cpu_model==CPU_SKYLAKE) || (cpu_model==CPU_SKYLAKE_HS) ||
			(cpu_model==CPU_KABYLAKE) || (cpu_model==CPU_KABYLAKE_2)) {

	 		result=read_msr(fd,MSR_PP1_ENERGY_STATUS);
			uncore_energy[j]=(double)result*cpu_energy_units[j];
		}


		/* Updated documentation (but not the Vol3B) says Haswell and	*/
		/* Broadwell have DRAM support too				*/
		if ((cpu_model==CPU_SANDYBRIDGE_EP) ||
			(cpu_model==CPU_IVYBRIDGE_EP) ||
			(cpu_model==CPU_HASWELL_EP) ||
			(cpu_model==CPU_BROADWELL_EP) ||
			(cpu_model==CPU_SKYLAKE_X) ||
			(cpu_model==CPU_HASWELL) ||
			(cpu_model==CPU_BROADWELL) ||
			(cpu_model==CPU_SKYLAKE) ||
			(cpu_model==CPU_SKYLAKE_HS) ||
			(cpu_model==CPU_KABYLAKE) ||
			(cpu_model==CPU_KABYLAKE_2)) {

			result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
			dram_energy[j]=(double)result*dram_energy_units[j];
		}

		/* PSys is Skylake+ */
		if ((cpu_model==CPU_SKYLAKE) || (cpu_model==CPU_SKYLAKE_HS) ||
			(cpu_model==CPU_KABYLAKE) || (cpu_model==CPU_KABYLAKE_2)) {

			result=read_msr(fd,MSR_PLATFORM_ENERGY_STATUS);
			psys_energy[j]=(double)result*cpu_energy_units[j];
		}


		close(fd);
	}


	return 0;
}

static int perf_event_open(struct perf_event_attr *hw_event_uptr,
                    pid_t pid, int cpu, int group_fd, unsigned long flags) {

        return syscall(__NR_perf_event_open,hw_event_uptr, pid, cpu,
                        group_fd, flags);
}

#define NUM_RAPL_DOMAINS	5

char rapl_domain_names[NUM_RAPL_DOMAINS][30]= {
	"energy-cores",
	"energy-gpu",
	"energy-pkg",
	"energy-ram",
	"energy-psys",
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

static int type;
static double scale[NUM_RAPL_DOMAINS];
static char units[NUM_RAPL_DOMAINS][BUFSIZ];
static int config[NUM_RAPL_DOMAINS];
static int paranoid_value;
static int fd[NUM_RAPL_DOMAINS][MAX_PACKAGES];
static long long value;

static int rapl_perf_detect(int core) {

	FILE *fff;

	char filename[BUFSIZ];
	struct perf_event_attr attr;

	int i,j;

	fff=fopen("/sys/bus/event_source/devices/power/type","r");
	if (fff==NULL) {
		printf("\tNo perf_event rapl support found (requires Linux 3.14)\n");
		printf("\tFalling back to raw msr support\n\n");
		return -1;
	}
	fscanf(fff,"%d",&type);
	fclose(fff);

	for(i=0;i<NUM_RAPL_DOMAINS;i++) {

		sprintf(filename,"/sys/bus/event_source/devices/power/events/%s",
			rapl_domain_names[i]);

		fff=fopen(filename,"r");

		if (fff!=NULL) {
			fscanf(fff,"event=%x",&config[i]);
			printf("\tType=%d Event=%s Config=%d ",type,
				rapl_domain_names[i],config[i]);
			fclose(fff);
		} else {
			continue;
		}

		sprintf(filename,"/sys/bus/event_source/devices/power/events/%s.scale",
			rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			fscanf(fff,"%lf",&scale[i]);
			printf("scale=%g ",scale[i]);
			fclose(fff);
		}

		sprintf(filename,"/sys/bus/event_source/devices/power/events/%s.unit",
			rapl_domain_names[i]);
		fff=fopen(filename,"r");

		if (fff!=NULL) {
			fscanf(fff,"%s",units[i]);
			printf("units=%s ",units[i]);
			fclose(fff);
		}

		printf("\n");
	}

	for(j=0;j<total_packages;j++) {

		for(i=0;i<NUM_RAPL_DOMAINS;i++) {

			fd[i][j]=-1;

			memset(&attr,0x0,sizeof(attr));
			attr.type=type;
			attr.config=config[i];
			if (config[i]==0) continue;

			fd[i][j]=perf_event_open(&attr,-1, package_map[j],-1,0);
			if (fd[i][j]<0) {
				if (errno==EACCES) {
					paranoid_value=check_paranoid();
					if (paranoid_value>0) {
						printf("\t/proc/sys/kernel/perf_event_paranoid is %d\n",paranoid_value);
						printf("\tThe value must be 0 or lower to read system-wide RAPL values\n");
					}

					printf("\tPermission denied; run as root or adjust paranoid value\n\n");
					return -1;
				}
				else {
					printf("\terror opening core %d config %d: %s\n\n",
						package_map[j], config[i], strerror(errno));
					return -1;
				}
			}
		}
	}

	return 0;
}


static int rapl_perf(int core) {

	int i,j;


	for(j=0;j<total_packages;j++) {

		for(i=0;i<NUM_RAPL_DOMAINS;i++) {

			if (fd[i][j]!=-1) {
				lseek(fd[i][j],0,SEEK_SET);
				read(fd[i][j],&value,8);
//				close(fd[i][j]);

				if (!strcmp("energy-pkg",rapl_domain_names[i])) {
					package_energy[j]=(double)value*scale[i];
				}
				else if (!strcmp("energy-cores",rapl_domain_names[i])) {
					cores_energy[j]=(double)value*scale[i];
				}
				else if (!strcmp("energy-gpu",rapl_domain_names[i])) {
					uncore_energy[j]=(double)value*scale[i];
				}
				else if (!strcmp("energy-ram",rapl_domain_names[i])) {
					dram_energy[j]=(double)value*scale[i];
				}
				else if (!strcmp("energy-psys",rapl_domain_names[i])) {
					psys_energy[j]=(double)value*scale[i];
				}
				else {
					printf("Unknown %s\n",rapl_domain_names[i]);
				}

			}

		}

	}

	return 0;
}

static int rapl_sysfs(int core) {

	char event_names[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
	char filenames[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
	char basename[MAX_PACKAGES][256];
	char tempfile[256];
	long long before[MAX_PACKAGES][NUM_RAPL_DOMAINS];
	int valid[MAX_PACKAGES][NUM_RAPL_DOMAINS];
	int i,j;
	FILE *fff;

	/* /sys/class/powercap/intel-rapl/intel-rapl:0/ */
	/* name has name */
	/* energy_uj has energy */
	/* subdirectories intel-rapl:0:0 intel-rapl:0:1 intel-rapl:0:2 */

	for(j=0;j<total_packages;j++) {
		i=0;
		sprintf(basename[j],"/sys/class/powercap/intel-rapl/intel-rapl:%d",
			j);
		sprintf(tempfile,"%s/name",basename[j]);
		fff=fopen(tempfile,"r");
		if (fff==NULL) {
			fprintf(stderr,"\tCould not open %s\n",tempfile);
			return -1;
		}
		fscanf(fff,"%s",event_names[j][i]);
		valid[j][i]=1;
		fclose(fff);
		sprintf(filenames[j][i],"%s/energy_uj",basename[j]);

		/* Handle subdomains */
		for(i=1;i<NUM_RAPL_DOMAINS;i++) {
			sprintf(tempfile,"%s/intel-rapl:%d:%d/name",
				basename[j],j,i-1);
			fff=fopen(tempfile,"r");
			if (fff==NULL) {
				//fprintf(stderr,"\tCould not open %s\n",tempfile);
				valid[j][i]=0;
				continue;
			}
			valid[j][i]=1;
			fscanf(fff,"%s",event_names[j][i]);
			fclose(fff);
			sprintf(filenames[j][i],"%s/intel-rapl:%d:%d/energy_uj",
				basename[j],j,i-1);

		}
	}

	/* Gather before values */
	for(j=0;j<total_packages;j++) {
		for(i=0;i<NUM_RAPL_DOMAINS;i++) {
			if (valid[j][i]) {
				fff=fopen(filenames[j][i],"r");
				if (fff==NULL) {
					fprintf(stderr,"\tError opening %s!\n",filenames[j][i]);
				}
				else {
					fscanf(fff,"%lld",&before[j][i]);
					fclose(fff);
				}
			}
		}
	}

	for(j=0;j<total_packages;j++) {
		for(i=0;i<NUM_RAPL_DOMAINS;i++) {
			if (valid[j][i]) {
				if (!strncmp("package",event_names[j][i],7)) {
					package_energy[j]=before[j][i]/1000000.0;
				}
				else if (!strcmp("core",event_names[j][i])) {
					cores_energy[j]=before[j][i]/1000000.0;
				}
				else if (!strcmp("uncore",event_names[j][i])) {
					uncore_energy[j]=before[j][i]/1000000.0;
				}
				else if (!strcmp("dram",event_names[j][i])) {
					dram_energy[j]=before[j][i]/1000000.0;
				}
				else {
					printf("Unknown %s\n",event_names[j][i]);
				}
			}
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
	int use_sysfs=0,use_perf_event=0,use_msr=0;
	int j;
	int first_time=1;

	struct timeval current_time;
	double ct,lt,ot;


	printf("\n");
	printf("RAPL read -- use -s for sysfs, -p for perf_event, -m for msr\n\n");

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
		if (result==0) {
			printf("\nUsing sysfs powercap interface to gather results\n\n");
			use_sysfs=1;
			goto ready;
		}
	}

	if ((force_perf_event) && (!force_msr)) {
		result=rapl_perf_detect(core);
		rapl_perf(core);
		if (result==0) {
			printf("\nUsing perf_event interface to gather results\n\n");
			use_perf_event=1;
			goto ready;
		}
	}

	result=rapl_detect_msr(core,cpu_model);
	rapl_msr(core,cpu_model);

	if (result==0) {
		printf("\nUsing /dev/msr interface to gather results\n\n");
		use_msr=1;
	}

	else {
		printf("Unable to read RAPL counters.\n");
		printf("* Verify you have an Intel Sandybridge or newer processor\n");
		printf("* You may need to run as root or have /proc/sys/kernel/perf_event_paranoid set properly\n");
		printf("* If using raw msr access, make sure msr module is installed\n");
		printf("\n");

		return -1;
	}

ready:

	gettimeofday(&current_time, NULL);
	lt=current_time.tv_sec+(current_time.tv_usec/1000000.0);
	ot=lt;
	for(j=0;j<total_packages;j++) {
		last_package[j]=package_energy[j];
		last_cores[j]=cores_energy[j];
		last_uncore[j]=uncore_energy[j];
		last_dram[j]=dram_energy[j];
		last_psys[j]=psys_energy[j];
	}

	/* PLOT LOOP */
	printf("Time (s)\t");
	for(j=0;j<total_packages;j++) {
		if (available&PACKAGE) printf("Package%d(W)\t",j);
		if (available&CORES) printf("Cores(W)\t");
		if (available&UNCORE) printf("GPU(W)\t\t");
		if (available&DRAM) printf("DRAM(W)\t\t");
		if (available&PSYS) printf("Psys(W)|\t");
	}
	printf("\n");
	while(1) {

		gettimeofday(&current_time, NULL);
		ct=current_time.tv_sec+(current_time.tv_usec/1000000.0);

		if (use_sysfs) {
			result=rapl_sysfs(core);
		}
		else if (use_perf_event) {
			result=rapl_perf(core);
		}
		else if (use_msr) {
			result=rapl_msr(core,cpu_model);
		}

		if (first_time) {
			first_time=0;
		}
		else {
		printf("%lf\t",ct-ot);
		for(j=0;j<total_packages;j++) {
			if (available&PACKAGE) printf("%lf\t",
					(package_energy[j]-last_package[j])/(ct-lt));
			if (available&CORES) printf("%lf\t",
					(cores_energy[j]-last_cores[j])/(ct-lt));
			if (available&UNCORE) printf("%lf\t",
					(uncore_energy[j]-last_uncore[j])/(ct-lt));
			if (available&DRAM) printf("%lf\t",
					(dram_energy[j]-last_dram[j])/(ct-lt));
			if (available&PSYS) printf("%lf\t",
					(psys_energy[j]-last_psys[j])/(ct-lt));
		}
		printf("\n");
		}
		usleep(500000);
		lt=ct;
		for(j=0;j<total_packages;j++) {
			last_package[j]=package_energy[j];
			last_cores[j]=cores_energy[j];
			last_uncore[j]=uncore_energy[j];
			last_dram[j]=dram_energy[j];
			last_psys[j]=psys_energy[j];
		}
		fflush(stdout);
	}

	return 0;
}
