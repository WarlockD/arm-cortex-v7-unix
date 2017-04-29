.syntax unified
//.cpu cortex-m7
//.fpu softvfp
.text
.thumb

#define __ASEMBLY__
#include "asm.h"
#define HW_STACK_SIZE (8*4)
#define SW_STACK_SIZE (9*4)
@
@ Most of the stack format comes from struct pt_regs, but with
@ the addition of 8 bytes for storing syscall args 5 and 6.
@ This _must_ remain a multiple of 8 for EABI.
@
#define S_OFF		8
/*
 * ARMv7-M exception entry/exit macros.
 *
 * xPSR, ReturnAddress(), LR (R14), R12, R3, R2, R1, and R0 are
 * automatically saved on the current stack (32 words) before
 * switching to the exception stack (SP_main). The order of struct
 * pt_regs members was changed to take advantage of the automatic
 * state saving.
 *
 * If exception is taken while in user mode, SP_main is
 * empty. Otherwise, SP_main is aligned to 64 bit automatically
 * (CCR.STKALIGN set).
 *
 * Linux assumes that the interrupts are disabled when entering an
 * exception handler and it may BUG if this is not the case. Interrupts
 * are disabled during entry and reenabled in the exit macro.
 *
 * The v7m_exception_entry macro preserves the original r0-r5, r7 for
 * the system call arguments.
 *
 * v7_exception_fast_exit is used when returning from interrupts.
 *
 * v7_exception_slow_exit is used when returning from SVC or PendSV.
 * When returning to kernel mode, we don't return from exception.
 */
.macro	v7m_exception_entry
	cpsid	i
	cmp	lr, #0xfffffffd		@ check the return stack
#ifdef CONFIG_VFPM
	cmpne	lr, #0xffffffed
#endif
	beq	1f			@ exception on process stack
	add	r12, sp, #32		@ MSP before exception
	stmdb	sp!, {r4-r12, lr}	@ push unsaved registers
	b	2f
1:
	mrs	r12, psp		@ get the process stack
	sub	sp, #S_FRAME_SIZE
	stmia	sp, {r4-r12, lr}	@ push unsaved registers
	ldmia	r12, {r0-r3, r6, r8-r10} @ load automatically saved registers
	add	r12, sp, #S_R0
	stmia	r12, {r0-r3, r6, r8-r10} @ fill in the rest of struct pt_regs
2:
	.endm

.macro	v7m_exception_fast_exit
	ldmia	sp!, {r4-r12, lr}	@ restore previously saved state
	cmp	lr, #0xfffffffd		@ check the return stack
	it eq
#ifdef CONFIG_VFPM
	cmpne	lr, #0xffffffed
#endif
	addeq	sp, #S_FRAME_SIZE >> 1	@ returning to PSP, just restore MSP
	cpsie	i
	bx	lr
	.endm

