/*
 * proc.hpp
 *
 *  Created on: May 11, 2017
 *      Author: Paul
 */

#ifndef MIMIX_CPP_PROC_HPP_
#define MIMIX_CPP_PROC_HPP_

#include "types.hpp"
#include <os\list.hpp>
#include <os\tailq.hpp>
#include <signal.h>
#include <os\bitmap.hpp>
#include <os\printk.hpp>

#include <stm32f7xx.h>


namespace mimx {

	static constexpr size_t HZ	          =60;	/* clock freq (software settable on IBM-PC) */
	static constexpr size_t BLOCK_SIZE     = 1024;	/* # bytes in a disk block */
	static constexpr pid_t SUPER_USER  =0;	/* uid of superuser */

	static constexpr dev_t MAJOR	           =8;	/* major device = (dev>>MAJOR) & 0377 */
	static constexpr dev_t MINOR	           =0;	/* minor device = (dev>>MINOR) & 0377 */

	static constexpr size_t NR_REGS			   =18; // 16 regs?  enough for now
//	static constexpr size_t NR_TASKS           =8;	/* number of tasks in the transfer vector */
//	static constexpr size_t NR_PROCS          =16;	/* number of slots in proc table */
	static constexpr size_t NR_SEGS            =3;	/* # segments per process */
	static constexpr size_t T                  =0;	/* proc[i].mem_map[T] is for text */
	static constexpr size_t D                  =1;	/* proc[i].mem_map[D] is for data */
	static constexpr size_t S                  =2;	/* proc[i].mem_map[S] is for stack */
	/* Process numbers of some important processes */
	static constexpr pid_t MM_PROC_NR         =0;	/* process number of memory manager */
	static constexpr pid_t FS_PROC_NR         =1;	/* process number of file system */
	static constexpr pid_t INIT_PROC_NR       =2;	/* init -- the process that goes multiuser */
	static constexpr pid_t LOW_USER           =2;	/* first user not part of operating system */

	const auto& panic = ::kpanic;;
	enum sys_function {
			SEND		   =1,	/* function code for sending messages */
			RECEIVE		   =2,	/* function code for receiving messages */
			BOTH		   =3,	/* function code for SEND + RECEIVE */
			CREATE_TASK    =4
		};
		static constexpr pid_t ANY   =		0x7ace;	/* a magic, invalid process number. */
#if 0
struct proc_hasher {
	list::int_hasher<pid_t> hasher;
	size_t operator()(const proc& p) { return hasher(p._nr); }
	size_t operator()(const pid_t pid){ return hasher(pid); }
};
struct proc_equals {
	list::int_hasher<pid_t> hasher;
	size_t operator()(const proc& p,const pid_t pid) { return p._nr == pid; }
	size_t operator()(const proc& l,const proc& r){ return l._nr == r._nr; }
};
using hash_list_t = list::hash<proc,&proc::_hash_link, 32, proc_hasher,proc_equals >;
#endif

	typedef void (*simple_callback)(void* data);
	using vir_bytes = uintptr_t;
	using vir_clicks = uintptr_t;
	using phys_clicks = uintptr_t;
	struct pc_psw {
		int (*pc)();			// storage for program counter
		phys_clicks cs;			// code segment register address?
		unsigned psw;			// program status word */
	};
	struct mem_map {
		vir_clicks mem_vir;		/* virtual address */
		phys_clicks mem_phys;		/* physical address */
		vir_clicks mem_len;		/* length */
	};
	/* This struct is used to build data structure pushed by kernel upon signal. */
	struct sig_info {
	  int signo;			/* sig number at end of stack */
	  struct pc_psw sigpcpsw;
	};
	// message, humm need to change this somehow


	/* Types relating to messages. */
	//#define M1                 1
	//#define M3                 3
	//#define M4                 4
	#define M3_STRING         14

	struct mess_1 {int m1i1, m1i2, m1i3; char *m1p1, *m1p2, *m1p3;} ;
	struct mess_2 {int m2i1, m2i2, m2i3; long m2l1, m2l2; char *m2p1;} ;
	struct mess_3 {int m3i1, m3i2; char *m3p1; char m3ca1[M3_STRING];} ;
	struct mess_4 {long m4l1, m4l2, m4l3, m4l4;} ;
	struct mess_5 {char m5c1, m5c2; int m5i1, m5i2; long m5l1, m5l2, m5l3;} ;
	struct mess_6 {int m6i1, m6i2, m6i3; long m6l1; int (*m6f1)();} ;


#if 0
	struct message {
	  int m_source;			/* who sent the message */
	  int m_type;			/* what kind of message is it */
	  union {
		mess_1 m_m1;
		mess_2 m_m2;
		mess_3 m_m3;
		mess_4 m_m4;
		mess_5 m_m5;
		mess_6 m_m6;
	  } m_u;
	  message&& copy(int source) {
		  message m = *this;
		  m.m_source = source;
		  return std::move(m);
	  }
	} __attribute__((aligned(4))) ;
	static constexpr size_t MESS_SIZE  = sizeof(message);	/* might need usizeof from fs here */
	static constexpr  message* NIL_MESS = nullptr;
#endif


#if 0
	enum class PSTATE {
		UNBLOCKED			 =0,
					/* Bits for p_flags in proc[].  A process is runnable iff p_flags == 0 */
		NO_MAP		=0x01,	/* keeps unmapped forked child from running */
		SENDING		=0x02,	/* set when process blocked trying to send */
		RECEIVING	=0x04,	/* set when process blocked trying to recv */
		PENDING		=0x08,	/* set when inform() of signal pending */
		SIG_PENDING	=0x10,	/* keeps to-be-signalled proc from running */
		P_STOP		=0x20,	/* set when process is being traced */
	};

