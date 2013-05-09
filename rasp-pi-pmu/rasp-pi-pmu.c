/*
 * rasp-pi-pmu.c -- lets you use the Rasp-Pi Performance Counters
 *                  even though the kernel doesn't due to lack of PMU
 *                  interrupt
 */

#define ICACHE_MISS	0x0
#define IBUF_STALL	0x1
#define DDEP_STALL	0x2	/* Stall because of data dependency */
#define ITLB_MISS	0x3	/* Instruction MicroTLB miss */
#define DTLB_MISS	0x4	/* Data MicroTLB miss */
#define BR_EXEC		0x5	/* Branch instruction executed */
#define BR_MISPREDICT	0x6	/* Branch mispredicted */
#define INSTR_EXEC	0x7	/* Instruction executed */
#define DCACHE_HIT	0x9	/* Data cache hit */
#define DCACHE_ACCESS	0xa	/* Data cache access */
#define DCACHE_MISS	0xb	/* Data cache miss */
#define DCACHE_WBACK	0xc	/* Data cache writeback */
#define SW_PC_CHANGE	0xd	/* Software changed the PC */
#define	MAIN_TLB_MISS	0xf	/* Main TLB miss */
#define	EXPL_D_ACCESS	0x10	/* Explicit external data cache access */
#define LSU_FULL_STALL	0x11	/* Stall because of full Load Store Unit request queue */
#define WBUF_DRAINED	0x12    /* Write buffer drained due to data synchronization barrier or strongly ordered operation */
#define ETMEXTOUT_0	0x20	/* ETMEXTOUT[0] was asserted */
#define ETMEXTOUT_1	0x21	/* ETMEXTOUT[1] was asserted */
#define ETMEXTOUT	0x22	/* Increment once for each of ETMEXTOUT[0] or ETMEXTOUT[1] */
#define PROC_CALL_EXEC	0x23	/* Procedure call instruction executed */
#define	PROC_RET_EXEC	0x24	/* Procedure return instruction executed */
#define PROC_RET_EXEC_PRED 0x25	/* Proceudre return instruction executed and address predicted */
#define PROC_RET_EXEC_PRED_INCORRECT	0x26	/* Procedure return instruction executed and address predicted incorrectly */
#define CPU_CYCLES	0xff

/* Change these to select event that is measured */
static int event1 = BR_EXEC;
static int event2 = INSTR_EXEC;


#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Some tests for rasp-pi counter support"

static int control;

static unsigned int cycles,count1,count2;
static unsigned int old_cycles=0,old_count1=0,old_count2=0;

static long long total_cycles,total_count1,total_count2;

static struct timer_list overflow_timer;

static int overflows=0;

static void read_counters(void) {

	/* read results */
        asm volatile("mrc p15, 0, %0, c15, c12, 0\n"
                     : "=r" (control));
        asm volatile("mrc p15, 0, %0, c15, c12, 1\n"
                     : "=r" (cycles));
        asm volatile("mrc p15, 0, %0, c15, c12, 2\n"
                     : "=r" (count1));
        asm volatile("mrc p15, 0, %0, c15, c12, 3\n"
                     : "=r" (count2));

}

static void update_counters(void) {

	read_counters();

	if (cycles<old_cycles) {
		overflows++;
		total_cycles+=(0xffffffff-old_cycles)+cycles+1;
	} else {
		total_cycles+=(cycles-old_cycles);
	}


	if (count1<old_count1) {
		overflows++;
		total_count1+=(0xffffffff-old_count1)+count1+1;
	} else {
		total_count1+=(count1-old_count1);
	}


	if (count2<old_count2) {
		overflows++;
		total_count2+=(0xffffffff-old_count2)+count2+1;
	} else {
		total_count2+=(count2-old_count2);
	}



	old_cycles=cycles;
	old_count1=count1;
	old_count2=count2;

}

static void avoid_overflow(unsigned long data) {
	mod_timer(&overflow_timer, jiffies + msecs_to_jiffies(250));
	update_counters();
}


static int __init rasp_pi_pmu_init(void)
{

	printk(KERN_INFO "VMW: Starting Rasp-pi 1176 counters, events %x and %x\n",
               event1,event2);

	/* read current values */
	read_counters();

//        printk(KERN_INFO "Start: Control=%x Cycles=%x Count1=%x Count2=%x\n",
//                          control,cycles,count1,count2);

	/* start counters */
	control=0;
	control|=((event1&0xff)<<20);  /* evtcount0 */
        control|=((event2&0xff)<<12);  /* evtcount1 */
        /* x = 0 */
        /* CCR overflow interrupts = off = 0 */
        /* 0 */
        /* ECC overflow interrupts = off = 0 */
        /* D div/64 = 0 = off */
        control|=(1<<2); /* reset cycle-count register */
        control|=(1<<1); /* reset count registers */
        control|=(1<<0); /* start counters */

	total_cycles=0;
	total_count1=0;
	total_count2=0;

	/* start the timer, 250 ms */
	setup_timer(&overflow_timer, avoid_overflow, 0);
	mod_timer(&overflow_timer, jiffies + msecs_to_jiffies(250));

        /* start the counters */

        asm volatile("mcr p15, 0, %0, c15, c12, 0\n"
                     : "+r" (control));

	return 0;
}

static void __exit rasp_pi_pmu_exit(void)
{




	/* update counters */
	update_counters();

        printk(KERN_INFO "VMW Stop: Control=%x Cycles=%lld Count1=%lld Count2=%lld Overflows=%d\n",
                          control,total_cycles,total_count1,total_count2,overflows);


	/* disable */
	control=0;
        asm volatile("mcr p15, 0, %0, c15, c12, 0\n"
                     : "+r" (control));

	del_timer(&overflow_timer);

}

module_init(rasp_pi_pmu_init);
module_exit(rasp_pi_pmu_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

