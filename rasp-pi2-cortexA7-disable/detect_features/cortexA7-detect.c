/*
 * cortexA7-detect.c -- read the status of various arch features
 * on Cortex-A7 as found on Pi2
 */

/* All documentations references are against the */
/* Cortex-A7 MPCore Technical Reference Manual (DDIO464D) */


#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Read CortexA7 features"


static int __init cortex_a7_features_init(void) {

	uint32_t control=0,aux=0;

	printk(KERN_INFO "VMW: Start\n");

	/* Read the System Control Register */
	/* See 4.3.27 */

	/* read SCTRL control register */
	/* Bit 11 "Z" is whether Program Flow Prediction is enabled */

        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (control));

	/* Note, branch prediction is always enabled when MMU is on */
	printk(KERN_INFO "VMW: SCTRL=%x icache=%d brpred=%d cache=%d align=%d mmu=%d\n",
		control,
		!!(control&(1<<12)),
		!!(control&(1<<11)),
		!!(control&(1<<2)),
		!!(control&(1<<1)),
		!!(control&(1<<0)));

	/* auxiliary register */
	/* ACTLR */
	/* see 4.3.31 */
        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (aux));

	printk(KERN_INFO "VMW: aux=%x "
		"dual_issue=%d l1_data_prefetch=%d "
		"l1d_read_allocate=%d l2d_read_allocate=%d "
		"optimized_mem_barrier=%d smp=%d\n",
		aux,
		!!(aux&(1<<28)),
		(aux&(3<<13))>>13,
		!!(aux&(1<<12)),
		!!(aux&(1<<11)),
		!!(aux&(1<<10)),
		!!(aux&(1<<6))
		);

	return 0;
}

static void __exit cortex_a7_features_exit(void) {

        printk(KERN_INFO "VMW Stop\n");


}

module_init(cortex_a7_features_init);
module_exit(cortex_a7_features_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

