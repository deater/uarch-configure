/*
 *  hello-2.c - Demonstrating the module_init() and module_exit() macros.
 *  This is preferred over using init_module() and cleanup_module().
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Some tests for rasp-pi counter support"

static int __init rasp_pi_pmu_init(void)
{
        int control,cycles,count1,count2;

	printk(KERN_INFO "VMW: Checking Rasp-pi 1176 counter support\n");

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


	return 0;
}

static void __exit rasp_pi_pmu_exit(void)
{
	printk(KERN_INFO "VMW: Unloading\n");
}

module_init(rasp_pi_pmu_init);
module_exit(rasp_pi_pmu_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

