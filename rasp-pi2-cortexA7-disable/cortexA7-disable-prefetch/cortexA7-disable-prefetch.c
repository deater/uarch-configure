/*
 * cortexA7-dsable-prefetch.c -- disable branch predictor on Cotex-A7
 */


#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Disable Cortex-A7 prefetch"

static uint32_t saved_aux=0;

static int __init cortex_a7_disable_init(void) {

	/* TODO: should verify we are in fact on an Cortex-A7 */

	uint32_t aux=0,new_aux=0;

	printk(KERN_INFO "VMW: Start\n");

	/* read AUX ACTRL register */

        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (aux) : : "cc");

	printk(KERN_INFO "Before VMW: aux=%x prefetch=%d\n",
		aux,(aux&(3<<13))>>13);

	saved_aux=aux;

	/* Try to disable */

	/* *NOTE* this won't work if Linux is running in non-secure mode */
	/* Unfortunately most bootloaders put it into this mode */
	/* The only way around this is to use a custom bootloader */
	/* See the custom boot in .. */

	aux&=~(3<<13);

	printk(KERN_INFO "VMW: writing %x\n",aux);

	asm volatile("mcr p15, 0, %0, c1, c0, 1\n"
			: "+r" (aux));
	isb();


        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (new_aux) :: "cc");

	printk(KERN_INFO "After VMW: aux=%x prefetch=%d\n",
		new_aux,(new_aux&(3<<13))>>13);

#if 0
	aux=aux|(1<<28);	/* disale dual issue */

	asm volatile("mcr p15, 0, %0, c1, c0, 1\n"
			: "+r" (aux));
#endif

	return 0;
}

static void __exit cortex_a7_disable_exit(void) {

	/* TODO: properly restore aux register */

	uint32_t aux=0;

        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (aux));

	printk(KERN_INFO "EXIT VMW: aux=%x brpred=%d\n",
		aux,(aux&(3<<13))>>13);


        printk(KERN_INFO "VMW Stop, should restore %x\n",saved_aux);


}

module_init(cortex_a7_disable_init);
module_exit(cortex_a7_disable_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */
