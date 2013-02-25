
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/types.h>

#define MAX_CPUS 32

int main(int argc, char *argv[]) {
	
   uint32_t reg;
   uint64_t data;
   int fd;
   int cpu = 0;
   char msr_file_name[64];

   reg = 0x1a0;

   for(cpu=0;cpu<MAX_CPUS;cpu++) {

      sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
      fd = open(msr_file_name, O_RDONLY);
      if (fd < 0) {
         if (errno == ENXIO) {
	    //fprintf(stderr, "rdmsr: No CPU %d\n", cpu);
	    continue;
	 } else if (errno == EIO) {
	    fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",cpu);
	    exit(3);
	 } else {
	    perror("rdmsr: open");
	    exit(127);
	 }
      }
      
      if (pread(fd, &data, sizeof data, reg) != sizeof data) {
	 if (errno == EIO) {
	    fprintf(stderr, "rdmsr: CPU %d cannot read MSR 0x%08"PRIx32"\n",
				cpu, reg);
	    exit(4);
	 } else {
	    perror("rdmsr: pread");
	    exit(127);
	 }
      }
      
      close(fd);

      printf("CPU %d: Current value is 0x%llx\n",cpu,(long long)data);
      printf("        Fast strings         = %d\n",!!(data&0x1));
      printf("        Thermal control      = %d\n",!!(data&(1ULL<<3)));
      printf("        Performance mon      = %d\n",!!(data&(1ULL<<7)));
      printf("        HW prefetch disabled = %d\n",!!(data&(1ULL<<9)));
      printf("        FERR# multiplex      = %d\n",!!(data&(1ULL<<10)));
      printf("        Branch trace unavail = %d\n",!!(data&(1ULL<<11)));
      printf("        PEBS unavail         = %d\n",!!(data&(1ULL<<12)));
      printf("        Therm avail          = %d\n",!!(data&(1ULL<<13)));
      printf("        Speedstep            = %d\n",!!(data&(1ULL<<16)));
      printf("        FSM                  = %d\n",!!(data&(1ULL<<18)));
      printf("        Adjacent Cache Disab = %d\n",!!(data&(1ULL<<19)));
      printf("        Speedstep lock       = %d\n",!!(data&(1ULL<<20)));
      printf("        Limit CPU Maxval     = %d\n",!!(data&(1ULL<<22)));
      printf("        xTPR disable         = %d\n",!!(data&(1ULL<<23)));
      printf("        XD disable           = %d\n",!!(data&(1ULL<<34)));
      printf("        DCU prefetch disable = %d\n",!!(data&(1ULL<<37)));
      printf("        IDA accel disable    = %d\n",!!(data&(1ULL<<38)));
      printf("        IP prefetch disable  = %d\n",!!(data&(1ULL<<39))); 


      

      data|= (1ULL<<9)|(1ULL<<19)|(1ULL<<37)|(1ULL<<39);
      printf("        Writing out new value: 0x%llx\n",(long long)data);
      

      sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
      fd = open(msr_file_name, O_WRONLY);
      if (fd < 0) {
         if (errno == ENXIO) {
	    fprintf(stderr, "wrmsr: No CPU %d\n", cpu);
	    exit(2);
	 } else if (errno == EIO) {
	    fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n",cpu);
	    exit(3);
	 } else {
	    perror("wrmsr: open");
	    exit(127);
	 }
      }
      
      if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
	 if (errno == EIO) {
	    fprintf(stderr,"wrmsr: CPU %d cannot set MSR "
			   "0x%08"PRIx32" to 0x%016"PRIx64"\n",
			   cpu, reg, data);
	    exit(4);
	 } else {
	    perror("wrmsr: pwrite");
	    exit(127);
         }
      }
      close(fd);      
   }
   
   return 0;
}
