#include "thread.hpp"
#include "fpage.hpp"
#include "schd.hpp"
#include "ktimer.hpp"
#include "ipc.hpp"
#include "irq.hpp"



namespace f9 {
__attribute__((weak)) void root_thread();
__attribute__((weak)) void root_thread() { while(1); }
	DECLARE_KTABLE(tcb_t, thread_table, CONFIG_MAX_THREADS);
	//volatile tcb_t *current = nullptr; 			/* Currently on CPU */
	volatile tcb_t *current = nullptr; 			/* Currently on CPU */
	void *current_utcb __USER_DATA;
	extern tcb_t *caller;
	tcb_t *thread_map[CONFIG_MAX_THREADS];
	int thread_count;

	static tcb_t *thread_sched(sched_slot_t *);
	void thread_init_subsys()
	{
		// don't worry about kept
#if 0
		fpage_t *last = nullptr;
		ktable_init(&thread_table);

		kip.thread_info.s.system_base = THREAD_SYS;
		kip.thread_info.s.user_base = THREAD_USER;

		/* Create KIP fpages
		 * last is ignored, because kip fpages is aligned
		 */
		fpage_t::assign_ext(-1,
		                  (memptr_t) &kip, sizeof(kip_t),
		                  &kip_fpage, &last);
		fpage_t::assign_ext(-1,
		                  (memptr_t) kip_extra, CONFIG_KIP_EXTRA_SIZE,
		                  &kip_extra_fpage, &last);
#endif

		sched_slot_set_handler(SSI::NORMAL_THREAD, thread_sched);
	}
	/*
	 * Return upper_bound using binary search
	 */
	static int thread_map_search(thread_id_t globalid, int from, int to)
	{
		int tid = GLOBALID_TO_TID(globalid);

		/* Upper bound if beginning of array */
		if (to == from || (int)GLOBALID_TO_TID(thread_map[from]->t_globalid) >= tid)
			return from;

		while (from <= to) {
			if ((to - from) <= 1)
				return to;

			int mid = from + (to - from) / 2;

			if ((int)GLOBALID_TO_TID(thread_map[mid]->t_globalid) > tid)
				to = mid;
			else if ((int)GLOBALID_TO_TID(thread_map[mid]->t_globalid) < tid)
				from = mid;
			else
				return mid;
		}

		/* not reached */
		return -1;
	}
	/*
	 * Insert thread into thread map
	 */
	static void thread_map_insert(thread_id_t globalid, tcb_t *thr)
	{
		if (thread_count == 0) {
			thread_map[thread_count++] = thr;
		} else {
			int i = thread_map_search(globalid, 0, thread_count);
			int j = thread_count;

			/* Move forward
			 * Don't check if count is out of range,
			 * because we will fail on ktable_alloc
			 */

			for (; j > i; --j)
				thread_map[j] = thread_map[j - 1];

			thread_map[i] = thr;
			++thread_count;
		}
	}
	static void thread_map_delete(thread_id_t globalid)
	{
		if (thread_count == 1) {
			thread_count = 0;
		} else {
			int i = thread_map_search(globalid, 0, thread_count);
			--thread_count;
			for (; i < thread_count; i++)
				thread_map[i] = thread_map[i + 1];
		}
	}
	tcb_t::tcb_t(thread_id_t globalid, utcb_t *utcb)
	: t_globalid(globalid)
	, t_localid(0)
	, state(TSTATE::INACTIVE)
	, stack_base(0)
	, stack_size(0)
	, ctx()
#ifdef 	CONFIG_ENABLE_FPAGE
	, as(nullptr)
#endif
	, utcb(utcb)
	, ipc_from(0)
	, t_sibling(nullptr)
	, t_parent(nullptr)
	, t_child(nullptr)
	, timeout_event(0) {
		thread_map_insert(globalid, this);
		if (utcb) utcb->t_globalid = globalid;
		dbg::print(dbg::DL_THREAD, "T: New thread: %t @[%p] \n", globalid, this);
	}

	volatile tcb_t* tcb_t::current() { return f9::current; }

	tcb_t* tcb_t::create(thread_id_t globalid, utcb_t *utcb){
		int id = GLOBALID_TO_TID(globalid);
		assert(caller != nullptr);
		if (id < static_cast<int>(THREAD::SYS) ||
		    globalid == ANYTHREAD ||
		    globalid == ANYLOCALTHREAD) {
			set_caller_error(UE::TC_NOT_AVAILABLE);
			return nullptr;
		}

		tcb_t *thr = new tcb_t(globalid, utcb);
		if (!thr) {
			set_caller_error(UE::OUT_OF_MEM);
			return nullptr;
		}
		thr->t_parent = caller;

		/* Place under */
		if (caller->t_child) {
			tcb_t *t = caller->t_child;

			while (t->t_sibling != 0)
				t = t->t_sibling;
			t->t_sibling = thr;

			thr->t_localid = t->t_localid + (1 << 6);
		} else {
			/* That is first thread in child chain */
			caller->t_child = thr;

			thr->t_localid = (1 << 6);
		}

		return thr;

	}
	tcb_t::~tcb_t() {
		tcb_t *parent, *child, *prev_child;

		/* remove thr from its parent and its siblings */
		parent = this->t_parent;

		if (parent->t_child == this) {
			parent->t_child = this->t_sibling;
		} else {
			child = parent->t_child;
			while (child != this) {
				prev_child = child;
				child = child->t_sibling;
			}
			prev_child->t_sibling = child->t_sibling;
		}

		/* move thr's children to caller */
		child = this->t_child;

		if (child) {
			child->t_parent = caller;

			while (child->t_sibling) {
				child = child->t_sibling;
				child->t_parent = caller;
			}

			/* connect thr's children to caller's children */
			child->t_sibling = caller->t_child;
			caller->t_child = this->t_child;
		}


		thread_map_delete(t_globalid);
	}
	void tcb_t::free_space(){
#ifdef CONFIG_ENABLE_FPAGE
		this->as.reset();
#endif

	}
	void tcb_t::space(thread_id_t spaceid, utcb_t *utcb){
#ifdef CONFIG_ENABLE_FPAGE
		/* If spaceid == dest than create new address space
		 * else share address space between threads
		 */
		if (GLOBALID_TO_TID(this->t_globalid) == GLOBALID_TO_TID(spaceid)) {
			this->as = std::make_shared<as_t>(this->t_globalid);

			/* Grant kip_fpage & kip_ext_fpage only to new AS */
			//this->as->map(kip_fpage, GRANT);
			//this->as->map(kip_extra_fpage, GRANT);

			dbg::print(dbg::DL_THREAD,
			           "\tNew space: as: %p, utcb: %p \n", this->as, utcb);
		} else {
			tcb_t *space = thread_by_globalid(spaceid);

			this->as = space->as;
		}

		/* If no caller, than it is mapping from kernel to root thread
		 * (some special case for root_utcb)
		 */
		if (caller)
			map_area(caller->as, this->as, (memptr_t) utcb,
			         sizeof(utcb_t), GRANT, caller->ispriviliged());
		else
			map_area(this->as, this->as, (memptr_t) utcb,
			         sizeof(utcb_t), GRANT, true);
#endif
	}

