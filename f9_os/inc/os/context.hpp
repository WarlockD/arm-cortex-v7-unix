/*
 * context.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_CONTEXT_HPP_
#define OS_CONTEXT_HPP_
#include <os/slist.hpp>
#include <cstdint>
#include <cstddef>

#include "types.hpp"

namespace os {

	class as_t;
	static constexpr size_t CONFIG_INTR_THREAD_MAX = 32;

	 enum class thread_tag_t {
		idle,
		kernel,
		root,
		interrupt,
		irq_request,
		log,
		sys	= 16,				/* Systembase */
		user	= CONFIG_INTR_THREAD_MAX	/* Userbase */
	} ;

	 enum class thread_state_t {
		inactive,
		runnable,
		svc_blocked,
		recv_blocked,
		send_blocked
	} ;
	 class ThreadContext {
	 public:
		 void ThreadContext::prepareSwitch(void){


		     os::scheduler.prepareContextSwitchNoInterrupts();

		     // update the context cache, where to save the stack pointer
		     // for the next PendSV
		     ms_ppStack = os::scheduler.getCurrentThread()->getContext().getPPStack();
		     // ----- end of critical section ----------------------------------------
		}
	      void create(uint32_t* pStackBottom,
	          size_t stackSizeBytes,
	          uint32_t trampolineEntryPoint, void* p1, void* p2,void* p3);
	      /// \brief Save the current context in the local storage.
	      ///
	      /// \par Parameters
	      ///    None.
	      /// \retval true  Context saved, returns for the first time
	      /// \retval false Context restored, returns for the second time
	      ///    Nothing.
	      bool save();
	      void restore();

	      ThreadContext switch_to() const; // switches to this context, returning the previous context
	      uint32_t** getPPStack(void) { return &_stack; }
	      ThreadContext(void);
	      ~ThreadContext(void);
	 private:
	      uint32_t* _stack;
	 };
	 struct context_t{
		uint32_t sp;
		uint32_t ret;
		uint32_t ctl;
		uint32_t regs[8];
	 #ifdef CONFIG_FPU
		/* lazy fpu */
		uint32_t fp_regs[16];
		uint32_t fp_flag;
	 #endif

	 } ;
	 class tcb_t {
		l4_thread_t t_globalid;
		l4_thread_t t_localid;

		thread_state_t state;

		memptr_t stack_base;
		size_t stack_size;

		context_t ctx;

		as_t *as;
		struct utcb *utcb;

		l4_thread_t ipc_from;

		tcb_t *t_sibling;
		tcb_t *t_parent;
		tcb_t *t_child;

		uint32_t timeout_event;
	 public:
		static tcb_t *thread_by_globalid(l4_thread_t globalid);
		tcb_t *thread_init(l4_thread_t globalid, l4::utcb_t *utcb);
		tcb_t *thread_create(l4_thread_t globalid, l4::utcb_t *utcb);
	 };

	template<typename T>
	struct thread_declare {
		const char* name;
		constexpr void operator()() {
			register void *kip_ptr asm ("r0");
			register void *utcb_ptr asm ("r1");
			T(kip_ptr, utcb_ptr);
			while (1);
		}
		thread_declare(const char* name) : name(name) {}

	} __attribute__ ((section(".user_text")));

};
#endif

