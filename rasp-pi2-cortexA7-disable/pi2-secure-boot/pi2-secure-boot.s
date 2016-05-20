/* rpi2-secure-boot.s - bootloader to put pi2 in secure mode */

/* uboot /arch/arm/cpu/armv7/nonsec_virt.S */

/* referecnes are to the Cortex-A7 MPCore Technical Referance Manual */
/* unless otherwise specified */

/* To use:
	1. compile this file
	2. mv /boot/kernel7.img /boot/kernel7.img.bak
	3. cat bootblk.bin /boot/kernel7.img.bak > /boot/kernel7.img
	4. Put "kernel_old=1" in your /boot/config.txt
*/

.arch_extension sec
.arch_extension virt

.section .init
.globl _start

/* the vector table for secure state and HYP mode */
_start:
	b	cpu_init		/* reset */
	.word	0			/* undef */
	adr pc,	_secure_monitor
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0

/*
 * secure monitor handler
 * U-boot calls this "software interrupt" in start.S
 * This is executed on a "smc" instruction, we use a "smc #0" to switch
 * to non-secure state.
 * We use only r0 and r1 here, due to constraints in the caller.
 */
_secure_monitor:
	mrc	p15, 0, r1, c1, c1, 0		@ read SCR
	bic	r1, r1, #0x4e			@ clear IRQ, FIQ, EA, nET bits

	@ bit 5 A means if non-secure can set alignment bit
	@ bit 4 F means if non-secure can set 'F' bit (?)

	@ bit 0 NS is nonsecure bit (bit 0)
	@ it does not set nonsecure mode, but just says we want
	@ to access the non-secure versions of the various registers

	orr	r1, r1, #0x31			@ enable NS, AW, FW bits
	mcr	p15, 0, r1, c1, c1, 0		@ write SCR (with NS bit set)
	isb




	@ Disable Caches in NS bank?
	mrc	p15, 0, r0, c1, c0, 0	@ Read System Control Register
	bic	r0, r0, #(SCR_CACHE_ENABLE)
	bic	r0, r0, #(SCR_ICACHE_ENABLE)
	mcr	p15, 0, r0, c1, c0, 0	@ Write System Control Register





	@ Reset CNTVOFF to 0 before leaving monitor mode
	mov	r0, #0
	mcrr	p15, 4, r0, r0, c14		@ Reset CNTVOFF to zero

	movs	pc, lr				@ return to non-secure SVC

cpu_init:

.equ SCR_CACHE_ENABLE,	(1<<2)
.equ SCR_ICACHE_ENABLE, (1<<12)

	@ Set System Control Register (p15,c1,c0) see 4.3.27

	mrc	p15, 0, r0, c1, c0, 0	@ Read System Control Register
					@ Enable cache+icache
	orr	r0, r0, #(SCR_CACHE_ENABLE)
	orr	r0, r0, #(SCR_ICACHE_ENABLE)

	@ Why doesn't disabling these work?

@	bic	r0, r0, #(SCR_CACHE_ENABLE)
@	bic	r0, r0, #(SCR_ICACHE_ENABLE)

	mcr	p15, 0, r0, c1, c0, 0	@ Write System Control Register


.equ AUX_DISABLE_DUAL_ISSUE,	(1<<28)
@ Note, various prefetch and read-allocate modes can be set
.equ AUX_SMP,			(1<<6)


	@ Set Auxiliary Control Register (p15,c1,c0) see 4.3.31

	mrc	p15, 0, r0, c1, c0, 1	@ Read Auxiliary Control Register
	orr	r0, r0, #(AUX_SMP)	@ Enable SMP
	orr	r0, r0, #(AUX_DISABLE_DUAL_ISSUE)
	mcr	p15, 0, r0, c1, c0, 1	@ Write Auxiliary Control Register

	@ Set the Non-Secure Access Control register
	@ So non-secure code can use the various co-processors
	@ (Mostly Floating Point and Vector)
	@ See 4.3.34

	ldr	r1, =0x60c00			@ Allow non-secure on coprocs
	mcr	p15, 0, r1, c1, c1, 2		@ Write the NSACR register

	@ Set Timer frequency
	ldr	r1, =19200000
	mcr	p15, 0, r1, c14, c0, 0		@ write CNTFRQ

	@ Set the Vector Base Address Register (p15,c12,c0) 4.2.13

	@ Set the Monitor interrupt Vector base to _start (i.e. 0x0)
	adr	r1, _start
	mcr	p15, 0, r1, c12, c0, 1		@ set MVBAR to secure vectors

	@ Copy current VBAR to ip (this should also be 0x0 at boot?)
	mrc	p15, 0, ip, c12, c0, 0		@ save secure copy of VBAR

	smc	#0				@ call into MONITOR mode


	@ Set the non-secure vector base to saved one
	mcr	p15, 0, ip, c12, c0, 0		@ write non-secure copy of VBAR

	@ Enable performance counters?
	@ B4.1.117 PMCR, Performance Monitors Control Register,

	@ Perf counters not working because we skip the device tree
	@ I am pretty sure, not because we are disabled

@	mrc	p15, 0, r0, c9, c12, 0
@	orr	r0,r0,#1			@ enable E bit
@	mcr	p15, 0, r0, c9, c12, 0



	mov	r4, #0x8000			@ point to kernel


	@ Prepare to park all but core #0

	mrc     p15, 0, r0, c0, c0, 5		@ Read the Multiproc Affinity
						@ register

	ubfx    r0, r0, #0, #2			@ mask off CPU id
						@ why ubfx and not just and?
	cmp	r0, #0
	beq	normal_boot			@ if not CPU0 then park
						@ core in infinite loop

	@ Mailbox magic?  Configuring the CPU? TODO findout what's going on

	cmp     r0, #0xff
	bge	loop_forever

	ldr	r5, =0x4000008C			@ mbox
	ldr	r3, =0x00000000			@ magic
	str	r3, [r5, r0, lsl #4]

	ldr	r5, =0x400000CC			@ mbox
mbox_loop:
	ldr	r4, [r5, r0, lsl #4]
	cmp	r4, r3
	beq	mbox_loop

normal_boot:

	@ Setup the standard ARM boot info

	mov	r0, #0			@ r0 = boot method (0 usually on pi)
	ldr	r1, =0xc42		@ hardware type
@	ldr	r1, =0xffffffff		@ hardware type
	mov	r2, #0x100		@ start of ATAGS
@	mov	r2, #0x2000		@ point to device tree file
	bx	r4			@ branch to 0x8000

loop_forever:
	wfi
	b	loop_forever		@ infinite loop

