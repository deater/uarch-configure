/*
 * rasp-pi-brpred.c -- read the status of the branch predictor config
 * on ARM1176 BC2835 as found on Pi A/B/A+/B+/Zero
 */


#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#define DRIVER_AUTHOR "Vince Weaver <vincent.weaver@maine.edu>"
#define DRIVER_DESC   "Read the branch predictor status"


static int __init rasp_pi_brpred_init(void)
{
	uint32_t control=0,aux=0,scr=0;

	printk(KERN_INFO "VMW: Start\n");

#if 0
	/* I always worry that TrustZone is blocking access.     */
	/* But I think if Linux is running then TrustZone is not */

	/* read SCR register */
        asm volatile("mrc p15, 0, %0, c1, c1, 0\n"
                     : "=r" (scr));
	printk(KERN_INFO "VMW: scr=%x\n",scr);
#endif

	/* read control register */
	/* Bit 11 "Z" is whether Program Flow Prediction is enabled */


        asm volatile("mrc p15, 0, %0, c1, c0, 0\n"
                     : "=r" (control));

	printk(KERN_INFO "VMW: control=%x brpred=%s\n",
		control,((control&(1<<11)))?"enabled":"disabled");


	/* auxiliary register */

        asm volatile("mrc p15, 0, %0, c1, c0, 1\n"
                     : "=r" (aux));

	printk(KERN_INFO "VMW: aux=%x folding=%s static=%s dynamic=%s return=%s\n",
		aux,
		(aux&0x8)?"enabled":"disabled",
		(aux&0x4)?"enabled":"disabled",
		(aux&0x2)?"enabled":"disabled",
		(aux&0x1)?"enabled":"disabled"
		);

	return 0;
}

static void __exit rasp_pi_brpred_exit(void)
{

        printk(KERN_INFO "VMW Stop\n");


}

module_init(rasp_pi_brpred_init);
module_exit(rasp_pi_brpred_exit);

MODULE_LICENSE("GPL");			/* Don't taint kernel       */
MODULE_AUTHOR(DRIVER_AUTHOR);		/* Who wrote this module?   */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */

