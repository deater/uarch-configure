/* Attempt to make sense of AMD APM support */
/* Application power management */

/* Fam 15h Model 30h (piledriver) */
/* See amd_fam15h_30_bkdg */
/* D18F5xE8 TDP Limit 3 */

/* note http://lm-sensors.lm-sensors.narkive.com/5MI2vOwZ/wrong-output-with-fam15h-power */

#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include <cpuid.h>



static int open_msr(int core) {

	char msr_filename[BUFSIZ];
	int fd;

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDONLY);
	if ( fd < 0 ) {
		if ( errno == ENXIO ) {
			fprintf(stderr, "rdmsr: No CPU %d\n", core);
			return -1;
		} else if ( errno == EIO ) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
				core);
			return -1;
		} else {
			perror("rdmsr:open");
			fprintf(stderr,"Trying to open %s\n",msr_filename);
			return -1;
		}
	}

	return fd;
}


static uint64_t read_msr(int fd, unsigned int which) {

	uint64_t data;

	if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
		perror("rdmsr:pread");
		return -1;
	}

	printf("MSR: read %"PRIx64"\n",data);

	return data;
}

#if 0
static unsigned long long rdtsc(void) {

	unsigned a,d;

	__asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

	return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}
#endif


static long long N,Tx,Ty,JMax,Jx,Jy,Jdelta,Tdelta;
static int msr_fd,tsc_size;

/****************************************************/
/* ApmPwrReporting: APM accumulated power reporting */
/****************************************************/
/* This is what the "perf" tool needs to provide rapl-like results */
/* Need Excavator (fam15h model60h) or Jaguar/blah (fam16h model 30h) */
/* Support is noted in cpuid 80000007:edx bit 12    */
/* See BKDG for AMD Family 15h Models 60h-6Fh Processors */
/*     BKDG for AMD Family 16h Models 30h-3Fh Processors */
/* For algorithm, see 17.5 in a 2016 edition of the */
/*     AMD64 Architecture Programmer's Manual, Volume 2 */
int init_apm_pwr_reporting(int family) {

	unsigned int eax,ebx,ecx,edx=0;

	double tsc_overflow;

	/* Support is noted in cpuid 80000007:edx bit 12    */
	__get_cpuid (0x80000007,&eax,&ebx,&ecx,&edx);
	if (!(edx&(1<<12))) {
		printf("Bit 12 (ApmPwrReporting) not set in cpuid 80000007:edx, no extra support\n\n");
		return -1;
	}
	printf("APM Accumulated Power Supported!\n");


	/* From 17.5.2 */
	/* To obtain a stable average power value, */
	/* Tm should be on the order of several milliseconds. */

	/* Determine the value of the ratio of Tsample to the TSC period */
	/* (CpuPwrSampleTimeRatio) by executing CPUID Fn8000_0007. */
	/* Call this value N. */
	N=ecx;
	printf("N=%lld %d\n",N,ecx);

	/* Check for perf_tsc */
	/* Support is noted in cpuid 80000001:ecx bit 27    */
	__get_cpuid (0x80000001,&eax,&ebx,&ecx,&edx);
	if (!(ecx&(1<<27))) {
		printf("Bit 27 (ApmPwrReporting) not set in cpuid 80000001:ecx, no extra support\n\n");
		return -1;
	}

	/* Check size of perf_tsc */
	/* Noted in cpuid 80000008:ecx bits 16-17    */
	__get_cpuid (0x80000008,&eax,&ebx,&ecx,&edx);
	switch( (ecx>>16)&3) {
		case 0:	tsc_size=40; break;
		case 1: tsc_size=48; break;
		case 2: tsc_size=56; break;
		case 3: tsc_size=64; break;
	}

	tsc_overflow=pow(2,tsc_size)/100000000.0;

	printf("APM perf_tsc detected, size %d bits, overflow in %lfs!\n",
		tsc_size,tsc_overflow);


	/* Read the full range of the cumulative energy value from */
	/* the register MaxCpuSwPwrAcc.				*/
	/* Jmax = value returned from RDMSR MaxCpuSwPwrAcc.	*/
	/* MSRC001_007B Max Compute Unit Power Accumulator */

	msr_fd=open_msr(0);
	if (msr_fd<0) {
		return -1;
	}

	JMax=read_msr(msr_fd,0xc001007bULL);
	printf("JMax=%llx (%lld)\n",JMax,JMax);

	return 0;
}


int start_apm_pwr_reporting(void) {

	/*At time x, read CpuSwPwrAcc and the TSC */
	/* J_x = value returned by RDMSR CpuSwPwrAcc	*/
	/* T_x = value returned by RDTSC		*/
	/* MSRC001_007A Compute Unit Power Accumulator */
	/* Note: Linux driver uses PTSC rather than RDTSC */
	/* MSRC001_0280 */
	Jx=read_msr(msr_fd,0xc001007aULL);
//	Tx=rdtsc();
	Tx=read_msr(msr_fd,0xc0010280ULL);

	return 0;
}

