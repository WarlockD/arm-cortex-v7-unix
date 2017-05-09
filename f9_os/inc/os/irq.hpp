
#ifndef OS_IRQ_HPP_
#define OS_IRQ_HPP_

#include "types.hpp"


namespace irq {
	static constexpr uint32_t SPL_PRIORITY_MAX =15;
	static constexpr uint32_t SPL_PRIORITY_MIN =1;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_CLOCK =SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_NET =SPL_PRIORITY_MIN+1;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_BIO =SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_IMP =SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_TTY=SPL_PRIORITY_MIN+6;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_SOFT_CLOCK=SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_STAT_CLOCK=SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_VM =SPL_PRIORITY_MIN+6;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_HIGH =SPL_PRIORITY_MAX;
	static constexpr uint32_t SPL_PRIORITY_SCHED =SPL_PRIORITY_MAX;
	static inline void spls(uint32_t x) {
		__asm volatile ( "msr basepri, %0" :  : "r"(x) );
	}
	static inline uint32_t splg() {
		uint32_t ret;
		__asm volatile ( "mrs %0, basepri"  :  "=r"(ret) :);
		return ret;
	}
	static inline uint32_t splx(uint32_t x) {
		uint32_t ret;
		__asm volatile (
				"mrs %0, basepri\n"
				"msr basepri, %1\n"
				: "=r"(ret) : "r"(x) );
		return ret;
	}
	static inline uint32_t spl0() { return splx(SPL_PRIORITY_MAX);}
	static inline uint32_t spl1() { return splx(2);}
	static inline uint32_t spl2() { return splx(3);}
	static inline uint32_t spl3() { return splx(5);}
	static inline uint32_t spl4() { return splx(3);}
	static inline uint32_t spl5() { return splx(2);}
	static inline uint32_t spl6() { return splx(0);}
	static inline uint32_t spl7() { return splx(0);}

	static inline uint32_t splsoftclock() { return splx(SPL_PRIORITY_SOFT_CLOCK);}
	static inline uint32_t splnet() 		{ return splx(SPL_PRIORITY_NET);}
	static inline uint32_t splbio() 		{ return splx(SPL_PRIORITY_BIO);}
	static inline uint32_t splimp() 		{ return splx(SPL_PRIORITY_IMP);}
	static inline uint32_t spltty() 		{ return splx(SPL_PRIORITY_TTY);}
	static inline uint32_t splclock() 	{ return splx(SPL_PRIORITY_CLOCK);}
	static inline uint32_t splstatclock() { return splx(SPL_PRIORITY_STAT_CLOCK);}
	static inline uint32_t splvm() 		{ return splx(SPL_PRIORITY_VM);}
	static inline uint32_t splhigh() 		{ return splx(SPL_PRIORITY_HIGH);}
	static inline uint32_t splsched() 	{ return splx(SPL_PRIORITY_SCHED);}

	static inline  void enable_interrupts() { __asm__ __volatile__ ("cpsie i"); }
	static inline  void disable_interrupts() { __asm__ __volatile__ ("cpsid i"); }

	static inline void set_interrupt_state(uint32_t status){ __asm__ __volatile__ ("MSR PRIMASK, %0\n": : "r"(status):"memory");}

	static inline uint32_t get_interrupt_state()
	{
			uint32_t sr;
		__asm__ __volatile__ ("MRS %0, PRIMASK" : "=r"(sr) );
		return sr;
	}
	// kernel is only active in an isr anyway
	static inline bool in_irq()  {
		uint32_t ret;
		__asm volatile ( "mrs %0, ipsr"  :  "=r"(ret) :);
		return ret != 0;
	}

	static inline void idle(){
		auto s = spl7();
		__asm volatile("wfi":::"memory");
		splx(s);
	}
	typedef void (*timer_callback)();
	static constexpr size_t HZ10 = 10U; // 10 times a second
	static constexpr size_t HZ60 = 60U; // 60 times a second
	static constexpr size_t HZ1000 = 1000U; // 60 times a second, 1ms
	void init_system_timer(timer_callback callback, size_t hz=(HZ10));// -1 is d

	void lock_system_timer();
	void unlock_system_timer();
	size_t ticks(); // ticks from timer

	// swithc from too
	typedef uint32_t* (*switch_callback)(uint32_t*);
	void init_context_switch(switch_callback func);
	void raise_context_switch();
	// takes a prebiult context and fixes it so that it calls a function
	// the the context starts back up
	uint32_t* init_stack_frame( uint32_t * stack, void (*exec)(), void (*exit_func)()=nullptr);

	uint32_t* push_exec(uint32_t* stack, void(*exec)(void*), void* arg) ;

};

#endif
