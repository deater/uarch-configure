/*
 * rasp-pi-dsable-brpred.c -- disable branch predictor on ARM1176
 * Tested on the BCM2835 in the Raspberry Pi
 */


#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Disable ARM1176 branch predictor"

static uint32_t saved_control=0;

static int __init rasp_pi_brpred_disable_init(void) {

	/* TODO: should verify we are in fact on an ARM11 */

	uint32_t control=0,new_control=0,aux=0;

	printk(KERN_INFO "VMW: Start\n");

	/* read control register */
	/* Bit 11 "Z" is whether Program Flow Prediction is enabled */

        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (control));

	printk(KERN_INFO "Before VMW: control=%x brpred=%s\n",
		control,((control&(1<<11)))?"enabled":"disabled");

	saved_control=control;

	/* Try to disable control flow prediction	*/
	/* *NOTE* This doesn't work			*/
	/* The first read will show it disabled		*/
	/* But soon after it is enabled again?		*/
	/* branch prediction counter also doesn't change*/

	control&=~(1<<11);

	asm volatile("mcr p15, 0, %0, c1, c0, 0\n"
			: "+r" (control));


        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (new_control));

	printk(KERN_INFO "After VMW: control=%x brpred=%s\n",
		new_control,((new_control&(1<<11)))?"enabled":"disabled");

	/* Disable static/dynamic/return stack in the auxiliary control reg */
	/* This actually does seem to work and affects branch miss rate */

        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (aux));

        printk(KERN_INFO "VMW: aux=%x folding=%s static=%s dynamic=%s return=%s\n",
                aux,
                (aux&0x8)?"enabled":"disabled",
                (aux&0x4)?"enabled":"disabled",
                (aux&0x2)?"enabled":"disabled",
                (aux&0x1)?"enabled":"disabled"
                );

	aux=aux&~0x4;
	aux=aux&~0x2;
	aux=aux&~0x1;

	asm volatile("mcr p15, 0, %0, c1, c0, 1\n"
			: "+r" (aux));



	return 0;
}

static void __exit rasp_pi_brpred_disable_exit(void) {

	/* TODO: properly restore aux register */

	uint32_t control=0;

        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (control));

	printk(KERN_INFO "EXIT VMW: control=%x brpred=%s\n",
		control,((control&(1<<11)))?"enabled":"disabled");


        printk(KERN_INFO "VMW Stop, should restore %x\n",saved_control);


}

module_init(rasp_pi_brpred_disable_init);
module_exit(rasp_pi_brpred_disable_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

