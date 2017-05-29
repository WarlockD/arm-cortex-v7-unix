
#if 0
#include "proc.hpp"


namespace mimx {

	static inline pid_t next_pid() {
		static pid_t g_pids = 1000;
		if(g_pids == 50000) g_pids = 1000;
		return g_pids;
	}
	kernel g_kernel;
	static constexpr size_t MAX_TCB_THREADS = 128;
	struct heap_compare {
		bool operator()(const tcb_t* a, const tcb_t* b){
			return a == nullptr ? true : b == nullptr ? false : a->t_globalid < b->t_globalid;
		}
	};


	tcb_t *kernel::thread_fork(){
		pid_t npid = next_pid();
		tcb_t * thr = nullptr;
		while((thr = thread_map_search(npid)) != nullptr) npid = next_pid();
		thr = new fork_task(npid, _caller);

		thread_map_add(thr);

		thr->t_parent = _caller;

		thr->t_globalid = npid;
		/* Place under */
		if (_caller->t_child) {
			tcb_t *t = _caller->t_child;

			while (t->t_sibling != 0)
				t = t->t_sibling;
			t->t_sibling = thr;

			//thr->t_localid = t->t_localid + (1 << 6);
		} else {
			/* That is first thread in child chain */
			_caller->t_child = thr;

			//thr->t_localid = (1 << 6);
		}
		_caller->ctx[REG::R0] = npid;
		thr->ctx[REG::R0] = 0;
		thr->state = T_RUNNABLE;
		ready(thr);
		return thr;
	}

	void kernel::syscall_handler() {
		uint32_t *svc_param1 = (uint32_t *) caller->ctx.sp;
		uint32_t svc_num = ((char *) svc_param1[REG_PC])[-2];
		uint32_t *svc_param2 = caller->ctx.regs;

		if (svc_num == SYS_THREAD_CONTROL) {
			// test vfork
			g_kernel.thread_fork();
		}
	}


	void __svc_handler(void)
	{
		extern tcb_t *kernel;
		/* Kernel requests context switch, satisfy it */
		if(g_kernel._current == &g_kernel._kernel_thread) return;
		g_kernel._caller = g_kernel._current;
		g_kernel._caller->state = T_SVC_BLOCKED;
		g_kernel.schedule(SYSCALL_SOFTIRQ);
		request_schedule();
	}
	__attribute__((naked))  void kernel::context_switch(tcb_t* from, tcb_t*  to){

	}
	__attribute__((naked))  void kernel::schedule_in_irq(){
		__asm__ __volatile__ ("push {lr}");
		register tcb_t *sel = schedule_select();
		__asm__ __volatile__ ("pop {lr}");				\
		__asm__ __volatile__ ("bx lr");
	}
};

#define irq_enter()							\


#define irq_return()							\


#define context_switch(from, to)					\
	{								\
		__asm__ __volatile__ ("pop {lr}");			\
		irq_save(&(from)->ctx);					\
		thread_switch((to));					\
		irq_restore(&(from)->ctx);				\
		__asm__ __volatile__ ("bx lr");				\
	}

#define schedule_in_irq()						\
	{								\
		register tcb_t *sel;					\
		sel = schedule_select();				\
		if (sel != current)					\
			context_switch(current, sel);			\
	}

#define request_schedule()						\
	do { *SCB_ICSR |= SCB_ICSR_PENDSVSET; } while (0)


extern "C" __attribute__((naked)) void PendSV_Handler()
extern "C" __attribute__((naked)) void PendSV_Handler()
{
	irq_enter();
	schedule_in_irq();
	irq_return();
}
#endif
