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
	static volatile f9::context* g_current = &g_inital;  // current
	static volatile f9::context* g_switchto = nullptr;  // current
}
namespace f9 {
 	volatile context* f9::current() { return g_current; }
	volatile context* f9::schedule_select(){ // override this for schedual select
		if(g_switchto != nullptr){
			volatile context* ret=nullptr;
			std::swap(g_switchto,ret);
			return ret;
		} else return this;
	}
	// hard simple switch, returns null if there wasn't a pending switch
	volatile context*  f9::switch_to(volatile context* ctx){
		std::swap(g_switchto,ctx);
		chip::request_schedule();
		return ctx;
	}
	__attribute__((naked)) void f9::call_return(){
		__asm volatile(
			"blx r12\n" // r12 has the call


		);


	}

	// we push a new call onto the stack and return the original
	void context::push_call(uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2=0) {
		context nctx(sp,call_return,arg0,arg1,arg2); // make aa new context based off the old stack
		if(is_user()) nctx.set_user() ; else nctx.set_kernel(); // user is set by default
		at(REG::R12) = pc;

		--sp;
		*sp = reinterpret_cast<uint32_t*>(this);
		sp = push_std_context(sp); // new context
				at(REG::LR_RET) = copy.at(REG::LR_RET);

				at(REG::LR) = ptr_to_int(context_error_return); // set error return
				at(REG::PC) = pc;
				at(REG::xPSR) = 0x1000000; /* Thumb bit on */
				at(REG::R0) = arg0;
				at(REG::R1) = arg1;
				at(REG::R2) = arg2;

				_init(ptr_to_int(pc), ptr_to_int(arg)...);
				if(__get_CONTROL() == 0x3)  // we are a unprivliaged thread

				else



				context copy = *this;
				sp -= sizeof(uint32_t);
				*reinterpret_cast<uint32_t*>(sp) = reinterpret_cast<uintptr_t>(this);

			    sp -= RESERVED_REGS * sizeof(uint32_t);
				at(REG::LR) = copy.at(REG::PC); // chain
				at(REG::PC) = ptr_to_int(push_call_return);
				at(REG::R0) = pc;
				at(REG::R1) = ptr_to_int(this);
				//at(REG::R2) =
				//at(REG::R3) = arg3;
	}
} /* namespace f9 */

extern "C" void __attribute__ (( naked ))PendSV_Handler() ;
extern "C" void __attribute__ (( naked ))PendSV_Handler() {
	__asm__ __volatile__ ("push {lr}");
	volatile f9::context* sel = f9::schedule_select();
	if(g_proc_current != sel){
		__asm__ __volatile__ ("pop {lr}");
		g_current->save();
		g_current = sel;
		g_current->restore();
		__asm__ __volatile__ ("bx lr");
	}
	__asm__ __volatile__ ("pop {lr}");				\
	__asm__ __volatile__ ("bx lr");
}
