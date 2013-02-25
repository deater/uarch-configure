/*  
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <asm/mipsregs.h>

int init_module(void)
{
   
   unsigned int x;
   
        x=__read_32bit_c0_register($22, 0);
        
	printk(KERN_INFO "Hello world. %x\n",x);

	/* 
	 * A non 0 return means init_module failed; module can't be loaded. 
	 */
        __write_32bit_c0_register($22, 0, 0x20300000);  /* default 2-bit */   
//        __write_32bit_c0_register($22, 0, 0x20310000);  /* not-taken */        
//        __write_32bit_c0_register($22, 0, 0x20320000);  /* taken */ 
//        __write_32bit_c0_register($22, 0, 0x20330000);  /* fwd=not, back=yes */
        
   
	return 0;
}

void cleanup_module(void)
{
   unsigned int x;
   
        x=__read_32bit_c0_register($22, 0);
	printk(KERN_INFO "Goodbye world %x.\n",x);
}
