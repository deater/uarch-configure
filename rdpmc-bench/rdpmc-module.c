/* Code taken from perfctr by Mikael Pettersson */
/* http://user.it.uu.se/~mikpe/linux/perfctr/   */
/* Slightly modified by Vince Weaver <vweaver1@eecs.utk.edu> */

#include <linux/module.h>
#include <linux/init.h>

#include <asm/msr.h>

#define MSR_P5_CESR             0x11
#define MSR_P5_CTR0             0x12
#define P5_CESR_VAL             (0x16 | (3<<6))

#define P6_EVNTSEL0_VAL         (0xC0 | (3<<16) | (1<<22))

#define K7_EVNTSEL0_VAL         (0xC0 | (3<<16) | (1<<22))

#define MSR_P4_IQ_COUNTER0      0x30C
#define P4_CRU_ESCR0_VAL        ((2<<25) | (1<<9) | (0x3<<2))
#define P4_IQ_CCCR0_VAL         ((0x3<<16) | (4<<13) | (1<<12))

#define CORE2_PMC_FIXED_CTR0    ((1<<30) | 0)


#define NITER   64
#define X2(S)   S";"S
#define X8(S)   X2(X2(X2(S)))

#ifdef __x86_64__
#define CR4MOV  "movq"
#else
#define CR4MOV  "movl"
#endif


#define rdtsc_low(low) \
  __asm__ __volatile__("rdtsc" : "=a"(low) : : "edx")


static void __init do_rdpmc(unsigned pmc, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__(X8("rdpmc") : : "c"(pmc) : "eax", "edx");
}

static void __init do_rdmsr(unsigned msr, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__(X8("rdmsr") : : "c"(msr) : "eax", "edx");
}

static void __init do_wrmsr(unsigned msr, unsigned data)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__(X8("wrmsr") : : "c"(msr), "a"(data), "d"(0));
}

static void __init do_rdcr4(unsigned unused1, unsigned unused2)
{
  unsigned i;
  unsigned long dummy;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__(X8(CR4MOV" %%cr4,%0") : "=r"(dummy));
}

static void __init do_wrcr4(unsigned cr4, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__(X8(CR4MOV" %0,%%cr4") : : "r"((long)cr4));
}

static void __init do_rdtsc(unsigned unused1, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__(X8("rdtsc") : : : "eax", "edx");
}

#if 0
static void __init do_wrlvtpc(unsigned val, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i) {
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
    apic_write(APIC_LVTPC, val);
  }
}
#endif

static void __init do_sync_core(unsigned unused1, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i) {
    sync_core();
    sync_core();
    sync_core();
    sync_core();
    sync_core();
    sync_core();
    sync_core();
    sync_core();
  }
}


static void __init do_empty_loop(unsigned unused1, unsigned unused2)
{
  unsigned i;
  for(i = 0; i < NITER/8; ++i)
    __asm__ __volatile__("" : : "c"(0));
}

static unsigned __init run(void (*doit)(unsigned, unsigned),
                           unsigned arg1, unsigned arg2)
{
  unsigned start, stop;
  sync_core();
  rdtsc_low(start);
  (*doit)(arg1, arg2);    /* should take < 2^32 cycles to complete */
  sync_core();
  rdtsc_low(stop);
  return stop - start;
}




static void __init
measure_overheads(unsigned msr_evntsel0, unsigned evntsel0, unsigned msr_perfctr0,
                  unsigned msr_cccr, unsigned cccr_val, unsigned is_core2)
{
  int i;
  unsigned int loop, ticks[14];
  const char *name[14];

  if( msr_evntsel0 )
    wrmsr(msr_evntsel0, 0, 0);
  if( msr_cccr )
    wrmsr(msr_cccr, 0, 0);

  name[0] = "rdtsc";
  ticks[0] = run(do_rdtsc, 0, 0);
  name[1] = "rdpmc";
  ticks[1] = run(do_rdpmc,1,0);
  name[2] = "rdmsr (counter)";
  ticks[2] = msr_perfctr0 ? run(do_rdmsr, msr_perfctr0, 0) : 0;
  name[3] = msr_cccr ? "rdmsr (escr)" : "rdmsr (evntsel)";
  ticks[3] = msr_evntsel0 ? run(do_rdmsr, msr_evntsel0, 0) : 0;
  name[4] = "wrmsr (counter)";
  ticks[4] = msr_perfctr0 ? run(do_wrmsr, msr_perfctr0, 0) : 0;
  name[5] = msr_cccr ? "wrmsr (escr)" : "wrmsr (evntsel)";
  ticks[5] = msr_evntsel0 ? run(do_wrmsr, msr_evntsel0, evntsel0) : 0;
  name[6] = "read cr4";
  ticks[6] = run(do_rdcr4, 0, 0);
  name[7] = "write cr4";
  ticks[7] = run(do_wrcr4, read_cr4(), 0);
  name[8] = "rdpmc (fast)";
  ticks[8] = msr_cccr ? run(do_rdpmc, 0x80000001, 0) : 0;
  name[9] = "rdmsr (cccr)";
  ticks[9] = msr_cccr ? run(do_rdmsr, msr_cccr, 0) : 0;
  name[10] = "wrmsr (cccr)";
  ticks[10] = msr_cccr ? run(do_wrmsr, msr_cccr, cccr_val) : 0;
  name[11] = "sync_core";
  ticks[11] = run(do_sync_core, 0, 0);
  name[12] = "read fixed_ctr0";
  ticks[12] = is_core2 ? run(do_rdpmc, CORE2_PMC_FIXED_CTR0, 0) : 0;
  name[13] = "wrmsr fixed_ctr_ctrl";
  ticks[13] = is_core2 ? run(do_wrmsr, MSR_CORE_PERF_FIXED_CTR_CTRL, 0) : 0;
  /*
  name[14] = "write LVTPC";
  ticks[14] = (perfctr_info.cpu_features & PERFCTR_FEATURE_PCINT)
    ? run(do_wrlvtpc, APIC_DM_NMI|APIC_LVT_MASKED, 0) : 0;
  */

  loop = run(do_empty_loop, 0, 0);

  if( msr_evntsel0 )
    wrmsr(msr_evntsel0, 0, 0);
  if( msr_cccr )
    wrmsr(msr_cccr, 0, 0);

  printk(KERN_INFO "COUNTER_OVERHEAD: NITER == %u\n", NITER);
  printk(KERN_INFO "COUNTER_OVERHEAD: loop overhead is %u cycles\n", loop);
  for(i = 0; i < ARRAY_SIZE(ticks); ++i) {
    unsigned int x;
    if( !ticks[i] )
      continue;
    x = ((ticks[i] - loop) * 10) / NITER;
    printk(KERN_INFO "COUNTER_OVERHEAD: %s cost is %u.%u cycles (%u total)\n",
	   name[i], x/10, x%10, ticks[i]);
  }
}


static inline void perfctr_p5_init_tests(void)
{
  measure_overheads(MSR_P5_CESR, P5_CESR_VAL, MSR_P5_CTR0, 0, 0, 0);
}

static inline void perfctr_p6_init_tests(void)
{
  measure_overheads(MSR_P6_EVNTSEL0, P6_EVNTSEL0_VAL, MSR_P6_PERFCTR0, 0, 
		    0, 0);
}

static inline void perfctr_core2_init_tests(void)
{
  measure_overheads(MSR_P6_EVNTSEL0, P6_EVNTSEL0_VAL, MSR_P6_PERFCTR0, 0, 
		    0, 1);
}

static inline void perfctr_p4_init_tests(void)
{
  measure_overheads(MSR_P4_CRU_ESCR0, P4_CRU_ESCR0_VAL, MSR_P4_IQ_COUNTER0,
		    MSR_P4_IQ_CCCR0, P4_IQ_CCCR0_VAL, 0);
}



static inline void perfctr_k7_init_tests(void)
{
  measure_overheads(MSR_K7_EVNTSEL0, K7_EVNTSEL0_VAL, MSR_K7_PERFCTR0, 0, 0, 0);
}

static inline void perfctr_generic_init_tests(void)
{
  measure_overheads(0, 0, 0, 0, 0, 0);
}




static int __init mymodule_init(void)
{

  printk(KERN_INFO "COUNTER_OVERHEAD: vendor %u, family %u, model %u, stepping %u\n",
	 boot_cpu_data.x86_vendor,
	 boot_cpu_data.x86,
	 boot_cpu_data.x86_model,
	 boot_cpu_data.x86_mask);

  if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD) {
     perfctr_k7_init_tests();
  }
  else if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL) {
     if (boot_cpu_data.x86_model==5) perfctr_p5_init_tests();
     else if (boot_cpu_data.x86_model==6) perfctr_p6_init_tests();
     else if (boot_cpu_data.x86_model==15) perfctr_p4_init_tests();
     else perfctr_core2_init_tests();     
  }
  else {
     perfctr_generic_init_tests();
  }
  return 0;
}

static void __exit mymodule_exit(void)
{
 printk ("COUNTER_OVERHEAD: Unloading.\n");
        return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