int stop_apm_pwr_reporting(int family) {

	long long PwrCPUave;

	/* At time y, read CpuSwPwrAcc and the TSC again	*/
	/* J_y = value returned by RDMSR CpuSwPwrAcc		*/
	/* T_y = value returned by RDTSC			*/
	Jy=read_msr(msr_fd,0xc001007aULL);
	Ty=read_msr(msr_fd,0xc0010280ULL);
	//Ty=rdtsc();

	/* Calculate the average power consumption for the processor	*/
	/* core over the measurement interval T_M = (Ty – Tx).		*/
	/* If (J y < J x ), rollover has occurred; */
	/*    set Jdelta = (J y + Jmax) – J x	*/
	/* else Jdelta = J y – J x	*/
	/* PwrCPUave = N * Jdelta / (T y - T x ) */

	if (Jy<Jx) {
		printf("Power overflow!\n");
		Jdelta=(Jy+JMax)-Jx;
	}
	else {
		Jdelta=(Jy-Jx);
	}

	if (Ty<Tx) {
		printf("Bug!  PTSC should not overflow!\n");
		if (family==0x16) {
			printf("On fam16h seems to be only 24 bits???\n");
			Tdelta=(Ty+(1ULL<<24))-Tx;
		}
		else {
			Tdelta=(Ty+(1ULL<<tsc_size))-Tx;
		}
	}
	else {
		Tdelta=(Ty-Tx);
	}

	PwrCPUave=N*Jdelta / (Tdelta);

	printf("deltaJ=%lld deltaT=%lld (%llx - %llx)\n",Jdelta,Tdelta,
		Ty,Tx);
	printf("PwrCPUave=%lldmW = %fW\n",
		PwrCPUave,(double)PwrCPUave/1000.0);

	return 0;
}

int test_apm_pwr_reporting(int family) {

	int result;

	printf("\n###############################\n");
	printf("Testing APM Accumulerated Power\n");
	printf("###############################\n\n");

	result=init_apm_pwr_reporting(family);
	if (result<0) return result;

	printf("\n\nMeasuring power while sleeping 5ms\n");

	start_apm_pwr_reporting();
	usleep(5000);
	stop_apm_pwr_reporting(family);

	printf("\n\nMeasuring power while calculating:\n");

	start_apm_pwr_reporting();

	{ int i; double a=0.0;
	for(i=0;i<20000000;i++) {
		a+=drand48();
		a-=drand48();
		a=a*a;

	}
	printf("%lf\n",a);
	}

	stop_apm_pwr_reporting(family);

	return 0;

}