	/* Values for p_priority */
	enum class PPRI {
		NONE	=0,	/* Slot is not in use */
		TASK	=1,	/* Part of the kernel */
		SERVER	=2,	/* System process outside the kernel */
		USER	=3,	/* User process */
		IDLE	=4,	/* Idle process */
	};
	enum thread_state_t {
		T_INACTIVE,
		T_RUNNABLE,
		T_WAITING,
		T_SVC_BLOCKED,
		T_RECV_BLOCKED,
		T_SEND_BLOCKED
	} ;
	enum thread_tag_t {
		THREAD_IDLE,
		THREAD_KERNEL,
		THREAD_ROOT,
		THREAD_INTERRUPT,
		THREAD_IRQ_REQUEST,
		THREAD_LOG,
		THREAD_SYS	= 16,				/* Systembase */
		THREAD_USER	= 100	/* Userbase */
	} ;
	/* System calls. */



	struct timer;
	typedef void (*tmr_func_t)(struct timer *tp);
	typedef union { int ta_int; void *ta_ptr; } tmr_arg_t;
	struct timer_t
	{
		timer_t	*tmr_next;	/* next in a timer chain */
	  int		tmr_task;	/* task this timer belongs to */
	  clock_t 	tmr_exp_time;	/* expiry time */
	  tmr_func_t	tmr_func;	/* function to call when expired */
	  tmr_arg_t	tmr_arg;	/* random argument */
	} ;

	#define TMR_NEVER LONG_MAX	/* timer not active. */

	// this is basicly a function handler that is run during the kernel thread
	// then we do the task swap if need




	enum softirq_type_t {
		KTE_SOFTIRQ,		/* Kernel timer event */
		ASYNC_SOFTIRQ,		/* Asynchronius event */
		SYSCALL_SOFTIRQ,

	#ifdef CONFIG_KDB
		KDB_SOFTIRQ,		/* KDB should have least priority */
	#endif

		NR_SOFTIRQ
	} ;
#if 0
	static constexpr const  char *softirq_names[static_cast<uint32_t>(NR_SOFTIRQ)] __attribute__((used)) = {
		"Kernel timer events",
		"Asynchronous events",
		"System calls",
	#ifdef CONFIG_KDB
		"KDB enters",
	#endif
	};
#endif
	typedef void (*softirq_handler_t)(void);

	typedef enum {
		SYS_KERNEL_INTERFACE,		/* Not used, KIP is mapped */
		SYS_EXCHANGE_REGISTERS,
		SYS_THREAD_CONTROL,
		SYS_SYSTEM_CLOCK,
		SYS_THREAD_SWITCH,
		SYS_SCHEDULE,
		SYS_IPC,
		SYS_LIPC,
		SYS_UNMAP,
		SYS_SPACE_CONTROL,
		SYS_PROCESSOR_CONTROL,
		SYS_MEMORY_CONTROL,
	} syscall_t;


	static inline void request_schedule(){ SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; }
	class tcb_t {
		enum stack_type_t {
			ST_STATIC =0,		// tcb_t is allocated outside of the stack
			ST_MALLOC,			// tcb_t was made with create
			ST_STACK			// tcb_t struc is below the stack that we don't control
		};
	public:
		friend void __svc_handler(void);
		static constexpr tcb_t * MAGIC_MARK = reinterpret_cast<tcb_t*>(0x97654321);
		tailq::entry<tcb_t> t_link;		// link in the running queue
		using queue_type = tailq::head<tcb_t, &tcb_t::t_link>;
		queue_type* t_queue; // we know what queue we are in
		void remove_from_queue() {
			if(t_queue != nullptr) {
				t_queue->remove(this);
				t_queue = nullptr;
			}
		}
		void add_to_queue(queue_type& q) {
			if(t_queue != nullptr) {
				if(t_queue == &q) return; // its already in there
				t_queue->remove(this); // remove from exisiting queue
			}
			q.push(this);
			t_queue = &q;
		}

		list::entry<tcb_t> t_hash;		// link in the look up que

		bool isrunnable() const { return state == T_RUNNABLE; }

		thread_state_t state=thread_state_t::T_INACTIVE;

		pid_t t_globalid=0;
		uintptr_t stack_base;
		size_t stack_size;
		f9_context_t ctx;
		tcb_t *t_sibling=nullptr;
		tcb_t *t_parent=nullptr;
		tcb_t *t_child=nullptr;
		void* t_chan = nullptr;
		//as_t *as;
	//	struct utcb *utcb;

		pid_t ipc_from=0;



		uint32_t timeout_event =0;
		// this copys the context to here
		// hopeful the parrents are previously setup
		void fork_copy(const tcb_t& copy) {
			// we just copy the context and stack to here
			assert(stack_size >= copy.stack_size);
			uint32_t offset = stack_size - copy.stack_size;
			uint8_t* stack = reinterpret_cast<uint8_t*>(ctx.sp)+offset;
			const uint8_t* copy_stack = reinterpret_cast<uint8_t*>(ctx.sp);
			std::copy(copy_stack,copy_stack+copy.stack_size,stack);
			ctx = copy.ctx; // copy the context
			ctx.sp = reinterpret_cast<uint32_t>(stack); // setup the new return
		}
		struct hasher {
			list::int_hasher<pid_t> int_hasher;
			constexpr size_t operator()(const pid_t t) const {
				return int_hasher(t);
			}
			constexpr size_t operator()(const tcb_t& t) const {
				return int_hasher(t.t_globalid);
			}
		};
		struct equaler {
			constexpr size_t operator()(const tcb_t& a,const tcb_t& b) const {
				return a.t_globalid == a.t_globalid;
			}
			constexpr size_t operator()(const tcb_t& t, pid_t pid) const {
				return t.t_globalid == pid;
			}
		};
		using hash_type = list::hash<tcb_t,&tcb_t::t_hash, 16, hasher,equaler>;



