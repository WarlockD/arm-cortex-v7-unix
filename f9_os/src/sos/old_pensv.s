

	/* Disable interrupts: */


	/*
	Exception frame saved by the NVIC hardware onto stack:
	+------+
	|      | <- SP before interrupt (orig. SP)
	| xPSR |
	|  PC  |
	|  LR  |
	|  R12 |
	|  R3  |
	|  R2  |
	|  R1  |
	|  R0  | <- SP after entering interrupt (orig. SP + 32 bytes)
	+------+

	Registers saved by the software (PendSV_Handler):
	+------+
	|  R7  |
	|  R6  |
	|  R5  |
	|  R4  |
	|  R11 |
	|  R10 |
	|  R9  |
	|  R8  | <- Saved SP (orig. SP + 64 bytes)
	+------+
	*/

	/* Save registers R4-R11 (32 bytes) onto current PSP (process stack
	   pointer) and make the PSP point to the last stacked register (R8):
	   - The MRS/MSR instruction is for loading/saving a special registers.
	   - The STMIA inscruction can only save low registers (R0-R7), it is
	     therefore necesary to copy registers R8-R11 into R4-R7 and call
	     STMIA twice. */
	cpsid	i
	tst lr, #4   						@ check the return stack
	ite eq
	mrseq	r2, psp					@ get the process stack
	mrsne	r2, msp					@ get the main stack
	//mrs	r2, psp
	stmdb	r2!,{r4-r11,lr}

	/* Save current task's SP: */
	str	r2, [r0]
	/* Load next task's SP: */
	ldr	r2, [r1]
	ldmia r2!,{r4-r11,lr}
	tst lr, #4
	ite eq
	msreq	psp, r2					@ get the process stack
	msrne	msp, r2,				@ get the main stack
	/* EXC_RETURN - Thread mode with PSP: */
	//ldr r2, =0xFFFFFFFD
	/* Enable interrupts: */
	cpsie	i
	//bx	r2
	bx lr
/*
 * old_pensv.s
 *
 *  Created on: Mar 13, 2017
 *      Author: warlo
 */