	void tcb_t::_init_ctx(uintptr_t spi, uintptr_t pc, uintptr_t regsi){
		/* Reserve 8 words for fake context */
		//sp -= RESERVED_STACK;
		uint32_t* sp = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(spi)-RESERVED_STACK);
		ctx.sp = reinterpret_cast<uint32_t>(sp);
		/* Set EXC_RETURN and CONTROL for thread and create initial stack for it
		 * When thread is dispatched, on first context switch
		 */
		if (GLOBALID_TO_TID(t_globalid) >= static_cast<uint32_t>(THREAD::ROOT)) {
			ctx.ret = 0xFFFFFFFD;
			ctx.ctl = 0x3;
		} else {
			ctx.ret = 0xFFFFFFF9;
			ctx.ctl = 0x0;
		}
		//uint32_t* spa = reinterpret_cast<uint32_t*>(sp);
		if (regsi == 0) {
			 sp[REG_R0] = 0x0;
			 sp[REG_R1] = 0x0;
			 sp[REG_R2] = 0x0;
			 sp[REG_R3] = 0x0;
		} else {
			uint32_t* regs = reinterpret_cast<uint32_t*>(regsi);
			sp[REG_R0] =  regs[0];
			sp[REG_R1] =  regs[1];
			sp[REG_R2] = regs[2];
			sp[REG_R3] = regs[3];
		}
		sp[REG_R12] = 0x0;
		sp[REG_LR] = 0xFFFFFFFF;
		sp[REG_PC] = pc;
		sp[REG_xPSR] = 0x1000000; /* Thumb bit on */
	}
	void tcb_t::_init_kernel_ctx(uintptr_t spi) {
		uint32_t* sp = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(spi)-RESERVED_STACK);
		ctx.sp = reinterpret_cast<uint32_t>(sp);
		ctx.ret = 0xFFFFFFF9;
		ctx.ctl = 0x0;
	}
	/*
	 * Search thread by its global id
	 */
	tcb_t *thread_by_globalid(thread_id_t globalid)
	{
		int idx = thread_map_search(globalid, 0, thread_count);

		if (GLOBALID_TO_TID(thread_map[idx]->t_globalid)
		    != GLOBALID_TO_TID(globalid))
			return nullptr;
		return thread_map[idx];
	}

	void tcb_t::switch_to() {
		assert(isrunnable());

		f9::current = this;
		current_utcb =utcb;
#if 0
		if (current->as)
			current->setup_mpu(current->as, current->ctx.sp,
			             ((uint32_t *) current->ctx.sp)[REG_PC],
			             current->stack_base, current->stack_size);
#endif
	}
	/* Select normal thread to run
	 *
	 * NOTE: all threads are derived from root
	 */
	static tcb_t *thread_select(tcb_t *parent)
	{
		tcb_t *thr = parent->t_child;
		if (thr == NULL)
			return NULL;

		while (1) {
			if (thr->isrunnable())
				return thr;

			if (thr->t_child != nullptr) {
				thr = thr->t_child;
				continue;
			}

			if (thr->t_sibling != nullptr) {
				thr = thr->t_sibling;
				continue;
			}

			do {
				if (thr->t_parent == parent)
					return nullptr;
				thr = thr->t_parent;
			} while (thr->t_sibling == nullptr);

			thr = thr->t_sibling;
		}
	}
	// systhread
	tcb_t *idle;
	tcb_t *kernel;
	tcb_t *root;
	static tcb_t *thread_sched(sched_slot_t *slot)
	{
		return thread_select(root);
	}
	utcb_t root_utcb __KIP;

	extern void root_thread(void);
	static void kernel_thread(void);
	static void idle_thread(void)
	{
		while (1)
	#ifdef CONFIG_KTIMER_TICKLESS
			ktimer_enter_tickless();
	#else
		ARM::wfi();
			//wait_for_interrupt();
	#endif /* CONFIG_KTIMER_TICKLESS */
	}

	static void kernel_thread(void)
	{
		while (1) {
			/* If all softirqs processed, call SVC to
			 * switch context immediately
			 */
			softirq_t::execute();
			irq_svc();
		}
	}
	void tcb_t::create_root_thread(void)
	{
		root = new tcb_t(TID_TO_GLOBALID(THREAD::ROOT), &root_utcb);
#ifdef CONFIG_ENABLE_FPAGE
		root->space(TID_TO_GLOBALID(THREAD::ROOT), &root_utcb);
		root->as->map_user();
#endif

		uint32_t regs[4] ={
#if 0
			[REG_R0] = (uint32_t) &kip,
#else
			[REG_R0] = 0, // no kip
#endif
			[REG_R1] = ptr_to_int(root->utcb),
			[REG_R2] = 0,
			[REG_R3] = 0
		};

		root->init_ctx((void *) &root_stack_end, &root_thread, regs);

		root->stack_base = (memptr_t) &root_stack_start;
		root->stack_size = (uint32_t) &root_stack_end -
		                   (uint32_t) &root_stack_start;

		sched_slot_dispatch(SSI::ROOT_THREAD, root);
		root->state = TSTATE::RUNNABLE;
	}

	void tcb_t::create_kernel_thread(void){
		kernel = new tcb_t(TID_TO_GLOBALID(THREAD::KERNEL), nullptr);

		kernel->init_kernel_ctx(&kernel_stack_end);

		/* This will prevent running other threads
		 * than kernel until it will be initialized
		 */
		sched_slot_dispatch(SSI::SOFTIRQ, kernel);
		kernel->state = TSTATE::RUNNABLE;
	}
	void tcb_t::create_idle_thread(void){
		idle = new tcb_t(TID_TO_GLOBALID(THREAD::KERNEL), nullptr);
		idle->init_ctx( &idle_stack_end, idle_thread, nullptr);

		sched_slot_dispatch(SSI::IDLE, idle);
		idle->state = TSTATE::RUNNABLE;
	}
	void tcb_t::switch_to_kernel(void){
		// startup?
		create_kernel_thread();

		f9::current = kernel;
		init_ctx_switch(&kernel->ctx, kernel_thread);
	}


	void set_kernel_state(TSTATE state)
	{
		kernel->state = state;
	}


	void set_user_error(tcb_t *thread, UE error)
	{
		assert(thread && thread->utcb);

		thread->utcb->error_code = static_cast<uint32_t>(error);
	}

	void set_caller_error(UE error)
	{
		if (caller)
			set_user_error((tcb_t *) caller, error);
		else
			panic("User-level error %d during in-kernel call!", error);
	}

	void user_log(tcb_t *from)
	{
		char *format = (char *) from->ctx.regs[1];
		va_list *va = (va_list *) from->ctx.regs[2];
		dbg::print(dbg::DL_KDB, format, *va);
	}

	void tcb_t::startup(){
		printk("tcb_t::startup()\r\n");
		create_idle_thread();
		create_root_thread();
		ktimer_event_init();
		assert(ktimer_event_t::create(64, ipc_deliver, nullptr));
		switch_to_kernel();
	}

};


void PendSV_Handler(void) __NAKED;
void PendSV_Handler(void)
{
	irq_enter();
	schedule_in_irq();
	irq_return();
}
