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

#include <signal.h>

namespace mimx {
	typedef void (*simple_callback)(void* data);

	struct tq_struct {
		list::entry<tq_struct> entry; /* linked list of active bh's */
		bool sync;			/* must be initialized to zero */
		void (*routine)(void *);	/* function to call */
		void *data;			/* argument to function */
		tq_struct() : sync(false),routine(nullptr),data(nullptr) {}
		tq_struct(simple_callback func, void* arg=nullptr) : sync(false),routine(func),data(arg) {}
		inline void run() {
			auto func = routine;
			auto a = data;
			sync = false;
			func(a);
		}
	};

	class tq_list {
		list::head<tq_struct,&tq_struct::entry> _head;
	public:
		tq_list() {}
		void queue_task(tq_struct& nh){
			if(!nh.sync) {
				irq_simple_lock lock;
				_head.push_front(&nh);
			}
		}
		/*
		 * queue_task_irq_off: put the bottom half handler "bh_pointer" on the list
		 * "bh_list".  You may call this function only when interrupts are off.
		 */
		void queue_task_irq_off(tq_struct& nh){
			if(!nh.sync) {
				nh.sync = true;
				_head.push_front(&nh);
			}
		}
		/*
		 * queue_task_irq: put the bottom half handler "bh_pointer" on the list
		 * "bh_list".  You may call this function only from an interrupt
		 * handler or a bottom half handler.
		 */
		void queue_task_irq(tq_struct& nh){
			if(!test_and_set(nh.sync))
				_head.push_front(&nh);
		}
		void run() {
			tq_struct*p;
			while((p = _head.first_entry())!=nullptr) {
				_head.remove(p);
				p->run();
			}
		}
		static void run_task_queue(void * f);
		static tq_list tq_immediate;
		static tq_list tq_scheduler;
		static tq_list tq_disk;
	};


	struct timer_list {
		list::entry<timer_list> _entry;
		clock_t expires;
		simple_callback func;
		void* arg;
		int slack;
		timer_list() : expires(0),func(nullptr),arg(nullptr),slack(0) {}
		timer_list(clock_t expires, simple_callback func, void* arg=nullptr) :
			expires(expires),func(func),arg(arg),slack(0) {}
	};

	struct rpc_timer {
		timer_list timer;
		list::head<timer_list,&timer_list::_entry> list;
		clock_t expires;
	};
	extern clock_t get_uptime();
/* Masks and flags for system calls. */
#define SYSCALL_FUNC	0x0F	/* mask for system call function */
#define SYSCALL_FLAGS   0xF0    /* mask for system call flags */
#define NON_BLOCKING    0x10	/* prevent blocking, return error */

/* System call numbers that are passed when trapping to the kernel. The
 * numbers are carefully defined so that it can easily be seen (based on
 * the bits that are on) which checks should be done in sys_call().
 */
	enum SYSCALL {
		SEND		  = 1,	/* 0 0 0 1 : blocking send */
		RECEIVE		  = 2,	/* 0 0 1 0 : blocking receive */
		SENDREC	 	  = 3,  	/* 0 0 1 1 : SEND + RECEIVE */
		NOTIFY		  = 4,	/* 0 1 0 0 : nonblocking notify */
		ECHO		  = 8,	/* 1 0 0 0 : echo a message */
	};
/* The following bit masks determine what checks that should be done. */
#define CHECK_PTR       0x0B	/* 1 0 1 1 : validate message buffer */
#define CHECK_DST       0x05	/* 0 1 0 1 : validate message destination */
#define CHECK_SRC       0x02	/* 0 0 1 0 : validate message source */

/*==========================================================================*
 * Types relating to messages. 						    *
 *==========================================================================*/

#define M1                 1
#define M3                 3
#define M4                 4
#define M3_STRING         14

	class message {
		int _source;
		int _type;
		bool _irq_pending;
		clock_t _timestamp;

	public:
		message(int source, int type, bool irq_pending=false) :
			_source(source), _type(type),_irq_pending(irq_pending), _timestamp(clock()) {}
		message(int source, int type,clock_t stamp, bool irq_pending=false) :
				_source(source), _type(type) ,_irq_pending(irq_pending), _timestamp(stamp){}
		int source() const { return _source; }
		int type() const { return _type; }
		const clock_t& timestamp() const { return _timestamp; }
		bool irq_pending() const { return _irq_pending; }
	};
	struct sw_regs {
		uint32_t R4;
		uint32_t R5;
		uint32_t R6;
		uint32_t R7;
		uint32_t R8;
		uint32_t R9;
		uint32_t R10;
		uint32_t R11;
	};
	struct hw_regs {
		uint32_t R0;
		uint32_t R1;
		uint32_t R2;
		uint32_t R3;
		uint32_t R12;
		uint32_t LR;
		uint32_t PC;
		uint32_t PSR;
	}__attribute__((aligned(8)));

	struct stackframe {
		struct sw_regs sw;
		struct hw_regs hw;
	};

class proc {



