/*
 *  cortex-a9-cache.c - change cortex a9 cache replacement
 */

/* Note this doesn't work due to Linux being in non-secure mode :( */

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Change Cortex a9 cache replacement"



static int __init cortex_a9_cache_rr_init(void)
{
        int sctlr_info;

	printk(KERN_INFO "VMW: Setting SCTLR register\n");

	/* Read System Control Register: SCTLR */
        /* Described in Cortex-A9 Technical Regerence Manual */
        /* 7.6.1 and 4.3.9 */

        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (sctlr_info));

//	printk(KERN_INFO "SCTLR info: parity=%d alloc_in_one_way=%d "
//                         "exclusive_cache=%d SMP=%d "
//                         "line_of_zeros=%d l1_prefetch=%d l2_prefetch=%d "
//                         "TLB_maint_broadcast=%d\n",
//                         (sctlr_info>>9)&1,
//                         (sctlr_info>>8)&1,
//                         (sctlr_info>>7)&1,
//                         (sctlr_info>>6)&1,
//                         (sctlr_info>>3)&1,
//                         (sctlr_info>>2)&1,
//                         (sctlr_info>>1)&1,
//                         (sctlr_info>>0)&1);

        printk(KERN_INFO "+ Changing to round-robin (read %x)\n",
               sctlr_info);

        /* Enable RR */
	sctlr_info |=1<<14; // (round robin)

        printk(KERN_INFO "+ writing %x\n",sctlr_info);

        asm volatile("mcr p15, 0, %0, c1, c0, 0\n"
                     : "+r" (sctlr_info));


        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (sctlr_info));


        printk(KERN_INFO "+ re-reading to verify %x\n",sctlr_info);

	return 0;
}

static void __exit cortex_a9_cache_rr_exit(void)
{

        int sctlr_info;

        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (sctlr_info));

        printk(KERN_INFO "Returning to random replacement (current %x)\n",
               sctlr_info);

        /* Enable Random */
	sctlr_info &=~(1<<14);

        printk(KERN_INFO "Writing %x\n",sctlr_info);

        asm volatile("mcr p15, 0, %0, c1, c0, 0\n"
                     : "+r" (sctlr_info));

}

module_init(cortex_a9_cache_rr_init);
module_exit(cortex_a9_cache_rr_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

