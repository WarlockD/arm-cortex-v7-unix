/*
 * thread.cpp
 *
 *  Created on: Apr 30, 2017
 *      Author: warlo
 */

#include <os/thread.hpp>



extern "C" os::stack_item_t* os_context_switch_hook(os::stack_item_t* sp) {
	return os::kernel::context_switch_hook(sp);
}



/*
 *   START MULTITASKING
 *
 * Note(s) : 1) os_start() MUST:
 *              a) Setup PendSV and SysTick exception priority to lowest;
 *              b) Call System timer initialization function;
 *              c) Enable interrupts (tasks will run with interrupts enabled);
 *              d) Jump to entry point of the highest priority process.
 */
extern "C" void os_start(os::stack_item_t *sp)
{
#if 0
    // Set PendSV lowest priority value
#if (defined SHP3_WORD_ACCESS)
    SHPR3 |= (0xFF << 16);
#else
    PendSvPriority = 0xFF;
#endif

#if (!defined __SOFTFP__)
    FPCCR |= ASPEN | LSPEN;
#endif

    asm volatile (
#if (defined __SOFTFP__)  // code without FPU
        "    LDR     R4, [%[stack], #(4 * 14)] \n" // Load process entry point into R4
        "    ADD     %[stack], #(4 * 16)       \n" // emulate context restore
#else
        "    LDR     R4, [%[stack], #(4 * 15)] \n" // Load process entry point into R4
        "    ADD     %[stack], #(4 * 17)       \n" // emulate context restore
#endif
        "    MSR     PSP, %[stack]             \n" // store process SP to PSP
        "    MOV     R0, #2                    \n" // set up the current (thread) mode: use PSP as stack pointer, privileged level
        "    MSR     CONTROL, R0               \n"
        "    ISB                               \n" // Insert a barrier
        "    LDR     R1, =__init_system_timer  \n" //
        "    BLX     R1                        \n" //
        "    CPSIE   I                         \n" // Enable interrupts at processor level
        "    BX      R4                        \n" // Jump to process exec() function
        : [stack]"+r" (sp)  // output
    );
#endif
    __builtin_unreachable(); // suppress compiler warning "'noreturn' func does return"
   while(1);
}

namespace os {
	void kernel::start_os() {
		_curproc = &_proc.front();
		 stack_item_t *sp = _curproc->_stack;
		 os_start(sp);
	}

	kernel::proc_lookup_hash_t kernel::_pid_lookup;
		kernel::proc_wait_hash_t kernel::_wait_lookup;
	 list::head<proc,&proc::_link> kernel::_queue;
	 tailq::head<proc,&proc::_runq> kernel::_runq;

	 kernel::proc_list_t kernel::_proc; // list of all procs, not nessarly in order
	 struct ::sigaction kernel::_sigactions[NSIG];
	 int kernel::_errno;
	 int kernel::_curpri;
	 int kernel::_runrun;
	 int kernel::_runout ;
	 proc* kernel::_curproc;	// static?
	 proc* kernel::_schedproc;	// static?
	 int kernel::cold; // humm
	 const char* kernel::panicstr ;
/* The total number of registers saved by software */



/* On entry into an IRQ, the hardware automatically saves the following
 * registers on the stack in this (address) order:
 */
	// takes the current proc stack state and makes a copy
	__attribute__((naked))
	inline int save(stack_item_t * reg){
		__asm volatile (
			"stmia	r0!, {r1-r15}\n"			// push regesters
			"mov r0, #0\n" // return zero
			"bx lr\n"
		 : :: "memory");
		// no return
	}
	__attribute__((naked))
	inline void retu(stack_item_t * reg){
		__asm volatile (
			"ldrdb	r0!, {r1-r15}\n"			// restore regesters
			"mov r0, #1\n" // return one
			"bx lr\n" // in save now
		 : :: "memory");
	}
	inline void raise_context_switch() { *((volatile uint32_t*)0xE000ED04) |= 0x10000000; }

