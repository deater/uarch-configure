/*
 *  cortex-a9-config.c - read cortex a9 config info
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Read cortex-a9-config"

static int tlb_size(int value) {

   switch(value) {
     case 0: return 64;
     case 1: return 128;
     case 2: return 256;
     case 3: return 512;
     default: return 0;
   }

   return 0;
}

static int __init cortex_a9_config_init(void)
{
        int tlb_info, actlr_info;

	printk(KERN_INFO "VMW: Checking Cortex A9 stats\n");

	/* TLB INFO */

        asm volatile("mrc p15, 0, %0, c0, c0, 3\n"
                     : "=r" (tlb_info));

	printk(KERN_INFO "TLB info: ilock %d dlock %d, entries %d unified=%d\n",
                         (tlb_info>>16)&0xff,
                         (tlb_info>>8)&0xff,
			 tlb_size((tlb_info>>1)&0x3),
                         !(tlb_info&1));


	/* Cache info: ACTLR */

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


#if 0
        asm volatile("mrc p15, 0, %0, c15, c12, 0\n"
                     : "=r" (control));
        asm volatile("mrc p15, 0, %0, c15, c12, 1\n"
                     : "=r" (cycles));
        asm volatile("mrc p15, 0, %0, c15, c12, 2\n"
                     : "=r" (count1));
        asm volatile("mrc p15, 0, %0, c15, c12, 3\n"
                     : "=r" (count2));

        printk(KERN_INFO "Start: Control=%x Cycles=%x Count1=%x Count2=%x\n",
                          control,cycles,count1,count2);

	/* start counters */
	control=0;
	control|=(0x5<<20); /* evtcount0 = 0x5 = branches */
        control|=(0x7<<12);  /* evtcount1 = 0x7  = instructions */
        /* x = 0 */
        /* CCR overflow interrupts = off = 0 */
        /* 0 */
        /* ECC overflow interrupts = off = 0 */
        /* D div/64 = 0 = off */
        control|=(1<<2); /* reset cycle-count register */
        control|=(1<<1); /* reset count registers */
        control|=(1<<0); /* start counters */

        asm volatile("mcr p15, 0, %0, c15, c12, 0\n"
                     : "+r" (control));


	/* do something */
        /* 1 million instructions */

    asm("\tldr     r2,count                    @ set count\n"
        "\tb       test_loop\n"
        "count:       .word 333332\n"
        "test_loop:\n"
        "\tadd     r2,r2,#-1\n"
        "\tcmp     r2,#0\n"
        "\tbne     test_loop                    @ repeat till zero\n"
       : /* no output registers */
       : /* no inputs */
       : "cc", "r2" /* clobbered */
        );


	/* read results */
        asm volatile("mrc p15, 0, %0, c15, c12, 0\n"
                     : "=r" (control));
        asm volatile("mrc p15, 0, %0, c15, c12, 1\n"
                     : "=r" (cycles));
        asm volatile("mrc p15, 0, %0, c15, c12, 2\n"
                     : "=r" (count1));
        asm volatile("mrc p15, 0, %0, c15, c12, 3\n"
                     : "=r" (count2));


        printk(KERN_INFO "Stop: Control=%x Cycles=%d Count1=%d Count2=%d\n",
                          control,cycles,count1,count2);


	/* disable */
	control=0;
        asm volatile("mcr p15, 0, %0, c15, c12, 0\n"
                     : "+r" (control));

#endif
	return 0;
}

static void __exit cortex_a9_config_exit(void)
{
	printk(KERN_INFO "VMW: Unloading\n");
}

module_init(cortex_a9_config_init);
module_exit(cortex_a9_config_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

