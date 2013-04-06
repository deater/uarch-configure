/*
 *  cortex-a9-l2-prefetch.c - enable / disable cortex a9 L2 hardware prefetch
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#include <linux/io.h>

#include <asm/hardware/cache-l2x0.h>

/* from ./mach-omap2/omap44xx.h */
#define OMAP44XX_L2CACHE_BASE           0x48242000

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Enable/Disable cortex-a9 hardware prefetch"

static void __iomem *l2cache_base;

static int __init cortex_a9_prefetch_init(void)
{
        int aux;

	printk(KERN_INFO "VMW: Checking Cortex A9 PL310 L2 Cache Control\n");

	/* Read PL310 Auxiliary Control Register: ACTLR */
        /* Described in PL310 Cache Controller  Technical Regerence Manual */
        /* 3.3.4 */

        l2cache_base = ioremap(OMAP44XX_L2CACHE_BASE, SZ_4K);

        aux = readl_relaxed(l2cache_base + L2X0_AUX_CTRL);

	printk("+ PL310 aux = %x\n",aux);

	return 0;
}

static void __exit cortex_a9_prefetch_exit(void)
{

        int aux;



}

module_init(cortex_a9_prefetch_init);
module_exit(cortex_a9_prefetch_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

