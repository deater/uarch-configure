/* Attempt to make sense of AMD APM support */
/* Application power management */

/* Fam 15h Model 30h (piledriver) */
/* See amd_fam15h_30_bkdg */
/* D18F5xE8 TDP Limit 3 */

/* note http://lm-sensors.lm-sensors.narkive.com/5MI2vOwZ/wrong-output-with-fam15h-power */


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <cpuid.h>

int main(int argc, char **argv) {

	unsigned int eax,ebx,ecx,edx=0;
	unsigned int stepping,model,family;
	char vendor_string[13];
	unsigned int running_avg_range,tdp_running_avg,running_avg_capture;
	int tdp_limit,tdp_limit3,tdp_to_watts;
	unsigned int processor_tdp,base_tdp,tdp;
	int result;
	int fd;

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
	if ((family==0x15) && (model>=0x60)) {
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

	if ((family==0x15) && (model>=0x60)) {
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


	/****************************************************/
	/* ApmPwrReporting: APM accumulated power reporting */
	/****************************************************/
	/* This is what the "perf" tool needs to provide rapl-like results */
	/* Need Excavator (fam15h model60h) or Jaguar/blah (fam16h model 30h) */
	/* Support is noted in cpuid 80000007:edx bit 12    */
	/* See BKDG for AMD Family 15h Models 60h-6Fh Processors */
	/*     BKDG for AMD Family 16h Models 30h-3Fh Processors */

	/* MSRC001_007A Compute Unit Power Accumulator */
	/* MSRC001_007B Max Compute Unit Power Accumulator */

	if (!(edx&(1<<12))) {
		printf("Bit 12 (ApmPwrReporting) not set in cpuid 80000007:edx, no extra support\n\n");
		return 0;
	}
	printf("APM Accumulated Power Supported!\n");

	return 0;

}