	static proc procs[NR_TASKS + NR_PROCS];	/* process table */
	static proc * pproc_addr[NR_TASKS + NR_PROCS];
	static proc *rdy_head[NR_SCHED_QUEUES]; /* ptrs to ready list headers */
	static proc *rdy_tail[NR_SCHED_QUEUES]; /* ptrs to ready list tails */
	static inline proc * BEG_PROC_ADDR() { return &procs[0]; }
	static inline proc * BEG_USER_ADDR() { return &procs[NR_TASKS]; }
	static inline proc * END_PROC_ADDR() { return &procs[NR_TASKS + NR_PROCS]; }

	constexpr static  pid_t ANY		=0x7ace;	/* used to indicate 'any process' */
	constexpr static  pid_t NONE 	=0x6ace;  	/* used to indicate 'no process at all' */
	constexpr static  pid_t SELF	=0x8ace; 	/* used to indicate 'own process' */

	/* Kernel tasks. These all run in the same address space. */
	constexpr static  pid_t  IDLE      =       -4;	/* runs when no one else can run */
	constexpr static  pid_t  CLOCK  	=	 -3;	/* alarms and other clock functions */
	constexpr static  pid_t  SYSTEM     =      -2;	/* request system functionality */
	constexpr static  pid_t  KERNEL     =      -1;	/* pseudo-process for IPC and scheduling */
	constexpr static  pid_t  HARDWARE   =  KERNEL;		/* for hardware interrupt handlers */

	constexpr static proc *  NIL_PROC      =    ((struct proc *) 0)	;
	constexpr static proc *  NIL_SYS_PROC      =    ((struct proc *) 1)	;
	constexpr static proc * cproc_addr(int n)   { return &(procs + NR_TASKS)[(n)]; }
	constexpr static proc * proc_addr(int n)   { return (pproc_addr + NR_TASKS)[(n)]; }


	enum runtime_flags {
		RUNNABLE = 0,

		/* Bits for the runtime flags. A process is runnable iff p_rts_flags == 0. */
		SLOT_FREE	=0x01,	/* process slot is free */
		NO_MAP		=0x02,	/* keeps unmapped forked child from running */
		SENDING		=0x04,	/* process blocked trying to SEND */
		RECEIVING	=0x08,	/* process blocked trying to RECEIVE */
		SIGNALED	=0x10,	/* set when new kernel signal arrives */
		SIG_PENDING	=0x20,	/* unready while signal being processed */
		P_STOP		=0x40,	/* set when process is being traced */
		NO_PRIV		=0x80,	/* keep forked system process from running */
	};
	stackframe* _regs;

	pid_t _nr;		/* number of this process (for fast access) */
	struct priv *p_priv;		/* system privileges structure */
	char _rts_flags;		/* SENDING, RECEIVING, etc. */

	char _priority;		/* current scheduling priority */
	char _max_priority;		/* maximum scheduling priority */
	char _ticks_left;		/* number of scheduling ticks left */
	char _quantum_size;		/* quantum size in ticks */

	clock_t _user_time;		/* user time in ticks */
	clock_t _sys_time;		/* sys time in ticks */

	proc *_nextready;	/* pointer to next ready process */
	proc *_caller_q;	/* head of list of procs wishing to send */
	proc *_q_link;	/* link to next proc wishing to send */
	message *p_messbuf;		/* pointer to passed message buffer */
	pid_t _getfrom;		/* from whom does process want to receive? */
	pid_t _sendto;		/* to whom does process want to send? */

	::sigset_t p_pending;		/* bit map for pending kernel signals */

	char p_name[8];	/* name of the process, including \0 */

	inline pid_t nr() const { return _nr; }
	inline pid_t proc_nr(proc* p)  { return p->_nr; }
	constexpr static inline bool isokproc(int n)   { return (n + NR_TASKS) < (NR_PROCS + NR_TASKS); }
	constexpr static inline bool isempty(proc* p)   { return  p->_rts_flags == SLOT_FREE ; }
	constexpr static inline bool isempty(int n)   { return  isempty(proc_addr(n)) ; }
	constexpr static inline bool iskernel(int n)   { return  n < 0 ; }
	constexpr static inline bool iskernel(proc* p)   { return iskernel(p->_nr); }
	constexpr static inline bool isuser(int n)   { return  n >= 0 ; }
	constexpr static inline bool isuser(proc* p)   { return isuser(p->_nr); }
	int mini_send(int dst,message* m_ptr, uint32_t flags);
	int mini_receive(int src, message *m_ptr, uint32_t flags);
	int mini_notify(int dst);
	void enqueue();
	void dequeue();
	void sched(int *queue, int *front) ;
	void pick_proc(void);
public:
	proc();
	virtual ~proc();
	inline bool isempty() const { return _rts_flags == SLOT_FREE; }
	inline bool iskernel() const { return _nr < 0; }
	inline bool isuser() const { return _nr >= 0; }

};

} /* namespace mimx */

#endif /* MIMIX_CPP_PROC_HPP_ */