		friend queue_type;
		friend class kerenel;

		tcb_t *thread_select(){
			tcb_t *thr = t_child;
			if (thr == nullptr) return nullptr;

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
					if (thr->t_parent == this)
						return nullptr;
					thr = thr->t_parent;
				} while (thr->t_sibling == nullptr);

				thr = thr->t_sibling;
			}
		}
		template<typename PC, typename AT>
		void init(PC pc, AT stack, size_t stack_size_) {
			uint32_t base = ptr_to_int(stack);
			stack_base= base;
			stack_size= stack_size_;
			ctx.init(base+stack_size_, pc);
			t_link.tqe_next = MAGIC_MARK;
		}
		template<typename PC, typename AT>
		tcb_t(PC pc, AT stack, size_t stack_size) :
			t_queue(nullptr),
			stack_base(ptr_to_int(stack)),
			stack_size(stack_size),
			ctx(ptr_to_int(stack)+stack_size, pc){
			t_link.tqe_next = MAGIC_MARK;
		}
		template<typename PC, typename AT, typename ARGT>
		tcb_t(PC pc, AT stack, size_t stack_size, ARGT arg) :
			t_queue(nullptr),
			stack_base(ptr_to_int(stack)),
			stack_size(stack_size),
			ctx(ptr_to_int(stack)+stack_size, pc,arg){
			t_link.tqe_next = MAGIC_MARK;
		}
		template<typename PC, typename AT>
		static tcb_t* create(PC pc, AT stack, size_t stack_size) {
			uint32_t base = ptr_to_int(stack);
			uint32_t after_tcb = base + stack_size - sizeof(tcb_t);
			tcb_t* ptr = new(reinterpret_cast <uint8_t*>(after_tcb)) tcb_t();
			assert(ptr);
			ptr->stack_base = base;
			ptr->stack_base = stack_size - sizeof(tcb_t);
			ptr->ctx.init(pc,after_tcb-sizeof(uint32_t));
			return ptr;
		}
		template<typename PC, typename AT, typename ARGT>
		static tcb_t* create(PC pc, AT stack, size_t stack_size,ARGT arg) {
			tcb_t* ptr =_create(pc,stack,stack_size);
			ptr->ctx[REG::R0] = ptr_to_int(arg);
			return ptr;
		}
	protected:
		tcb_t() : t_queue(nullptr),stack_base(0),stack_size(0) {}
	} __attribute__((aligned(8)));

	class fork_task : public tcb_t {
		uint8_t* _stack;
	public:
		fork_task(pid_t pid, const tcb_t& copy) {
			_stack = new uint8_t[copy.stack_size];
			t_globalid = pid;
			fork_copy(copy);
		}
		fork_task(const fork_task& copy){
			_stack = new uint8_t[copy.stack_size];
			fork_copy(copy);
		}
		fork_task(fork_task&& move){
			_stack = move._stack;
			ctx = move.ctx;
			move._stack = nullptr;
		}
		~fork_task() { if(_stack) {delete[] _stack; _stack = nullptr; } }
	};

	class ktimer {
		static void execute();
		void recalc(uint32_t delta);
	protected:

		uint32_t _delta;
		virtual uint32_t handler()  = 0 ; // return delta
		friend class softirq_ktimers_t;
		friend class kernel;
		ktimer();
	public:

		tailq::entry<ktimer> _link;
		virtual ~ktimer() {}
		void schedule(uint32_t ticks);
		void unschedule();
	};

	template<size_t _STACK_SIZE>
	class ktask : public tcb_t {
	protected:
		static constexpr size_t STACK_BYTE_SIZE = _STACK_SIZE;
		static constexpr size_t STACK_WORD_SIZE = _STACK_SIZE/sizeof(uint32_t);
		uint32_t _fixed_stack[STACK_WORD_SIZE];

		//virtual void init() = 0;	// run at start
		static void exec_hack(ktask* t) {
			t->exec();
		}
		virtual void exec() = 0;	// thread, never exits
	public:
		//ktask() : tcb_t(&ktask::exec, &_fixed_stack[0],  _STACK_SIZE,  this) {}
		ktask() : tcb_t(&ktask::exec_hack, &_fixed_stack[0],  _STACK_SIZE,  this) {}
		virtual ~ktask(){}
	};
	static inline void irq_svc() { __asm__ __volatile__ ("svc #0"); } // switches threadS?
	static inline void wait_for_interrupt() { __asm__ __volatile__ ("wfi"); } // switches threadS?
	static constexpr size_t KERNEL_STACK_SIZE = 1024;

	void set_kernel_state(thread_state_t state);

	class sched {
	public:
		struct sched_slot;
		typedef tcb_t *(*sched_handler_t)(sched_slot *slot);
		struct sched_slot {
			sched_handler_t handler = nullptr;
			tcb_t* scheduled = nullptr;
		};
		enum sched_slot_id_t {
			SSI_SOFTIRQ,			/* Kernel thread */
			SSI_INTR_THREAD,
			SSI_ROOT_THREAD,
			SSI_IPC_THREAD,
			SSI_NORMAL_THREAD,
			SSI_IDLE,

			NUM_SCHED_SLOTS
		} ;

		tcb_t *schedule_select()
		{
			/* For each scheduler slot try to dispatch thread from it */
			for (uint32_t slot_id = 0; slot_id < static_cast<uint32_t>(NUM_SCHED_SLOTS); ++slot_id) {
				tcb_t *scheduled = _slots[slot_id].scheduled;

				if (scheduled && scheduled->isrunnable()) {
					/* Found thread, try to dispatch it */
					return scheduled;
				} else if (_slots[slot_id].handler) {
					/* No appropriate thread found (i.e. timeslice
					 * exhausted, no softirqs in kernel),
					 * try to redispatch another thread
					 */
					scheduled = _slots[slot_id].handler(&_slots[slot_id]);

					if (scheduled) {
						_slots[slot_id].scheduled = scheduled;
						return scheduled;
					}
				}
			}

			/* not reached (last slot is IDLE which is always runnable) */
			panic("Reached end of schedule()\n");
			return nullptr;
		}

		void dispatch(sched_slot_id_t slot_id, tcb_t *thread)
			{ _slots[static_cast<uint32_t>(slot_id)].scheduled = thread; }
		void handler(sched_slot_id_t slot_id, sched_handler_t handler)
			{ _slots[static_cast<uint32_t>(slot_id)].handler = handler; }
	private:

		sched_slot _slots[NUM_SCHED_SLOTS];
	};

	template<size_t _MAXTHREADS>
		struct thread_map_t {
			static constexpr size_t MAX_THREADS = _MAXTHREADS;
			std::array<tcb_t*, _MAXTHREADS> thread_map;
			size_t thread_count = 0;
			/*
			 * Return upper_bound using binary search
			 */
			int search(pid_t tid, int from, int to)
			{
				/* Upper bound if beginning of array */
				if (to == from || thread_map[from]->t_globalid >= tid)
					return from;

				while (from <= to) {
					if ((to - from) <= 1)
						return to;

					int mid = from + (to - from) / 2;

					if (thread_map[mid]->t_globalid > tid)
						to = mid;
					else if (thread_map[mid]->t_globalid < tid)
						from = mid;
					else
						return mid;
				}
				/* not reached */
				return -1;
			}
			void insert(pid_t tid, tcb_t *thr)
			{
				if (thread_count == 0) {
					thread_map[thread_count++] = thr;
				} else {
					int i = search(tid, 0, thread_count);
					int j = thread_count;
					for (; j > i; --j) thread_map[j] = thread_map[j - 1];
					thread_map[i] = thr;
					++thread_count;
				}
			}

			void remove(pid_t tid)
			{
				if (thread_count == 1) {
					thread_count = 0;
				} else {
					int i = search(tid, 0, thread_count);
					--thread_count;
					for (; i < thread_count; i++)
						thread_map[i] = thread_map[i + 1];
				}
			}
		};

	class kernel  : tcb_t {
		tcb_t::hash_type g_table;
		tcb_t::queue_type q_running_queue;
		tcb_t::queue_type q_waiting_queue;
		thread_map_t _thread_map;
		void thread_map_add(tcb_t* thr) {
			g_table.insert(thr);
		}
		void thread_map_remove(tcb_t* thr) {
			g_table.remove(thr);
		}
		tcb_t* thread_map_search(pid_t pid) {
			return g_table.search(pid);
		}
		tcb_t* _current=nullptr;
		tcb_t* _root=nullptr;
		tcb_t* _caller = nullptr;
		pid_t _last_user_pid = thread_tag_t::THREAD_USER;
		sched _scheduler;

		class kernel_thread_t : public ktask<KERNEL_STACK_SIZE> {
		private:
			struct  softirq_t{
				uint32_t 		  schedule;
				softirq_handler_t handler;
			};
			softirq_t _softirq[static_cast<uint32_t>(NR_SOFTIRQ)];
			softirq_t& get(softirq_type_t type) { return _softirq[static_cast<uint32_t>(type)]; }
			void exec() {
				uint32_t softirq_schedule = 0;
				while (1) {
				do {
					irq_simple_lock lock;
					lock.enable();
						for(auto& si : _softirq) {
							if(si.schedule && si.handler){
								si.schedule = 0;
								si.handler();
							//	dbg_printf(DL_SOFTIRQ, "SOFTIRQ: executing %s\n", softirq_names[i]);
							}
						}
						lock.disable();
						softirq_schedule=0;
						for(const auto& si : _softirq) softirq_schedule |= si.schedule;
						set_kernel_state((softirq_schedule) ? T_RUNNABLE : T_INACTIVE);
						lock.enable();
					} while(softirq_schedule);
					irq_svc();
				}
			}
		public:
			kernel_thread_t() {}
			void register_handler(softirq_type_t type, softirq_handler_t handler)
				{ get(type).handler = handler; get(type).schedule = 0; }
			void schedule(softirq_type_t type) { get(type).schedule = 1; state = T_RUNNABLE; }

		};
		class idle_thread_t : public ktask<KERNEL_STACK_SIZE> {
		private:
			void exec() {
				while (1)
			#ifdef CONFIG_KTIMER_TICKLESS
					ktimer_enter_tickless();
			#else
					wait_for_interrupt();
			#endif /* CONFIG_KTIMER_TICKLESS */
			}
		};
		kernel_thread_t _kernel_thread;
		idle_thread_t _idle_thread;
		tcb_t* schedule_select() {
			if(q_running_queue.empty()) return &_idle_thread;
			else{
				tcb_t*  sel =q_running_queue.pop();
				q_running_queue.push(sel); // push to back
				return sel;
			}
		}
		void thread_switch(tcb_t *thr){
			_current = thr;
		}
		int schedule()
		{
			tcb_t *scheduled = _scheduler.schedule_select();
			thread_switch(scheduled);
			return 1;
		}
		void __svc_handler();
		tcb_t * global_search(pid_t pid)  { return g_table.search(pid); } // find by pid
		void global_insert(tcb_t* t) { g_table.insert(*t); }
		void global_remove(tcb_t* t) { g_table.remove(*t); }
		void ready(tcb_t* t){
			assert(t);
			irq_simple_lock lock;
			if(t->state == thread_state_t::T_INACTIVE){
				q_running_queue.push_back(t);
				t->state = thread_state_t::T_RUNNABLE;
			}
		}
		void unready(tcb_t* t){
			assert(t);
			irq_simple_lock lock;
			if(t->state == thread_state_t::T_RUNNABLE){
				q_running_queue.remove(t);
				t->state = thread_state_t::T_INACTIVE;
			}
		}
		void wait(void* chan) {
			assert(_current->state == thread_state_t::T_RUNNABLE);
			_current->state = thread_state_t::T_WAITING;
			_current->t_chan = chan;
			q_running_queue.remove(_current);
			q_waiting_queue.push_back(_current);
			schedule();
			request_schedual();

			unready(_current);
					q_waiting_queue
		}
		tcb_t *kernel::thread_fork();

	public:
		static __attribute__((naked))   void schedule_in_irq();
		static __attribute__((naked))  void  context_switch(tcb_t* from, tcb_t*  to);
		void startup_kernel(void (*root_thread)(), uint32_t* stack, size_t stack_size) {
			_current = &_kernel_thread;
			ctx.init(stack,root_thread);

			//extern char _end; // get the
			__asm__ __volatile__ ("mov r0, %0" : : "r" (_kernel_thread.ctx.sp));
			__asm__ __volatile__ ("msr msp, r0");
			__asm__ __volatile__ ("mov r1, %0" : : "r" (_kernel_thread.ctx[REG::PC]));
			__asm__ __volatile__ ("cpsie i");
			__asm__ __volatile__ ("bx r1");
			//init_ctx_switch(&kernel->ctx, kernel_thread);
		}
		static void syscall_handler();
		tcb_t *thread_create();
#if 0
		kernel(void (*root_thread)(), uint32_t* stack, size_t stack_size) :
			tcb_t(root_thread,stack,stack_size),  {
			_root = this;
			_kernel_thread.t_globalid = thread_tag_t::THREAD_KERNEL;
			_idle_thread.t_globalid = thread_tag_t::THREAD_IDLE;
			t_globalid = thread_tag_t::THREAD_ROOT;
			_scheduler.dispatch(sched::SSI_IDLE, &_idle_thread);
			_scheduler.dispatch(sched::SSI_ROOT_THREAD, this);
			_scheduler.dispatch(sched::SSI_SOFTIRQ, &_kernel_thread);

			register_handler(SYSCALL_SOFTIRQ, syscall_handler);

			_idle_thread.state = thread_state_t::T_RUNNABLE;
			_kernel_thread.state = thread_state_t::T_RUNNABLE;
			state = thread_state_t::T_RUNNABLE;
#if 0
			global_insert(this);
			global_insert(&_kernel_thread);
			global_insert(&_idle_thread);

			q_waiting_queue.push(&_idle_thread);
			q_waiting_queue.push(&_kernel_thread);
			q_running_queue.push(this);
			state = thread_state_t::T_RUNNABLE;
			_idle_thread.state = thread_state_t::T_INACTIVE;
			_kernel_thread.state = thread_state_t::T_INACTIVE;
#endif

		}
#endif
		tcb_t* current() const { return _current; }
		void register_handler(softirq_type_t type, softirq_handler_t handler) {
			_kernel_thread.register_handler(type,handler);
		}
		void schedule(softirq_type_t type) {
			_kernel_thread.schedule(type);
		}
		void dispatch(sched::sched_slot_id_t slot_id, tcb_t *thread) { _scheduler.dispatch(slot_id,thread); }
		void register_handler(sched::sched_slot_id_t slot_id, sched::sched_handler_t handler) { _scheduler.handler(slot_id,handler); }
	};
	extern kernel g_kernel;
