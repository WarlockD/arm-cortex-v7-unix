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
	#define M1                 1
	#define M3                 3
	#define M4                 4
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
	/* System calls. */

	enum sys_function {
		SEND		   =1,	/* function code for sending messages */
		RECEIVE		   =2,	/* function code for receiving messages */
		BOTH		   =3,	/* function code for SEND + RECEIVE */
		ANY   =		0x7ace,	/* a magic, invalid process number. */
	};


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
		mem_map p_map[NR_SEGS];/* memory map */
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
		int p_getfrom;		/* from whom does process want to receive? */
		int p_sendto;

		proc *p_nextready;	/* pointer to next ready process */
		sigset_t p_pending;		/* bit map for pending signals */
		unsigned p_pendcount;		/* count of pending and unfinished signals */

		char p_name[16];		/* name of the process */
		// static part, more ore less the kernel
		static proc procs[NR_TASKS+NR_PROCS];
		static proc *pproc_addr[NR_TASKS + NR_PROCS];

	  	static proc *proc_ptr;	/* &proc[cur_proc] */
	  	//static proc *prev_proc_ptr; // previous process
	  	static proc *bill_ptr;	/* ptr to process to bill for clock ticks */
	  	static proc *rdy_head[NQ];	/* pointers to ready list headers */
	  	static proc *rdy_tail[NQ];	/* pointers to ready list tails */
	  	static bitops::bitmap_t<NR_TASKS+1> busy_map;		/* bit map of busy tasks */
	  	static message *task_mess[NR_TASKS+1];	/* ptrs to messages for busy tasks */


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

	    constexpr static inline  phys_bytes proc_vir2phys(proc* p, vir_bytes vir)  {
	    	return (p->p_map[D].mem_phys << CLICK_SHIFT) + vir;
	    }
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
	    int mini_send(int dest, message* m_ptr){
	    	switching = true;
	    	int result = _mini_send(dest,m_ptr);
	    	switching = false;
	    	return result;
	    }
	    __attribute__((naked))
	    static void _raw(int func, int src_dst, message* msg){
	    	__asm volatile("swi #0\n");
	    }
	    void send(int dst, message* msg){
	    	_raw(sys_function::SEND,dst,msg);
	    }
	    void receive(int src, message* msg){
	    	_raw(sys_function::RECEIVE,src,msg);
	    }
	    void sendrec(int src_dst, message* msg){
	    	_raw(sys_function::BOTH,src_dst,msg);
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
	    static int sys_call(int function, int src_dest, message* m_ptr);
	    void inform() {} // filler for right now as we don't have signal handling yet
	//private:
	    static volatile bool switching;
	    void _ready();
	    void _unready();
	    static void _sched();
	    int _mini_send(int dest, message* m_ptr);
	    int _mini_rec(int src, message* m_ptr);


	    static void cp_mess(int src,proc* src_p,message* src_m, proc* dst_p, message* dst_m);
	    static void pick_proc();
		static void interrupt(pid_t task);
		friend stackframe_s* _syscall(int function, int src_dest, message* m_ptr, stackframe_s* stack);

	} ;

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

	    void send(int dst, message* msg){ _proc->send(dst,msg);   }
	    void receive(int src, message* msg){ _proc->receive(src,msg);   }
	    void sendrec(int src_dst, message* msg) { _proc->sendrec(src_dst,msg);   }
		virtual void run() = 0;
	public:
		virtual ~task() {}
		void dump() {
			printk("task dump: %d", _proc->p_nr);
			_proc->p_reg->dump();
		}
	};







} /* namespace mimx */

template<>
struct enable_bitmask_operators<mimx::PSTATE>{
    static const bool enable=true;
};
#endif /* MIMIX_CPP_PROC_HPP_ */
