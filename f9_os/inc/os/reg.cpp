#include "reg.cpp"
#include <>
namespace {
	chip::irq_state g_idle_state;// filler
	chip::irq_state* g_irq_state = &g_idle_state;
	chip::irq_state* g_next_irq_state;
}

static volatile uint32_t* os_context_switch_hook_default(volatile uint32_t* state) {
	if(g_irq_state) *g_irq_state = state;
	if(!g_next_irq_state) {
		state = g_next_irq_state->regs();
		g_irq_state = g_next_irq_state;
		g_next_irq_state=nullptr;
	}
	return state;
}

chip::os_hook_func os_context_switch_hook= os_context_switch_hook_default;

// 0xE000ED04 - Interrupt Control State Register

namespace chip {
	void set_os_hook(os_hook_func func){
		if(func) os_context_switch_hook = func;
		else os_context_switch_hook=os_context_switch_hook_default;
	}
	irq_state current_irq_state(){
		return g_irq_state;
	}
	__attribute((naked)) void switch_to(irq_state& from, irq_state& to){
		g_irq_state =&from;
		g_next_irq_state =&to;
		raise_context_switch();
	}
	__attribute((naked)) void switch_to(irq_state& to) {
		g_next_irq_state =&to;
		raise_context_switch();
	}
	__attribute((naked)) static void chip::do_call_return(){

	}
	void irq_state::startup(uintptr_t sp, uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2=0, uint32_t arg3=0) {
		irq_state fake;
		auto current_backup = g_irq_state;
		chip::switch_to(*this,*this); // do a fake context switch to save our state

		__asm__ __volatile__ ("push	{lr}");
		__asm__ __volatile__ ("mov r12, r2");
		__asm__ __volatile__ ("mov r2, sp");
		__asm__ __volatile__ ("push	{ r0, r3}");
		__asm__ __volatile__ ("msr 	r3, basepri");
		__asm__ __volatile__ ("mov  r0, %0" : : "r" (_regs) );
		__asm__ __volatile__ ("ldm 	r0, {r2-r11, lr}"); // just save it all in case
		__asm__ __volatile__ ("pop	{ r0, r3}");
		__asm__ __volatile__ ("mov r2, r12"); // we are saved if we do a return from here
		sp -= hw_reg_count;	// add space for calling back here
		_sp = reinterpret_cast<volatile uint32_t*>(sp);
		at(REG::LR) = at(REG::EXC_RETURN); // fix the return
		_ret = EXC_RETURN_BASE;
		// now we push the new call
		push_call(pc, arg0, arg1, arg2, arg3);
		switch_to(*this); //
		// if we come back, we are back to the original call, yea! it works!

		__asm__ __volatile__ ("pop { lr } \nbx lr\n");
	}
}
extern "C" void __attribute__ (( naked ))PendSV_Handler() ;
extern "C" void __attribute__ (( naked ))PendSV_Handler() {
	chip::pendv_irq_context();
}
#if 0
	if(g_irq_state) g_irq_state->save();
	if(g_next_irq_state) {
		g_irq_state = g_next_irq_state;
		g_irq_state->restore();

}
#endif