int test_tdp_reporting(int is_excavator) {

	unsigned int eax,ebx,ecx,edx=0;
	unsigned int running_avg_range,tdp_running_avg,running_avg_capture;
	int tdp_limit,tdp_limit3,tdp_to_watts;
	unsigned int processor_tdp,base_tdp,tdp;
	int fd,result;

	printf("\n###############################\n");
	printf("Testing TDP Reporting\n");
	printf("###############################\n\n");

	/* 2.5.9.1 */
	/* Support for APM is specified by CPUID Fn8000_0007_EDX[CPB]. */
	/* which is bit 9, CPU "core performance boost" */
	__get_cpuid (0x80000007,&eax,&ebx,&ecx,&edx);

	if (!(edx&(1<<9))) {
		printf("Bit 9 (CDP) not set in cpuid 80000007:edx, no APM support\n\n");
		return 0;
	}

	/* APM is enabled if all of the following conditions are true: */
	/* * MSRC001_0015[CpbDis] = 0 for all cores.
	   * D18F4x15C[ApmMasterEn] = 1.
	   * D18F4x15C[BoostSrc] = 01b.
	   * D18F4x15C[NumBoostStates] != 0.
	*/


	/* the drivers/hwmon/fam15h_power.c tweaks the value in */
	/* 00:18.5 Host bridge: Advanced Micro Devices, Inc. [AMD] Family 15h Processor Function 5 */
	/* D18F5xE0 Processor TDP Running Average */
	/* sudo setpci -s 18.5 0xe0.l */
	/* only low 4 bits matter?  Seem to be 0x9 on 4.7/piledriver */
	/* which should be right */


	/* FIXME: we need to do this for each processor package */

	fd=open("/sys/bus/pci/devices/0000:00:18.5/config",O_RDONLY);
	if (fd<0) {
		printf("Couldn't open PCI: %s\n",strerror(errno));
		return 0;
	}

	result=pread(fd,&tdp_running_avg,4,0xe0);
	if (result<4) {
		printf("Couldn't read PCI: %s\n",strerror(errno));
		return 0;
	}

	running_avg_range=tdp_running_avg&0xf;

	printf("Running avg range: %x\n",running_avg_range);
	if (running_avg_range!=9) {
		printf("Later documentation suggests setting this to 0x9\n");
	}
	close(fd);

	/* Extended on excavator */
	if (is_excavator) {
		running_avg_capture = tdp_running_avg >> 4;
		/* sign extend starting bit 27? */
	}
	else {
		running_avg_capture = (tdp_running_avg >> 4)&0x3fffff;
		/* sign extend starting bit 21? */
	}


	/* D18F5xE8 TDP Limit 3 */
	/* 28:16 ApmTdpLimit */
	/* 9:0 Tdp2Watt fixed point 0.10 conversion factor */

	fd=open("/sys/bus/pci/devices/0000:00:18.5/config",O_RDONLY);
	if (fd<0) {
		printf("Couldn't open PCI: %s\n",strerror(errno));
		return 0;
	}

	result=pread(fd,&tdp_limit3,4,0xe8);
	if (result<4) {
		printf("Couldn't read PCI: %s\n",strerror(errno));
		return 0;
	}

	if (is_excavator) {
		tdp_limit = tdp_limit3 >> 16;
	}
	else {
		tdp_limit = (tdp_limit3 >> 16) & 0x1fff;
	}
	close(fd);

	printf("TDP Limit=%d\n",tdp_limit);

	/* This is what the fam15h_power.c driver does */
	/* A lot of this doesn't seem to be documented?*/

	/* read 18f4x1b8 REG_PROCESSOR_TDP -- undocumented? */
	fd=open("/sys/bus/pci/devices/0000:00:18.4/config",O_RDONLY);
	if (fd<0) {
		printf("Couldn't open PCI: %s\n",strerror(errno));
		return 0;
	}

	result=pread(fd,&processor_tdp,4,0x1b8);
	if (result<4) {
		printf("Couldn't read PCI: %s\n",strerror(errno));
		return 0;
	}

	base_tdp=processor_tdp>>16;
	tdp=processor_tdp&0xffff;

	printf("ProcessorTDP: raw=%x, base=%d\n",
		processor_tdp,base_tdp);

	/* bits 10:15 are undocumented.  Lower part of fixed point? */
	tdp_to_watts = ((tdp_limit3 & 0x3ff) << 6) | ((tdp_limit3 >> 10) & 0x3f);
	tdp *= tdp_to_watts;

	/* Must be < 256 */
	if ((tdp >> 16) >= 256) {
		printf("Error, TDP too high\n");
		return 0;
	}
	printf("TDP=%d microwatts\n",tdp);
	tdp=(tdp*15625)>>10;
	printf("TDP=%d microwatts\n",tdp);

	/* D18F5xE0 Processor TDP Running Average */
	/* Time interval = 2^(RunAvgRange + 1) * FreeRunSampleTimer rate. */
	/* A value of 0 disables the TDP running average accumulator capture function. See 2.5.9 [Application Power Management (APM)]. */

	/* D18F4x110 Sample and Residency Timers */


	/* On our machine 029e007e 00 0111 1110 */

	(void)running_avg_capture;

	return 0;
}

#define MAX_CPUS	1024
#define MAX_PACKAGES	16

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


int main(int argc, char **argv) {

	unsigned int eax,ebx,ecx,edx=0;
	unsigned int stepping,model,family;
	char vendor_string[13];
	int is_excavator=0;

	/* Get CPUID leaf 0 which has name and level */
	__get_cpuid (0x0,&eax,&ebx,&ecx,&edx);
	memcpy(vendor_string,&ebx,4);
	memcpy(vendor_string+4,&edx,4);
	memcpy(vendor_string+8,&ecx,4);
	vendor_string[12]=0;

	__get_cpuid (0x1,&eax,&ebx,&ecx,&edx);
	stepping=eax&0xf;
	model=(eax>>4)&0xf;
	family=(eax>>8)&0xf;

	/* All newer procs?? */
	family+=((eax>>20)&0xff);
	model+=(((eax>>16)&0xf)<<4);

	printf("Looking for AMD APM support...\n");

	printf("\tFound family %d model %d stepping %d %s processor\n",
		family,model,stepping,vendor_string);

	if (strcmp("AuthenticAMD",vendor_string)) {
		printf("Not an AMD processor!  Exiting.\n\n");

		return 0;
	}

	if (family<0x15) {
		printf("Family %x processor too old (need at least 0x15)\n\n",
			family);

		return 0;
	}

	if ((family==0x15) && (model>=0x60)) is_excavator=1;



	detect_packages();


	test_tdp_reporting(is_excavator);

	test_apm_pwr_reporting(family);

	return 0;

}
