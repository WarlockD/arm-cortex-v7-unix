/*
 * context.cpp
 *
 *  Created on: Jun 2, 2017
 *      Author: Paul
 */

#include "context.hpp"
#include "cortex_m.h"

namespace {
	static f9::context g_inital;						// the initial context
	static f9::context* g_current = &g_inital;  // current
	static f9::context* g_switchto = nullptr;  // current
}
static void pendset() {
asm volatile (
    "    CPSID     I                 \n" // Prevent interruption during context switch
    "    MRS       R0, PSP           \n" // PSP is process stack pointer
    "    TST       LR, #0x10         \n" // exc_return[4]=0? (it means that current process
    "    IT        EQ                \n" // has active floating point context)
    "    VSTMDBEQ  R0!, {S16-S31}    \n" // if so - save it.
    "    STMDB     R0!, {R4-R11, LR} \n" // save remaining regs r4-11 and LR on process stack

    // At this point, entire context of process has been saved
    "    LDR     R1, =os_context_switch_hook  \n"   // call os_context_switch_hook();
    "    BLX     R1                \n"

    // R0 is new process SP;
    "    LDMIA     R0!, {R4-R11, LR} \n" // Restore r4-11 and LR from new process stack
    "    TST       LR, #0x10         \n" // exc_return[4]=0? (it means that new process
    "    IT        EQ                \n" // has active floating point context)
    "    VLDMIAEQ  R0!, {S16-S31}    \n" // if so - restore it.
    "    MSR       PSP, R0           \n" // Load PSP with new process SP
    "    CPSIE     I                 \n"
    "    BX        LR                \n" // Return to saved exc_return. Exception return will restore remaining context
		);
}
namespace f9 {
 	context* context::current() { return g_current; }
	context* context::schedule_select(){ // override this for schedual select
		if(g_switchto != nullptr){
			context* ret=nullptr;
			std::swap(g_switchto,ret);
			return ret;
		} else return this;
	}
	// hard simple switch, returns null if there wasn't a pending switch
	context*  context::switch_to(context* ctx){
		std::swap(g_switchto,ctx);
		chip::request_schedule();
		return ctx;
	}
	// we push a new call onto the stack and return the original
	void context::push_call(uintptr_t pc, uint32_t arg0, uint32_t arg1, uint32_t arg2) {
		regs.push_handler_call(pc,arg0,arg1,arg2);
	}
} /* namespace f9 */
#if 0
extern "C" void __attribute__ (( naked ))PendSV_Handler() ;
extern "C" void __attribute__ (( naked ))PendSV_Handler() {
	__asm__ __volatile__ ("push {lr}");
	auto sel = g_current->schedule_select();
	__asm__ __volatile__ ("pop {lr}");
	if(g_current != sel){
		g_current->save();
		g_current = sel;
		g_current->restore();
	}
	__asm__ __volatile__ ("bx lr");
}
#endif