#endif
	/* System calls. */
	//#define SEND		   1	/* function code for sending messages */
	//#define RECEIVE		   2	/* function code for receiving messages */
	//#define BOTH		   3	/* function code for SEND + RECEIVE */
	//#define ANY		0x7ace	/* a magic, invalid process number.

	/* Here is the declaration of the process table.  Three assembly code routines
	 * reference fields in it.  They are restart(), save(), and csv().  When
	 * changing 'proc', be sure to change the field offsets built into the code.
	 * It contains the process' registers, memory map, accounting, and message
	 * send/receive information.
	 */

	enum MSG {
		CANCEL      = 0,	/* general req to force a task to cancel */
		HARD_INT     = 2,	/* fcn code for all hardware interrupts */
	};

	struct proc;
	// trying to make the message system overwritable
	static constexpr size_t MESSAGE_BUCKET_COUNT = 32;
	class message {
		int _type; // type can be anything, but its used for comparing
		int _source;
		void* _data;
		size_t _size;
		tailq::entry<message> 	p_link; // quueue of messages
		list::entry<message> 	p_hash; // lookup if message is used
		friend struct proc;
		friend list::tailq<message,&message::p_link>;
		friend list::hash<message,&message::p_hash,MESSAGE_BUCKET_COUNT>;
	public:
		using message_queue_t = tailq::head<message,&message::p_link>;
		message() :_type(0), _source(0),_data(nullptr), _size(0) {}
		message(int type, int source) : _type(0), _source(0),_data(nullptr), _size(0) {}
		message(int type, int source, void* data, size_t size) : _type(0), _source(0),_data(data), _size(size) {}
		message(int source, const message& copy) : _type(copy._type), _source(source),_data(copy._data), _size(copy._size) {}
		virtual ~message() {}
		message copy(int source) const {
			return message(_type,source,_data,_size);
		}
		int type() const { return _type; }
		int source() const { return _source; }
		template<typename T=void*>
		T data() const { return  reinterpret_cast<T>(_data); }
		int size() const { return _size; }
		virtual bool operator==(const message& r) const { return _type == r._type; }
		bool operator!=(const message& r) const { return !(*this==r); }
	} __attribute__((aligned(4))) ;
	using message_list_t = tailq::head<message,&message::p_link>;
	using message_hash_t = list::hash<message,&message::p_hash,MESSAGE_BUCKET_COUNT>;

	struct proc {
		static constexpr size_t NR_TASKS  = 8;
		static constexpr size_t NR_PROCS  = 16;
		static constexpr int HARDWARE     = -1;	/* used as source on interrupt generated msgs */
		static constexpr int IDLE         = -999;	/* used as source on interrupt generated msgs */
		static constexpr proc* IDLEPROC   = reinterpret_cast <proc*>(IDLE); 	/* used as source on interrupt generated msgs */
		f9_context_t ctx;
		/* Bits for p_flags in proc[].  A process is runnable iff p_flags == 0 */
		enum flag_t{
			P_SLOT_FREE   	 =001,	/* set when slot is not in use */
			NO_MAP           =002,	/* keeps unmapped forked child from running */
			SENDING          =004,	/* set when process blocked trying to send */
			RECEIVING        =010,	/* set when process blocked trying to recv */
		};
		enum queues_t {
			TASK_Q             =0,	/* ready tasks are scheduled via queue 0 */
			SERVER_Q           ,	/* ready servers are scheduled via queue 1 */
			USER_Q             ,	/* ready users are scheduled via queue 2 */
			NQ                 	/* # of scheduling queues */
		};
		enum task_numbers_t {
			MM_PROC_NR        = 0,	/* process number of memory manager */
			FS_PROC_NR        = 1,	/* process number of file system */
			INIT_PROC_NR      = 2,	/* init -- the process that goes multiuser */
		};
		static constexpr int LOW_USER          = 2;	/* first user not part of operating system */
		/* The following items pertain to the 3 scheduling queues. */

		tailq::entry<proc> p_link;
		using proc_queue_t = tailq::head<proc,&proc::p_link>;
		int p_flags;			/* P_SLOT_FREE, SENDING, RECEIVING, etc. */
		int *p_splimit;		/* lowest legal stack value */
		int p_pid;			/* process id passed in from MM */

		clock_t user_time;		/* user time in ticks */
		clock_t sys_time;		/* sys time in ticks */
		clock_t child_utime;	/* cumulative user time of children */
		clock_t child_stime;	/* cumulative sys time of children */
		clock_t p_alarm;		/* time of next alarm in ticks, or 0 */

	//	proc_queue_t p_callerq;	/* head of list of procs wishing to send */
		//proc *p_sendlink;	/* link to next proc wishing to send */
		message_list_t p_messageq;
	//	message* p_messbuf;		/* pointer to message buffer */
		int p_getfrom;		/* from whom does process want to receive? */

		proc *p_nextready;	/* pointer to next ready process */
		int p_pending;		/* bit map for pending signals 1-16 */



		static proc *proc_ptr;	/* &proc[cur_proc] */
		static proc *bill_ptr;	/* ptr to process to bill for clock ticks */
		static int prev_proc;
		static int cur_proc;
		static uint32_t busy_map;   /* list of message backups */
		static message task_mess[NR_TASKS+1];	/* ptrs to messages for busy tasks */
		static  proc_queue_t rdy_queue[NQ];	/* pointers to ready list tails */
		static proc procs[NR_TASKS+NR_PROCS];
		static message_hash_t message_lookup;
		static proc* proc_addr(int n) { return &procs[NR_TASKS+n]; }
		void ready();
		void unready();
		static void  sched();
		static int mini_send(int caller, int dest, message& m_ptr);
		static int sys_call(int function, int caller, int src_dest, message& m_ptr);
		/*===========================================================================*
		 *				interrupt				     *
		 *===========================================================================*/
		// kernel task hardware interrupt
		// need to increase the mask
		static int interrupt(int task, message& m_ptr);
	private:
		static int mini_rec(int caller, int src, message& m_ptr);

		int proc_number() const { return static_cast<int>(this - procs - NR_TASKS);	} /* task or proc number */

		proc_queue_t& proc_queue() const {
			int r = proc_number();
			return rdy_queue[(r < 0 ? TASK_Q : r < LOW_USER ? SERVER_Q : USER_Q)];
		}

		static void pick_proc();
	};
