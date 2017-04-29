.syntax unified
.cpu cortex-m4
//.fpu softvfp
#define __ASEMBLY__
#include "asm.h"
//#include "syscall.h"
#include "lazyfpu.h"
.thumb

.macro END PROC
	.size \PROC, .-\PROC
	.endm

.macro	ENDPROC PROC
	.type \PROC, function;
	END \PROC
	.endm

.global    atomic_add_or_fail
.type    atomic_add_or_fail, %function
atomic_add_or_fail:
    ldrex r2, [r0]
    add   r2, r2, r1
    strex r1, r2, [r0]
    eor   r0, r1, #1
    bx    lr
HW_STACK_SIZE = (8*4)
SW_STACK_SIZE = (9*4)
STACK_SIZE = HW_STACK_SIZE + SW_STACK_SIZE
.macro	svc_entry
	tst lr, #4   						@ check the return stack
	beq	1f								@ exception on process stack
	add	r12, sp, #HW_STACK_SIZE			@ MSP before exception (8*4)
	stmdb	sp!, {r4-r12, lr}			@ push unsaved registers
	b	2f
1:
	mrs	r12, psp						@ get the process stack
	sub	sp, #STACK_SIZE					@ expand stack for frame
	stmia	sp, {r4-r12, lr}			@ push unsaved registers
	ldmia	r12, {r0-r3, r6, r8-r10} 	@ load automatically saved registers
	add	r12, sp, #SW_STACK_SIZE 		@ offset to R0 (9*4)
	stmia	r12, {r0-r3, r6, r8-r10} 	@ fill in the rest of struct pt_regs
2:
	.endm

.macro	svc_exit
	ldmia	sp!, {r4-r12, lr}			@ restore previously saved state
	cmp	lr, #0xfffffffd					@ check the return stack
	addeq	sp, #STACK_SIZE >> 1		@ returning to PSP, just restore MSP
//	cpsie	i
	bx	lr
	.endm

//.global HardFault_Handler
.extern g_syscalls
.extern debug_syscall
.global SVC_Handler
.align  2
SVC_Handler:
	tst lr, #4   						@ check the return stack
	ite eq
	mrseq	r12, psp					@ get the process stack
	mrsne	r12, msp					@ get the main stack
	ldr 	r12, [r12, #6]				@ get pc pointer to swi
	ldrb	r12, [r12, #2]				@ get the swi argument
	stmdb	sp!, {r4-r12, lr}			@ push unsaved registers
	mov 	r0, r12
	blx debug_syscall
	bx	lr
ENDPROC SVC_Handler

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
