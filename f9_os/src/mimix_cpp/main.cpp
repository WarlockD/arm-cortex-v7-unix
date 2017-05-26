/*
 * main.cpp
 *
 *  Created on: May 16, 2017
 *      Author: Paul
 */

#include "main.hpp"
#include <stm32f7xx.h>
#include <stm32746g_discovery.h>

namespace mimx {
#if 0
	class test_task: public task<1024> {
		int _waitfor;
		int _sendto;
		int _tasknr;
		message m;
	public:
		test_task(int tasknr, int waitfor, int sendto) : task(tasknr), _waitfor(waitfor), _sendto(sendto), _tasknr(tasknr) {}
		void run() {
			printk("Task %d starting\r\n", _tasknr);
		//	m.m_type = 1; //start up
			//send(_sendto, &m);
			//receive(_sendto,&m);
			pid_t src;
			if(m.type() == 0)
				printk("Task %d linked to  %d type %d\r\n", _tasknr, _sendto);
			while(true) {
				src = receive(ANY,&m);
				printk("Task %d recived %d type %d\r\n", _tasknr, src,m.m_type);
				m.m_type = 0; // anoloege
				send(m.m_source, &m);
			}
		}

	};
	class boot_task: public task<1024> {
		message m;
	public:
		boot_task() : task(1) { proc::bill_ptr = proc::proc_ptr = _proc;}
		void run() {
			printk("Boot task started!\r\n");
			while(true) {
				m.m_type = 1;
				send(3, &m);
				receive(3, &m);
				m.m_type = 1;
				send(4, &m);
				receive(4, &m);
			}
		}
		stackframe_s* startup_stack() { return _proc->p_reg; }
	};
	boot_task btask;
	test_task task1(3,4,4);
	test_task task2(4,3,3);
	void mem_init(){
		  /* Initialize the interrupt controller. */
		//  intr_init(1);

		  /* Interpret memory sizes. */
		  mem_init();

		  /* Clear the process table.
		   * Set up mappings for proc_addr() and proc_number() macros.
		   */
		  proc* rp;
		  int t;
		  for (rp = proc::BEG_PROC_ADDR(), t = -NR_TASKS; rp < proc::END_PROC_ADDR(); ++rp, ++t) {
			rp->p_nr = t;		/* proc number from ptr */
		        (proc::pproc_addr + NR_TASKS)[t] = rp;        /* proc ptr from number */
		  }

	}
#define init_ctx_switch(ctx, r0, pc) \
	__asm__ __volatile__ ("mov r4, %0" : : "r" ((ctx)));	\
	__asm__ __volatile__ ("mov r5, %0" : : "r" ((r0)));	\
	__asm__ __volatile__ ("mov r6, %0" : : "r" ((pc)));	\
	__asm__ __volatile__ ("msr psp, r4");	\
	__asm__ __volatile__ ("MOV R0, #2"); \
	__asm__ __volatile__ ("MSR CONTROL, R0"); \
	__asm__ __volatile__ ("mov r0, r5"); \
	__asm__ __volatile__ ("ISB "); \
	__asm__ __volatile__ ("CPSIE   I "); \
	__asm__ __volatile__ ("bx r6");

	static void real_startup(stackframe_s* sp) {
		__set_PSP(reinterpret_cast<uintptr_t>(sp));
		init_ctx_switch(sp,sp->r0(),sp->pc());

	    asm volatile (
#if (defined __SOFTFP__)  // code without FPU
		"    LDR     R4, [%[stack], #(4 * 14)] \n" // Load process entry point into R4
		"    ADD     %[stack], #(4 * 16)       \n" // emulate context restore
#else
		// "    LDR     R4, [%[stack], #(4 * 16)] \n" // Load process entry point into R4
	    "    LDR     R4, [%[stack], #(4 * 16)] \n" // Load process entry point into R4
		"    ADD     %[stack], #(4 * 17)       \n" // emulate context restore
#endif
		"    MSR     PSP, %[stack]             \n" // store process SP to PSP
		"    MOV     R0, #2                    \n" // set up the current (thread) mode: use PSP as stack pointer, privileged level
		"    MSR     CONTROL, R0               \n"
		"    ISB                               \n" // Insert a barrier
	//	"    LDR     R1, =__init_system_timer  \n" //
	//	"    BLX     R1                        \n" //
		"    CPSIE   I                         \n" // Enable interrupts at processor level
		"    BX      R4                        \n" // Jump to process exec() function
		: [stack]"+r" (sp)  // output
	);


	__builtin_unreachable(); // suppress compiler warning "'noreturn' func does return"
	}
#endif

	void startup(){
		printk("mimx::startup\r\n");
		//btask.dump();
	//	__builtin_trap();
		//real_startup(btask.startup_stack());
		while(1);
	}
} /* namespace mimx */
