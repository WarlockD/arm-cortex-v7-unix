.syntax unified
.cpu cortex-m4
//.fpu softvfp
.text
.thumb
#define __ASEMBLY__
#include "lazyfpu.h"

FP_HW_FRAME_SIZE =(17*4)
HW_FRAME_SIZE= (20*4)
HW_OFFSET_TO_R0 = (10*4)
EXC_RETURN_BASE =         0xffffffe1
EXC_RETURN_PROCESS_STACK =(1 << 2)
EXC_RETURN_THREAD_MODE  = (1 << 3)
EXC_RETURN_STD_CONTEXT =  (1 << 4)
EXC_RETURN_HANDLER    =   0xfffffff1
EXC_RETURN_PRIVTHR   =  (EXC_RETURN_BASE | EXC_RETURN_STD_CONTEXT |  EXC_RETURN_THREAD_MODE)
EXC_RETURN_UNPRIVTHR =  (EXC_RETURN_BASE | EXC_RETURN_STD_CONTEXT |  EXC_RETURN_THREAD_MODE | EXC_RETURN_PROCESS_STACK)
S_FRAME_SIZE = (10*4)


.macro common_exception_entry disable_irq=1
	mrs		r3, basepri				// R3=Current BASEPRI setting
.if \disable_irq
	mov		r2, #NVIC_SYSH_DISABLE_PRIORITY
	msr		basepri, r2				/* Set the BASEPRI */
.endif
	tst		r14, #EXC_RETURN_PROCESS_STACK /* nonzero if context on process stack */
	itt		ne						/* Branch if context already on the MSP */
	mrsne	r1, psp					/* R1=The process stack pointer (PSP) */
	movne   sp, r1					/* Set the MSP to the PSP */
	/* r1 holds the value of the stack pointer AFTER the exception handling logic
	 * pushed the various registers onto the stack.  Get r2 = the value of the
	 * stack pointer BEFORE the interrupt modified it.
	 */
	mov		r2, sp					/* R2=Copy of the main/process stack pointer */
	add		r2, #HW_XCPT_SIZE		/* R2=MSP/PSP before the interrupt was taken */
	sub		sp, #(4*SW_FPU_REGS)	// skip over fpu stack, not used unless we are switching
	stmdb	sp!, {r2-r11,r14}		/* Save the remaining registers plus the SP value */
	// mrs	r0, ipsr			/* R0=exception number */
	// mov		r1, sp
	 mov		r4, sp /* Also save the top of the stack in a preserved register */
	/* Otherwise, we will re-use the interrupted thread's stack.  That may
	 * mean using either MSP or PSP stack for interrupt level processing (in
	 * kernel mode).
	 */
	//bic		r2, r4, #7				/* Get the stack pointer with 8-byte alignment */
	//mov		sp, r2					/* Instantiate the aligned stack */
	.endm

.macro final_exception_exit
	/* The EXC_RETURN value will be 0xfffffff9 (privileged thread) or 0xfffffff1
	 * (handler mode) if the stack is on the MSP.  It can only be on the PSP if
	 * EXC_RETURN is 0xfffffffd (unprivileged thread)
	 */
	mrs		r2, control				/* R2=Contents of the control register */
	tst		r14, #EXC_RETURN_PROCESS_STACK /* nonzero if context on process stack */
	ittee   eq						/* Branch if privileged */
	biceq	r2, r2, #1				/* Privileged mode */
	msreq	msp, r1					/* R1=The main stack pointer */
	orrne	r2, r2, #1				/* Unprivileged mode */
	msrne	psp, r1					/* R1=The process stack pointer */
	msr		control, r2				/* Save the updated control register */
	msr		basepri, r3				/* Restore interrupts priority masking */
	bx		r14						/* And return */
	.endm

.macro fast_exception_exit	// not checking for context switch so just rewind
	ldmia	r1!, {r2-r11,r14}		/* Recover R4-R11, r14 + 2 temp values */
	add		r1, #(4*SW_FPU_REGS)	// skip the floating point numbers
	final_exception_exit
	.endm