#if 0
	struct proc {

		enum prioritys_t {
			PSWP   	= 0,
			PVM     =4,
			PINOD   =8,
			PRIBIO  =16,
			PVFS    =20,
			PZERO   =22,          /* No longer magic, shouldn't be here.  XXX */
			PSOCK   =24,
			PWAIT   =32,
			PLOCK   =36,
			PPAUSE  =40,
			PUSER   =50,
			MAXPRI  =127,         /* Priorities range from 0 through MAXPRI. */
		};

		bool p_not_a_zombie=false;
		using pfixpt_t = fixpt_t<11>;
		static constexpr pfixpt_t ldavg[] = {  5.68, 10.32,14.94, 19.55 };
		static constexpr pfixpt_t ccpu = 0.95122942450071400909; //* decay 95% of `p_pctcpu' in 60 seconds; see CCPU_SHIFT before changing */

		/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
		template<typename T>
		static constexpr inline T loadfactor(T loadav)  { return 2* loadav ; }

		template<typename T>
		static constexpr inline T decay_cpu(const pfixpt_t& loadfac, T cpu) { return  static_cast<T>((loadfac * cpu) / (loadfac + pfixpt_t::ONE)); }


		/* Guard word for task stacks. */
		constexpr static reg_t STACK_GUARD	 = ((reg_t) (sizeof(reg_t) == 2 ? 0xBEEF : 0xDEADBEEF));
		static constexpr int  IDLE     =       -999;	/* 'cur_proc' = IDLE means nobody is running */
		static constexpr int  HARDWARE     =       -1;	/* 'cur_proc' = IDLE means nobody is running */

		f9_context_t ctx;
		tailq::entry<proc> 	p_link; // sorted quueue of all running process

		// priority system
	    uint32_t p_estcpu;           /* Time averaged value of p_cpticks. */
	    int 	 p_nice;			// ever used anymore?
	    int      p_cpticks;          /* Ticks of cpu time. */
	    pfixpt_t  p_pctcpu;           /* %cpu for this process during p_swtime */
	    uint32_t p_slptime;
	    uint32_t p_usrpri;
		clock_t  user_time;		/* user time in ticks */
		clock_t  sys_time;		/* sys time in ticks */
		clock_t  child_utime;		/* cumulative user time of children */
		clock_t  child_stime;		/* cumulative sys time of children */

	    void resetpriority();
		void update_priority();

		PPRI p_priority;		/* task, server, or user process */
		message::message_queue_t q_messages;
		size_t message_count; // unprocessed messages
		uint32_t real_priority;
		int priority() const { return 0; }
		pid_t p_pid;		// the pid
		int p_nr;			/* number of this process (for fast access) */

		bool p_int_blocked;		/* nonzero if int msg blocked by busy task */
		bool p_int_held;		/* nonzero if int msg held by busy syscall */
		struct proc *p_nextheld;	/* next in chain of held-up int processes */

		PSTATE p_flags;			/* SENDING, RECEIVING, etc. */
		pid_t p_pid;			/* process id passed in from MM */




		timer_t *p_exptimers;		/* list of expired timers */

		proc *p_callerq;	/* head of list of procs wishing to send */
		proc *p_sendlink;	/* link to next proc wishing to send */
		message *p_messbuf;		/* pointer to message buffer */
		size_t p_msgsize;
		int p_getfrom;		/* from whom does process want to receive? */
		int p_sendto;

		proc *p_nextready;	/* pointer to next ready process */
		sigset_t p_pending;		/* bit map for pending signals */
		unsigned p_pendcount;		/* count of pending and unfinished signals */

		char p_name[16];		/* name of the process */
		// static part, more ore less the kernel
		static proc procs[NR_TASKS+NR_PROCS];
		static proc *pproc_addr[NR_TASKS + NR_PROCS];



	    constexpr static proc* NIL_PROC = (struct proc *)nullptr;

	    /* Magic process table addresses. */
	   inline static proc* BEG_PROC_ADDR() { return &procs[0]; }
	   inline   static proc* END_PROC_ADDR()  { return &procs[NR_TASKS + NR_PROCS]; }
	   inline  static proc* END_TASK_ADDR()  { return (&procs[NR_TASKS]);}
	   inline   static proc* BEG_SERV_ADDR()  { return (&procs[NR_TASKS]);}
	   inline  static proc* BEG_USER_ADDR()  { return (&procs[NR_TASKS + LOW_USER]);}

	     inline bool  isempty() const { return p_priority == PPRI::NONE; }
	     inline bool  istask() const { return p_priority == PPRI::TASK; }
	     inline bool  isserv() const { return p_priority == PPRI::SERVER; }
	     inline bool  isuser() const { return p_priority == PPRI::USER; }

	    constexpr inline static bool isokprocn(int n) { return  ((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS); }

	    constexpr inline static bool isidlehardware(int n) { return ((n) == IDLE || (n) == HARDWARE); }
	    constexpr inline static bool isoksrc_dest(int n) { return isokprocn(n) || n == ANY; }
	    constexpr inline static bool isrxhardware(int n) { return ((n) == ANY || (n) == HARDWARE); }
	    constexpr inline static bool issysentn(int n) { return ((n) == FS_PROC_NR || (n) == MM_PROC_NR); }


		constexpr static inline proc* cproc_addr(pid_t n) { return (&(procs + NR_TASKS)[(n)]); }
	    constexpr static inline proc* proc_addr(pid_t n) { return (pproc_addr + NR_TASKS)[(n)]; }


	    inline void ready() {
	    	switching = true;
	    	_ready();
	    	switching = false;
	    }
	    inline void unready() {
	    	switching = true;
	    	_unready();
	    	switching = false;
	    }
	    int mini_send(int dest, message* m_ptr,size_t msgsize){
	    	switching = true;
	    	int result = _mini_send(dest,m_ptr,msgsize);
	    	switching = false;
	    	return result;
	    }

		  template <typename T,
		              typename = typename std::enable_if<std::is_base_of<T, message>::value
		                                              || std::is_same<T,message>::value>::type>
	    int mini_send(int dest, typename std::remove_pointer<T>::type & m_ptr){
			  return mini_send(dest,&m_ptr,sizeof(T));
	    }


	    int proc_number() const { return p_nr; }
	//public: // public interface
	    static void unhold(); // flush unhandled interrupts
	    static void lock_sched();
	    static void sched() {
	    	switching = true;
	    	_sched();
	    	switching = false;
	    }
	    static int sys_call(int function, int src_dest, message* m_ptr,size_t msg_size);


	    void inform() {} // filler for right now as we don't have signal handling yet
	//private:
	    static volatile bool switching;
	    void _ready();
	    void _unready();
	    static void _sched();
	    int _mini_send(int dest, message* m_ptr, size_t msg_size);
	    int _mini_rec(int src, message* m_ptr, size_t msg_size);


	    static void cp_mess(int src,proc* src_p,message* src_m, proc* dst_p, message* dst_m, size_t msg_size);
	    static void pick_proc();
		static void interrupt(pid_t task);
		friend stackframe_s* _syscall(int function, int src_dest, message* m_ptr, stackframe_s* stack);

	} ;
#endif

#if 0
	template<size_t _STACK_SIZE>
	class task {
	protected:
		static constexpr size_t STACK_SIZE = (_STACK_SIZE+(sizeof(uint32_t)*2)-1) /(sizeof(uint32_t)*2);
		uint8_t _stack[STACK_SIZE];
		proc* _proc;
		static void bad_exit() {
			panic("TASK EXITED!\r\n");
			while(1);
		}
		static void start(task* ptr) {
			ptr->run();
			bad_exit();
		}
		template<typename T> static inline reg_t cast_to_reg(T a) { return static_cast<reg_t>(a); }
		template<typename T> static inline reg_t cast_to_reg(T* a) { return reinterpret_cast<reg_t>(a); }

		task(int nbr) : _proc(&proc::procs[nbr]) {
			_proc->p_stack_begin = reinterpret_cast<uint32_t*>(_stack);
			_proc->p_stack_end = reinterpret_cast<uint32_t*>(_stack + STACK_SIZE);
			_proc->p_reg = reinterpret_cast<stackframe_s*>(_proc->p_stack_end) -1;
			_proc->p_reg->set_r0(cast_to_reg(this));
			_proc->p_reg->set_pc(cast_to_reg(&task::start));
			_proc->p_reg->set_lr(cast_to_reg(&task::bad_exit));
		}
		  template <typename T,
		              typename = typename std::enable_if<std::is_base_of<T, message>::value
		                                              || IsBase<Other>::value>::type>
	    pid_t send(pid_t dst, message* msg){ return _proc->send(dst,msg);   }
	    pid_t receive(pid_t src, message* msg){ return _proc->receive(src,msg);   }
	    pid_t sendrec(pid_t src_dst, message* msg) { return _proc->sendrec(src_dst,msg);   }
		virtual void run() = 0;
	public:
		virtual ~task() {}
		void dump() {
			printk("task dump: %d", _proc->p_nr);
			_proc->p_reg->dump();
		}
	};

#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
	  template <typename T,
	              typename = typename std::enable_if<std::is_base_of<T, message>::value
	                                              || std::is_same<T,message>::value>::type>
	  __attribute__( ( always_inline ) ) static inline pid_t send(pid_t dst, T& msg){
			__asm__ __volatile__ ("mov r0, %0" : : "r" (dst) : "r0");
			__asm__ __volatile__ ("mov r1, %0" : : "r" (&msg) : "r1");
			__asm__ __volatile__ ("mov r2, %0" : : "i" (sizeof(T)) : "r2");
  	__asm volatile("swi #1\nbx lr\n");
  }
	  template <typename T,
	              typename = typename std::enable_if<std::is_base_of<T, message>::value
	                                              || std::is_same<T,message>::value>::type>
	  __attribute__( ( always_inline ) ) static inline pid_t receive(pid_t src, T& msg,size_t msg_size){
		__asm__ __volatile__ ("mov r0, %0" : : "r" (src) : "r0");
		__asm__ __volatile__ ("mov r1, %0" : : "r" (&msg) : "r1");
		__asm__ __volatile__ ("mov r2, %0" : : "i" (sizeof(T)) : "r2");
		__asm volatile("swi #2\nbx lr\n");
  }
	  template <typename T,
	              typename = typename std::enable_if<std::is_base_of<T, message>::value
	                                              || std::is_same<T,message>::value>::type>
	  __attribute__( ( always_inline ) ) static inline pid_t sendrec(pid_t src_dst, T& msg,size_t msg_size){
		__asm__ __volatile__ ("mov r0, %0" : : "r" (src_dst) : "r0");
		__asm__ __volatile__ ("mov r1, %0" : : "r" (&msg) : "r1");
		__asm__ __volatile__ ("mov r2, %0" : : "i" (sizeof(T)) : "r2");
		__asm volatile("swi #3\nbx lr\n");
  }
	  __attribute__( ( always_inline ) ) static inline  pid_t fork(){
			__asm volatile("swi #0\nbx lr\n");
	  }
	  __attribute__( ( always_inline ) ) static inline  pid_t _create_task(uintptr_t pc, uint32_t* stack, size_t stack_size){
			__asm__ __volatile__ ("mov r0, %0" : : "r" (pc) : "r0");
			__asm__ __volatile__ ("mov r1, %0" : : "r" (stack) : "r1");
			__asm__ __volatile__ ("mov r2, %0" : : "r" (stack_size) : "r2");
			__asm volatile("swi #0\nbx lr\n");
	  }
	  template<typename T>
	  __attribute__( ( always_inline ) ) static inline  pid_t create_task(T pc, uint32_t* stack, size_t stack_size) {
		  return  _create_task(ptr_to_int(pc),stack,stack_size);
	  }

#pragma GCC diagnostic pop

};
#if 0
template<>
struct enable_bitmask_operators<mimx::PSTATE>{
    static const bool enable=true;
};
#endif
#endif /* MIMIX_CPP_PROC_HPP_ */
