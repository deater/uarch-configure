/*
 *  cortex-a9-l2-prefetch.c - enable / disable cortex a9 L2 hardware prefetch
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#include <linux/io.h>

#include <asm/hardware/cache-l2x0.h>

extern void omap_smc1(u32 fn, u32 arg);


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

	printk(KERN_INFO "+ PL310 disable prefetch aux = %x\n",aux);

        printk(KERN_INFO "+ InstructionPrefetch=%d DataPrefetch=%d "
                         "NonSecInt=%d NonSecLock=%d "
                         "ForceWrite=%d SharedApp=%d "
                         "Parity=%d EventMon=%d "
                         "WaySize=%d Assoc=%d "
                         "Exclusive=%d LatTag=%d "
	                 "LatWrite=%d LatRead=%d\n",
	       (aux>>29)&1,
               (aux>>28)&1,
	       (aux>>27)&1,
               (aux>>26)&1,
	       (aux>>23)&3,
               (aux>>22)&1,
	       (aux>>21)&1,
               (aux>>20)&1,
	       (aux>>17)&7,
               (aux>>16)&1,
	       (aux>>12)&1,
               (aux>>6)&7,
	       (aux>>3)&7,
               (aux>>0)&7);


	/* disable prefetch */
	aux&=~(1<<29);
        aux&=~(1<<28);

	//        if (omap_rev() != OMAP4430_REV_ES1_0)
	   omap_smc1(0x109, aux);


	   //	writel_relaxed(aux, l2cache_base + L2X0_AUX_CTRL);


	return 0;
}

static void __exit cortex_a9_prefetch_exit(void)
{

        int aux;

        aux = readl_relaxed(l2cache_base + L2X0_AUX_CTRL);

	printk(KERN_INFO "+ PL310 re-enable prefetch aux = %x\n",aux);

	/* enable prefetch */
	aux|=(1<<29);
        aux|=(1<<28);

	writel_relaxed(aux, l2cache_base + L2X0_AUX_CTRL);

}

module_init(cortex_a9_prefetch_init);
module_exit(cortex_a9_prefetch_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

