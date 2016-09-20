#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Oops the kernel"

static int crash_module_init(void) {

	int *p = 0;

	printk("VMW: trying to oops the kernel\n");

	/* Dereference null pointer */
	printk("%d\n", *p);

	return 0;
}

static void crash_module_exit(void) {

	printk("VMW: oops module exiting\n");

}

module_init(crash_module_init);
module_exit(crash_module_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

