// I just got fucking tired of all the inline asembly
.syntax	unified
.thumb


.thumb_func
#if 0
	stmdb   sp!, {r0-r3, ip, lr}
	bl	trace_hardirqs_off
	ldmia	sp!, {r0-r3, ip, lr}
#endif

.macro save_context, reg
		TST       LR, #4            	 // exc_return[2]=0? (it means that current is thread
	    ITE       EQ                	 // has active floating point context)
	    MRSEQ     \reg, PSP           	 // PSP is process stack pointer
	    MRSNE     \reg, MSP           	 // MSP is process stack pointer
	    TST       LR, #0x10         	 // exc_return[4]=0? (it means that current process
	    ITTE      EQ                	 // has active floating point context)
	    ADDEQ     R2, \reg, #((8*16)*4)   // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
	    VSTMDBEQ  R0!, {S16-S31}    	 // if so - save it.
	    ADDNE     R2, R0, #(8*4)  		 // SP is (HW_REG[8] + 32)
		MRS       R3, BASEPRI			 // get the irq state
	    STMDB     \reg!, {R2-R11, LR} 	 // save remaining regs r4-11 and LR on process stack
.endm

// pushes the context onto the stack and returns it
//
.globl	up_push_context
.type	up_push_context, function
	    CPSID     I                 	 // Prevent interruption during context switch
	    TST       LR, #4            	 // exc_return[2]=0? (it means that current is thread
	    ITE       EQ                	 // has active floating point context)
	    MRSEQ     R12, PSP           	 // PSP is process stack pointer
	    MRSNE     R12, MSP           	 // MSP is process stack pointer
	    TST       LR, #0x10         	 // exc_return[4]=0? (it means that current process
	    ITTE      EQ                	 // has active floating point context)
	    ADDEQ     R2, R12, #((8*16)*4)   // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
	    VSTMDBEQ  R12!, {S16-S31}    	 // if so - save it.
	    ADDNE     R2, R12, #(8*4)  		 // SP is (HW_REG[8] + 32)
		MRS       R3, BASEPRI			 // get the irq state
	    STMDB     R0!, {R2-R11, LR} 	 // save remaining regs r4-11 and LR on process stack
	    ITE       EQ                	 // has active floating point context)
	    MSREQ     PSP, R0            	 // PSP is process stack pointer
	    MSRNE     MSP, R0            	 // MSP is process stack pointer
	    CPSIE     I
.size	up_push_context, .-up_push_context

.thumb_func
.globl	up_restore_context
.type	up_restore_context, function
	    CPSID     I                 	 // Prevent interruption during context switch
	    TST       LR, #4            	 // exc_return[2]=0? (it means that current is thread
	    ITE       EQ                	 // has active floating point context)
	    MRSEQ     R12, PSP           	 // PSP is process stack pointer
	    MRSNE     R12, MSP           	 // MSP is process stack pointer
	    TST       LR, #0x10         	 // exc_return[4]=0? (it means that current process
	    ITTE      EQ                	 // has active floating point context)
	    ADDEQ     R2, R12, #((8*16)*4)   // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
	    VSTMDBEQ  R12!, {S16-S31}    	 // if so - save it.
	    ADDNE     R2, R12, #(8*4)  		 // SP is (HW_REG[8] + 32)
		 MRS       R3, BASEPRI			 // get the irq state
	    STMDB     R12!, {R2-R11, LR} 	 // save remaining regs r4-11 and LR on process stack
	    STR       R0, [R12]             // save the state
	    CPSIE     I
.size	up_restore_context, .-up_restore_context
