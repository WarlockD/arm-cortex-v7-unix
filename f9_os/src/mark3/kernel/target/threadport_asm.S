.syntax		unified
.thumb
.text
#define __ASSEMBLY__
#include "linkage.h"

/// position in thread.h
#define TOP_OF_STACK (8)
#define START_OF_STACK (12)


#if 0
.global g_pclNext
.global g_pclCurrent
CURR_: .word g_pclCurrent
NEXT_: .word g_pclNext
.global 	PendSV_Handler
.global 	SVC_Handler


.text
.thumb_func
ENTRY(PendSV_Handler)
	cpsid i
	ldr 	r1, CURR_
	mrs 	r3, psp
	str 	r3, [r1, #TOP_OF_STACK]		// store the top of the stack
	ldr 	r2, [r1, #START_OF_STACK]
	stmia 	r2!, {r3-r11, r14}			// save the extra regesters there
#ifndef __SOFTFP__
    tst 	r14, #0x10			 // save more if need be
    it 		eq
    vstmdbeq r2!, {s16-s31}
#endif
	// swap the theads
    // Equivalent of Thread_Swap() -- g_pclNext -> g_pclCurrent
   // ldr r1, CURR_
    ldr 	r0, NEXT_
    ldr 	r0, [r0]
    str 	r0, [r1]
	// restore the new context
	ldr 	r2, [r0, #START_OF_STACK]
	ldmia 	r2!, {r3-r11, r14}
#ifndef __SOFTFP__
    tst 	r14, #0x10
    it 		eq
    vldmiaeq r2!, {s16-s31}
#endif
    // technicly all we have to do is msr psp, r3, but lets use that pointer
    ldr r2, [r0, #TOP_OF_STACK]	// get the start of the stack
    msr psp, r2
    cpsie i
    // lr contains the proper EXC_RETURN value, we're done with exceptions.
    bx lr
ENDPROC(PendSV_Handler)
.text
.thumb_func
ENTRY(SVC_Handler)
	mov 	r0, CURR_
	ldr 	r2, [r0, #START_OF_STACK]
	ldmia 	r2!, {r3-r11, r14}
#ifndef __SOFTFP__
    tst 	r14, #0x10
    it 		eq
    vldmiaeq r2!, {s16-s31}
#endif
    // technicly all we have to do is msr psp, r3, but lets use that pointer
    ldr r2, [r0, #TOP_OF_STACK]	// get the start of the stack
    msr psp, r2
    mrs r0, control
    mov r1, #0x02
    orr r0, r1
    msr control, r0
    cpsie i
    isb
	bx lr
ENDPROC(SVC_Handler)

#endif
