/*
 * rasp-pi-dsable-brpred.c -- disable branch predictor on ARM1176
 * Tested on the BCM2835 in the Raspberry Pi
 */


#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Disable ARM1176 branch predictor"

static int __init rasp_pi_brpred_disable_init(void) {

	/* TODO: should verify we are in fact on an ARM11 */

	long *addr=(long *)0xffffffff810e4ca8;

	printk(KERN_INFO "VMW: ffffffff810e4ca8:=%lx\n",
		*addr);

	return 0;
}

static void __exit rasp_pi_brpred_disable_exit(void) {


}

module_init(rasp_pi_brpred_disable_init);
module_exit(rasp_pi_brpred_disable_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

