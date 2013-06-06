/* Read the RAPL registers on a sandybridge-ep machine                */
/* Code based on Intel RAPL driver by Zhang Rui <rui.zhang@intel.com> */
/*                                                                    */
/* The /dev/cpu/??/msr driver must be enabled and permissions set     */
/* to allow read access for this to work.                             */
/*                                                                    */
/* Compile with:   gcc -O2 -Wall -o rapl_msr rapl_msr.c -lm           */
/*                                                                    */
/* Vince Weaver -- vweaver1@eecs.utk.edu -- 16 March 2012             */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>

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

int open_msr(int core) {

  char msr_filename[BUFSIZ];
  int fd;

  sprintf(msr_filename, "/dev/cpu/%d/msr", core);
  fd = open(msr_filename, O_RDONLY);
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

long long read_msr(int fd, int which) {

  uint64_t data;

  if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
    perror("rdmsr:pread");
    exit(127);
  }

  return (long long)data;
}

#define CPU_SANDYBRIDGE     42
#define CPU_SANDYBRIDGE_EP  45
#define CPU_IVYBRIDGE       58
#define CPU_IVYBRUDGE_EP    62

int cpu_model=CPU_IVYBRIDGE;

int main(int argc, char **argv) {

  int fd;
  int core=0;
  long long result;
  double power_units,energy_units,time_units;
  double package_before,package_after;
  double pp0_before,pp0_after;
  double pp1_before=0.0,pp1_after;
  double dram_before=0.0,dram_after;
  double thermal_spec_power,minimum_power,maximum_power,time_window;

  printf("\n");

  fd=open_msr(core);

  /* Calculate the units used */
  result=read_msr(fd,MSR_RAPL_POWER_UNIT);
  
  power_units=pow(0.5,(double)(result&0xf));
  energy_units=pow(0.5,(double)((result>>8)&0x1f));
  time_units=pow(0.5,(double)((result>>16)&0xf));

  printf("Power units = %.3fW\n",power_units);
  printf("Energy units = %.8fJ\n",energy_units);
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
  printf("Package maximum time window: %.3fs\n",time_window);


  printf("\n");

  result=read_msr(fd,MSR_RAPL_POWER_UNIT);


  result=read_msr(fd,MSR_PKG_ENERGY_STATUS);  
  package_before=(double)result*energy_units;
  printf("Package energy before: %.6fJ\n",package_before);

  result=read_msr(fd,MSR_PP0_ENERGY_STATUS);  
  pp0_before=(double)result*energy_units;
  printf("PowerPlane0 (core) for core %d energy before: %.6fJ\n",core,pp0_before);

  /* not available on SandyBridge-EP */
  if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE)) {
     result=read_msr(fd,MSR_PP1_ENERGY_STATUS);  
     pp1_before=(double)result*energy_units;
     printf("PowerPlane1 (on-core GPU if avail) before: %.6fJ\n",pp1_before);
  }
  else {
     result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);
     dram_before=(double)result*energy_units;
     printf("DRAM energy before: %.6fJ\n",dram_before);
  }

  printf("\nSleeping 1 second\n\n");
  sleep(1);

  result=read_msr(fd,MSR_PKG_ENERGY_STATUS);  
  package_after=(double)result*energy_units;
  printf("Package energy after: %.6f  (%.6fJ consumed)\n",
	 package_after,package_after-package_before);

  result=read_msr(fd,MSR_PP0_ENERGY_STATUS);  
  pp0_after=(double)result*energy_units;
  printf("PowerPlane0 (core) for core %d energy after: %.6f  (%.6fJ consumed)\n",
	 core,pp0_after,pp0_after-pp0_before);

  /* not available on SandyBridge-EP */
  if ((cpu_model==CPU_SANDYBRIDGE) || (cpu_model==CPU_IVYBRIDGE)) {
     result=read_msr(fd,MSR_PP1_ENERGY_STATUS);  
     pp1_after=(double)result*energy_units;
     printf("PowerPlane1 (on-core GPU if avail) after: %.6f  (%.6fJ consumed)\n",
	 pp1_after,pp1_after-pp1_before);
  }
  else {
     result=read_msr(fd,MSR_DRAM_ENERGY_STATUS);  
     dram_after=(double)result*energy_units;
     printf("DRAM energy after: %.6f  (%.6fJ consumed)\n",
	 dram_after,dram_after-dram_before);
  }

  printf("\n");
  printf("Note: the energy measurements can overflow in 60s or so\n");
  printf("      so try to sample the counters more often than that.\n\n");
  close(fd);

  return 0;
}
