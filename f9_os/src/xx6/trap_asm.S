//#define __ASSEMBLER__
#include "arm.h"


// should be run in protection mode
// but we are still using the sp stack?
// need to trace down when switch is called from, what isr
# Context switch
#
#   void swtch(struct context **old, struct context *new);
#
# Save current register context in old
# and then load register context from new.
# The stack is as r4_svc-r12_svc, lr_svc, sp_usr, lr_usr, and pc_usr
// this just swaps the user stack
GLOBAL(swtch)
	stmfd 	sp!, {r4-r11, lr}		// store unsaved regesters
	str 	sp,  [r0]
	mov 	sp,  r1
	stmfd 	r0,  {r4-r11, lr}		// store unsaved regesters
	bx		lr
END(swtch)

.macro ctx_trap_save
	tst lr, #4
	ite eq
	mrseq r0, msp
	mrsne r0, psp
	add r2, r0, #(8*4)		// lazy stacking check
	mrs r3, basepri
	stmdb r0!, {r2-r11, lr}
	mov r4, sp	// save the pointer
	mov r5, r0  // save the stack, just in case
	mov sp, r0
.endm
.macro ctx_trap_restore
	mov sp, r4	// restore the pointer
	mov r0, r5
	ldm r0!, {r2-r11, lr}
	msr basepri, r3
	bx lr
.endm

.macro	v7m_exception_entry
	cpsid	i						// to support nested expcetions, its disabled till the stack is fixed
	mov		r2, sp					// R2=Copy of the main/process stack pointer
	add		r2, #TF_HW_SIZE			// R2=MSP/PSP before the interrupt was taken
	mrs     r3, basepri				// copy base pri
	stmdb	sp!, {r2-r11,lr}		// Save the remaining registers plus the SP value
	mov		r4, sp					// Save the current stack
	bic		r2, r4, #7				// Get the stack pointer with 8-byte alignment
	mov		sp, r2					// Instantiate the aligned stack
	cpsie	i						// re enable interrupt
.endm

.macro	v7m_exception_fast_exit
//v7m_exception_fast_exit:
	cpsid	i
	// check on context switch here but just continue
	mov		r1, r4					/* Recover R1=main stack pointer */
	ldmia	r1!, {r2-r11,lr}		/* Recover R4-R11, r14 + 2 temp values */
	msr		msp, r1					/* Recover the return MSP value */
	msr		basepri, r3				/* Restore interrupts priority masking */
	cpsie	i					// reinable with the base pri set up
	bx	lr
.endm

GLOBAL(trapret)
	v7m_exception_fast_exit
END(trapret)

// int do_trap(int irq, struct trap_frame*)
GLOBAL(all_trap_handle)
	v7m_exception_entry
	mrs	r0, ipsr
	and	r0, #0xff
	sub	r0, #16			@ IRQ number
	mov	r1, r4			// get trapframe
	bl  do_trap
	ldr	lr, [sp, #REGS_NAME(ex_lr)]
	v7m_exception_fast_exit
END(all_trap_handle)

GLOBAL(SysTick_Handler)
	v7m_exception_entry
	mov	r0, r4
	bl do_systick			// void do_systick(struct trap_frame*)
	v7m_exception_fast_exit
END(SysTick_Handler)

#if 0

GLOBAL(SVC_Handler)
	v7m_exception_entry
   	ldr		r4, [sp, #REGS_NAME(pc)]			@ get SWI instruction
   	ldrb 	r5, [r4, -2]

clrex
	v7m_exception_fast_exit
END(SVC_Handler)


GLOBAL(PendSV_Handler)

END(PendSV_Handler)





#endif

// handle faults here
.extern hard_fault_trap
// this is just the extra stuff

// to try to keep the stack clean for the elcipse debugger
// I am copying eveything to a stack varaiable
GLOBAL(HardFault_Handler)
	ldr r1, =hard_fault_trap
	tst lr, #4
	ite eq
	mrseq r0, msp
	mrsne r0, psp
	add r2, r0, #(8*4)		// lazy stacking check
	mrs r3, basepri
	stmia r1!, {r2-r11, lr}
	//v7m_exception_entry
	//mov	r0, r4
	//ctx_trap_save
	//mov r0, sp
	//mov r4, lr
	b do_hardfault			// void do_systick(struct trap_frame*)
	ctx_trap_restore
	//v7m_exception_fast_exit
END(HardFault_Handler)

GLOBAL(MemManage_Handler)
	v7m_exception_entry
	mov	r0, r4
	bl do_memmangefault			// void do_systick(struct trap_frame*)
	v7m_exception_fast_exit
END(MemManage_Handler)

GLOBAL(BusFault_Handler)
	v7m_exception_entry
	mov	r0, r4
	bl do_busfault			// void do_systick(struct trap_frame*)
	v7m_exception_fast_exit
END(BusFault_Handler)

GLOBAL(UsageFault_Handler)
	v7m_exception_entry
	mov	r0, r4
	bl do_usagefault			// void do_systick(struct trap_frame*)
	v7m_exception_fast_exit
END(UsageFault_Handler)

