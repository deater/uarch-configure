/*
 *  cortex-a9-prefetch.c - enable / disable cortex a9 hardware prefetch
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Enable/Disable cortex-a9 hardware prefetch"


static int __init cortex_a9_prefetch_init(void)
{
        int actlr_info;

	printk(KERN_INFO "VMW: Checking Cortex A9 stats\n");

	/* Read Auxiliary Control Register: ACTLR */
        /* Described in Cortex-A9 Technical Regerence Manual */
        /* 7.6.2 and 4.3.10 */

        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (actlr_info));

	printk(KERN_INFO "ACTLR info: parity=%d alloc_in_one_way=%d "
                         "exclusive_cache=%d SMP=%d "
                         "line_of_zeros=%d l1_prefetch=%d l2_prefetch=%d "
                         "TLB_maint_broadcast=%d\n",
                         (actlr_info>>9)&1,
                         (actlr_info>>8)&1,
                         (actlr_info>>7)&1,
                         (actlr_info>>6)&1,
                         (actlr_info>>3)&1,
                         (actlr_info>>2)&1,
                         (actlr_info>>1)&1,
                         (actlr_info>>0)&1);

        printk(KERN_INFO "Enabling L1 and L2 prefetch (read %x)\n",
               actlr_info);

        /* Enable L1 */
	actlr_info |=0x4;

        /* Enable L2 */
        actlr_info |=0x2;

        printk(KERN_INFO "writing %x\n",actlr_info);

        asm volatile("mcr p15, 0, %0, c1, c0, 1\n"
                     : "+r" (actlr_info));


	return 0;
}

static void __exit cortex_a9_prefetch_exit(void)
{

        int actlr_info;

        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (actlr_info));

        printk(KERN_INFO "Disabling L1 and L2 prefetch (current %x)\n",
               actlr_info);

        /* Enable L1 */
	actlr_info &=~0x4;

        /* Enable L2 */
        actlr_info &=~0x2;

        printk(KERN_INFO "Writing %x\n",actlr_info);

        asm volatile("mcr p15, 0, %0, c1, c0, 1\n"
                     : "+r" (actlr_info));

}

module_init(cortex_a9_prefetch_init);
module_exit(cortex_a9_prefetch_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