	tailq::prio_head<callo,&callo::link> callo::_callouts;
	void callo::timeout(callo* call, timeval_t t){
		//auto p1 = _callouts.begin();
		auto s = irq::splhigh();
		call->time = t;
		_callouts.push(call);
		irq::splx(s);
	}
   void callo::clock(const timeval_t& delta){
	   for(auto it=_callouts.begin(); it != _callouts.end(); ){
		   if(it->time <= delta) {
			   callo* call = &(*it);
			   it = _callouts.erase(it); // remove it
			   call->action(); // do it
		   } else {
			  it->time -= delta;
			  ++it;
		   }
	   }
   }
   proc::proc(){
	   _state = proc_state_t::zombie;

   }
   proc::proc(stack_item_t * StackPoolEnd , void (*exec)()){
	    init_stack_frame(StackPoolEnd, exec);
	   kernel::register_process(this);
	   _pid = kernel::new_pid();
	   _state = proc_state_t::running;
	  // _flag = 0;
	   _uid =0;
	   _pgrp =0;
	   _nice = 0;
   }
   void kernel::register_process(proc* p){
	   _proc.push_front(p);
   }

       void  proc::init_stack_frame(stack_item_t * Stack , void (*exec)()){
    	   irq::init_stack_frame(Stack,exec);
       }

	   stack_item_t* kernel::context_switch_hook(stack_item_t* sp) {
		   _curproc->_stack = sp;
		   sp = _schedproc->_stack;
		   _curproc = _schedproc;
		   _schedproc = nullptr; // tell the switch we are done
		   return sp;
	   }

	   static bool swapflag = false;
	   int kernel::swtch_proc()
	   		{
	   			proc *p;
	   			uint32_t n;

	   		loop:
	   			p = nullptr;
	   			n = 128;
	   			/*
	   			 * Search for highest-priority runnable process
	   			 */
	   			if(_runrun!=0 && !swapflag) {
	   				for(auto& rp : _proc) {
	   					if(&rp != p && rp._state == proc_state_t::running) {
	   						p = &rp;
	   						n = rp._pri;
	   						_runrun = 0;
	   						break;
	   					}
	   				}
	   			} else {
	   				if(_curproc->_state == proc_state_t::running) {
	   					p = _curproc;
	   					n = p->_pri;
	   				}
	   			}
	   			/*
	   			 * If no process is runnable, idle.
	   			 */
	   			if(p == nullptr) {
	   				idle();
	   				goto loop;
	   			}
	   			_curpri = n;
	   		/*
	   			if(hp > &hbuf[36])
	   				hp = hbuf;
	   			*hp++ = n;
	   			*hp++ = u.u_procp;
	   			*hp++ = time[1];
	   		*/
	   			if(p != _curproc && !swapflag) {
	   				// we do the swaping here
	   				if(p->_state ==  proc_state_t::zombie)
	   					panic("We don't support swap from disk yet!\r\n");
	   				_schedproc = p;
	   				raise_context_switch();
	   				while(_schedproc != p);// until context switch done, not sure we need this
	   #if 0
	   			/*
	   			 * Save stack of current user and
	   			 * and use system stack.
	   			 * and swap out of memory if need be
	   			 */
	   				swapflag++;
	   				savu(u.u_ssav);
	   				retu(u.u_rsav);
	   				u.u_procp->p_flag =& ~SLOAD;
	   				if(u.u_procp->p_stat != SZOMB)
	   					swap(u.u_procp, B_WRITE);
	   				swap(rp, B_READ);
	   				rp->p_flag =| SLOAD;
	   				retu(u.u_ssav);
	   				swapflag--;
	   			}
	   #endif
	   			/*
	   			 * The value returned here has many subtle implications.
	   			 * See the newproc comments.
	   			 */

	   		}
	   	   return (1);
}; //

};  /* namespace os */