.macro common_exit
	cmp		r0, r1					/* Context switch? */
	beq		2f						/* Branch if no context switch */
	// context switch here
	//bl		up_restorefpu			/* Restore the FPU registers */
	add		r1, r0, #SW_XCPT_SIZE	/* R1=Address of HW save area in reg array */
	ldmia	r1, {r4-r11}			/* Fetch eight registers in HW save area */
	ldr		r1, [r0, #(4*REG_SP)]	/* R1=Value of SP before interrupt */
	stmdb	r1!, {r4-r11}			/* Store eight registers in HW save area */
	ldmia	r0, {r2-r11,r14}		/* Recover R4-R11, r14 + 2 temp values */
2:
	final_exception_exit
	.endm


.macro	v7m_exception_entry
	cpsid	i
	cmp	lr, #0xfffffffd		@ check the return stack
	beq	1f			@ exception on process stack
	add	r12, sp, #32		@ MSP before exception
	stmdb	sp!, {r4-r12, lr}	@ push unsaved registers
	b	2f
1:
	mrs	r12, psp		@ get the process stack
	sub	sp, #HW_FRAME_SIZE		// Size of pt regs
	stmia	sp, {r4-r12, lr}	@ push unsaved registers
	ldmia	r12, {r0-r3, r6, r8-r10} @ load automatically saved registers
	add	r12, sp, #HW_OFFSET_TO_R0 // offset to R0
	stmia	r12, {r0-r3, r6, r8-r10} @ fill in the rest of struct pt_regs
2:
	.endm

.macro	v7m_exception_fast_exit
	ldmia	sp!, {r4-r12, lr}	@ restore previously saved state
	cmp	lr, #0xfffffffd		@ check the return stack
	addeq	sp, #S_FRAME_SIZE >> 1	@ returning to PSP, just restore MSP
	cpsie	i
	bx	lr
	.endm


.macro	disable_irq
	mov r12, #0
	mrs r3, basepri
	cpsid i		/* Errata workaround. */
	msr basepri, r12
	dsb
	isb
	cpsie i		 /* Errata workaround. */
.endm

.macro	enable_irq
	cpsid i		/* Errata workaround. */
	msr basepri, r3
	dsb
	isb
	cpsie i		 /* Errata workaround. */
.endm
.macro	get_stack
	tst lr, #4
	ite     eq
	mrseq r0, msp
	mrsne r0, psp
.endm


/* If symbol 'name' is treated as a subroutine (gets called, and returns)
 * then please use ENDPROC to mark 'name' as STT_FUNC for the benefit of
 * static analysis tools such as stack depth analyzer.
 */

.macro END PROC
	.size \PROC, .-\PROC
	.endm

.macro	ENDPROC PROC
	.type \PROC, function;
	END \PROC
	.endm
	.text

//.global HardFault_Handler
.global HardFault_Handler
HardFault_Handler:
	common_exception_entry
	mov r0, sp
	bl	do_busfault
	fast_exception_exit
ENDPROC HardFault_Handler

.global BusFault_Handler
BusFault_Handler:
	v7m_exception_entry
	mov	r0, sp
	bl	do_busfault
ENDPROC BusFault_Handler

.global UsageFault_Handler
UsageFault_Handler:
	v7m_exception_entry
	mov	r0, sp
	bl	do_usagefault
ENDPROC HardFault_Handler

.global MemManage_Handler
MemManage_Handler:
	v7m_exception_entry
	mov	r0, sp
	bl	do_menmanage
ENDPROC MemManage_Handler
.extern do_irq
exception_common:
	mrs		r3, basepri				// R3=Current BASEPRI setting
	mov		r0, #NVIC_SYSH_DISABLE_PRIORITY
	msr		basepri, r0				/* Set the BASEPRI */
	tst		r14, #EXC_RETURN_PROCESS_STACK /* nonzero if context on process stack */
	itt		ne						/* Branch if context already on the MSP */
	mrsne	r1, psp					/* R1=The process stack pointer (PSP) */
	movne   sp, r1					/* Set the MSP to the PSP */
	mov		r2, sp					/* R2=Copy of the main/process stack pointer */
	add		r2, #HW_XCPT_SIZE		/* R2=MSP/PSP before the interrupt was taken */
	sub		sp, #(4*SW_FPU_REGS)	// skip over fpu stack, not used unless we are switching
	stmdb	sp!, {r2-r11,r14}		/* Save the remaining registers plus the SP value */
	mrs		r0, ipsr			/* R0=exception number */
	mov		r1, sp
	mov		r4, sp /* Also save the top of the stack in a preserved register */
	bl		do_irq
	fast_exception_exit
ENDPROC exception_common
.section  .text.Default_Handler,"ax",%progbits

.global Default_Handler
Default_Handler:
	b exception_common
	//bic		r2, r4, #7				/* Get the stack pointer with 8-byte alignment */
	//mov		sp, r2					/* Instantiate the aligned stack */
	//common_exception_entry
	//mov r0, sp
	//bl	do_defaulthandler
	//fast_exception_exit
Infinite_Loop:
  b  Infinite_Loop
 ENDPROC Default_Handler
 //.size  Default_Handler, .-Default_Handler

#if 0
// in pendsv, we have to save the entire context then call the
// sceduedualer
// the schedualer returns a non zero if we swap
.extern switch_context // int switch_context(int operation, volatile context_t* ptr)
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
	disable_irq
	mrs r2, psp
	sub	r1, r2, #HW_FRAME_SIZE  // put the start of the frame in r1
	tst r14, #0x10	/* Is the task using the FPU context?  If so, push high vfp registers. */
	itt eq
	vstmdbeq r2!, {s16-s31}
	subeq r1, #FP_HW_FRAME_SIZE  // also subtract the fp stuff
	stmdb r2!, {r4-r11, r14} /* Save the core registers. */
	bl switch_context
	ldmia r2!, {r4-r11, r14}
	tst r14, #0x10	// do we push the fpu back on the stack?
	it eq
	vldmiaeq r2!, {s16-s31}
	msr psp, r2
	isb
	bx r14
.size PendSV_Handler, .-PendSV_Handler
#endif
