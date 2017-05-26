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

	enum MSG {
		CANCEL      = 0,	/* general req to force a task to cancel */
		HARD_INT     = 2,	/* fcn code for all hardware interrupts */
	};

	class message {
		int _type; // type can be anything, but its used for comparing
		int _source;
		friend struct proc;
	public:
		message() :_type(0), _source(0) {}
		message(int type, int source) : _type(0), _source(0) {}
		virtual ~message() {}
		int type() const { return _type; }
		int source() const { return _source; }
		virtual bool operator==(const message& r) const { return _type == r._type; }
		bool operator!=(const message& r) const { return !(*this==r); }
	} __attribute__((aligned(4))) ;
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

#endif

	static constexpr size_t MESS_SIZE  = sizeof(message);	/* might need usizeof from fs here */
	static constexpr  message* NIL_MESS = nullptr;

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

	enum sys_function {
		SEND		   =1,	/* function code for sending messages */
		RECEIVE		   =2,	/* function code for receiving messages */
		BOTH		   =3,	/* function code for SEND + RECEIVE */
		CREATE_TASK    =4
	};
	static constexpr pid_t ANY   =		0x7ace;	/* a magic, invalid process number. */

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
	class softirq {

		//static softirq* _head;
		//static softirq* _tail;
		//softirq* _next; // next in quueue
		friend struct proc;
		void execute();
	protected:
		softirq();
		virtual void handler() = 0;
		virtual ~softirq() {}
	public:
		tailq::entry<softirq> _link;
		void schedule();
	};
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
		tailq::entry<tcb_t> t_link;
		using queue_type = list::tailq<tcb_t, &tcb_t::t_link>;
		static queue_type q_running_queue;
		static queue_type q_waiting_queue;
		static tcb_t* _current;
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

		void thread_switch(){
			_current = this;
		}
	public:
		bool isrunnable() const { return state == T_RUNNABLE; }
		pid_t t_globalid=0;
		pid_t t_localid=0;

		thread_state_t state=thread_state_t::T_INACTIVE;

		uintptr_t stack_base;
		size_t stack_size;

		f9_context_t ctx;

		//as_t *as;
	//	struct utcb *utcb;

		pid_t ipc_from=0;

		tcb_t *t_sibling=nullptr;
		tcb_t *t_parent=nullptr;
		tcb_t *t_child=nullptr;

		uint32_t timeout_event =0

		virtual void handler() = 0;
		template<typename PC>
		tcb_t(PC pc, uint32_t* stack, size_t stack_size) : stack_base(ptr_to_int(stack)), stack_size(stack_size), ctx(pc,stack+(stack_size/4)){

		}

	};
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


	struct proc {
		bool p_not_a_zombie=false;
		/* Guard word for task stacks. */
		constexpr static reg_t STACK_GUARD	 = ((reg_t) (sizeof(reg_t) == 2 ? 0xBEEF : 0xDEADBEEF));
		static constexpr int  IDLE     =       -999;	/* 'cur_proc' = IDLE means nobody is running */
		static constexpr int  HARDWARE     =       -1;	/* 'cur_proc' = IDLE means nobody is running */
		/* The following items pertain to the 3 scheduling queues. */
		/* Scheduling priorities for p_priority. Values must start at zero (highest
		 * priority) and increment.  Priorities of the processes in the boot image
		 * can be set in table.c. IDLE must have a queue for itself, to prevent low
		 * priority user processes to run round-robin with IDLE.
		 */
		enum {
			TASK_Q            = 0,	/* ready tasks are scheduled via queue 0 */
			SERVER_Q,				/* ready servers are scheduled via queue 1 */
			USER_Q,					/* ready users are scheduled via queue 2 */
			NQ,						/* # of scheduling queues */
		};
		enum SEGS {
			T =0,	/* proc[i].mem_map[T] is for text */
			D,	/* proc[i].mem_map[D] is for data */
			S,	/* proc[i].mem_map[S] is for stack */
			NR_SEGS,	/* # segments per process */
		};
		f9_context_t ctx;
		tailq::entry<proc> p_link;

		//stackframe_s p_reg;	/* process' registers saved in stack frame */
		uint32_t* p_stack_begin; // stack memory begin
		uint32_t* p_stack_end; // stack memory end
		stackframe_s* p_reg; /* process' registers saved in stack frame */
		reg_t *p_stguard;		/* stack guard word */

		int p_nr;			/* number of this process (for fast access) */

		bool p_int_blocked;		/* nonzero if int msg blocked by busy task */
		bool p_int_held;		/* nonzero if int msg held by busy syscall */
		struct proc *p_nextheld;	/* next in chain of held-up int processes */

		PSTATE p_flags;			/* SENDING, RECEIVING, etc. */
		pid_t p_pid;			/* process id passed in from MM */
		PPRI p_priority;		/* task, server, or user process */

		clock_t user_time;		/* user time in ticks */
		clock_t sys_time;		/* sys time in ticks */
		clock_t child_utime;		/* cumulative user time of children */
		clock_t child_stime;		/* cumulative sys time of children */

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



};

template<>
struct enable_bitmask_operators<mimx::PSTATE>{
    static const bool enable=true;
};
#endif /* MIMIX_CPP_PROC_HPP_ */