.macro	v7m_exception_slow_exit ret_r0
	cpsid	i
	ldr	lr, [sp, #S_EXC_LR]	@ read exception LR
	cmp	lr, #0xfffffffd		@ check the return stack
#ifdef CONFIG_VFPM
	cmpne	lr, #0xffffffed
#endif
	beq	1f			@ returning to PSP
	@ Prepare the MSP stack
	ldmia	sp, {r4-r11}		@ restore previously saved state
	ldr	lr, [sp, #S_PC]
	add	sp, #S_R0
	ldmia	sp, {r0-r3, r12}	@ restore the rest of registers
	add	sp, #32			@ restore the stack pointer
	cpsie	i
	bx	lr
1:
	@ Prepare the PSP stack
	ldr	r12, [sp, #S_SP]	@ read original PSP
	.if	\ret_r0
	add	r11, sp, #S_R1
	ldmia	r11, {r1-r7}		@ read state saved on MSP (r0 preserved)
	.else
	add	r11, sp, #S_R0
	ldmia	r11, {r0-r7}		@ read state saved on MSP
	.endif
	msr	psp, r12		@ restore PSP
	stmia	r12, {r0-r7}		@ restore saved state to PSP
	ldmia	sp, {r4-r11}		@ restore previously saved state
	add	sp, #S_FRAME_SIZE	@ restore the original MSP
	cpsie	i
	bx	lr
.endm

.macro	restore_user_regs, fast = 0, offset = 0
	.if	\offset
	add	sp, #\offset
	.endif
	v7m_exception_slow_exit ret_r0 = \fast
.endm

.macro	svc_exit, rpsr
	clrex					@ clear the exclusive monitor
	ldr	r0, [sp, #S_SP]			@ top of the stack
	ldr	r1, [sp, #S_PC]			@ return address
	tst	r0, #4				@ orig stack 8-byte aligned?
	stmdb	r0, {r1, \rpsr}			@ rfe context
	ldmia	sp, {r0 - r12}
	ldr	lr, [sp, #S_LR]
	addeq	sp, sp, #S_FRAME_SIZE - 8	@ aligned
	addne	sp, sp, #S_FRAME_SIZE - 4	@ not aligned
	rfeia	sp!
.endm

.macro	get_thread_info, rd
	mov	\rd, sp, lsr #13
	mov	\rd, \rd, lsl #13
.endm

.align	2
GLOBAL(do_default)
	v7m_exception_entry

	@
	@ Invoke the IRQ handler
	@
	mrs	r0, ipsr
	and	r0, #0xff
	//sub	r0, #16			@ IRQ number
	mov	r1, sp
	@ routine called with r0 = irq number, r1 = struct pt_regs *
	bl	asm_do_IRQ

	@
	@ Check for any pending work if returning to user
	@
	ldr	lr, [sp, #S_EXC_LR]
	cmp	lr, #0xfffffffd		@ check the return stack
	bne	2f					@ returning to kernel
#if 0
// do pend set then check this in the handler
	get_thread_info tsk
	ldr	r1, [tsk, #TI_FLAGS]
	tst	r1, #_TIF_WORK_MASK
	beq	2f					@ no work pending
	ldr	r1, =0xe000ed04		@ ICSR
	mov	r0, #1 << 28		@ ICSR.PENDSVSET
	str	r0, [r1]		@ raise PendSV
#endif
2:
	v7m_exception_fast_exit
ENDPROC(do_default)


.align	2
GLOBAL(HardFault_Handler)
	v7m_exception_entry

	@
	@ Invoke the Hard Fault handler
	@ routine called with r0 = struct pt_regs *
	mov	r0, sp
	bl	do_hardfault
#if 0
	@ execute the pending work, including reschedule
	get_thread_info tsk
	mov	why, #0
	b	ret_to_user
#endif
        @ v7m_exception_fast_exit
ENDPROC(HardFault_Handler)


.align	2
GLOBAL(MemManage_Handler)

	v7m_exception_entry

	@
	@ Invoke the Hard Fault handler
	@ routine called with r0 = struct pt_regs *
	mov	r0, sp
#ifdef USE_MPU
	bl	do_memmanage
#else
	bl	do_busfault
#endif
#if 0
	@ execute the pending work, including reschedule
	get_thread_info tsk
	mov	why, #0
	b	ret_to_user
#endif


        @ v7m_exception_fast_exit
ENDPROC(MemManage_Handler)

.align	2
GLOBAL(BusFault_Handler)
	v7m_exception_entry

	@
	@ Invoke the Hard Fault handler
	@ routine called with r0 = struct pt_regs *
	mov	r0, sp
	bl	do_busfault
#if 0
	@ execute the pending work, including reschedule
	get_thread_info tsk
	mov	why, #0
	b	ret_to_user
#endif
        @ v7m_exception_fast_exit
ENDPROC(BusFault_Handler)

.align	2
GLOBAL(UsageFault_Handler)
	v7m_exception_entry

	@
	@ Invoke the Hard Fault handler
	@ routine called with r0 = struct pt_regs *
	mov	r0, sp
	bl	do_usagefault
#if 0
	@ execute the pending work, including reschedule
	get_thread_info tsk
	mov	why, #0
	b	ret_to_user
#endif
        @ v7m_exception_fast_exit
ENDPROC(UsageFault_Handler)


