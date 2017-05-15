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

	/* Here is the declaration of the process table.  Three assembly code routines
	 * reference fields in it.  They are restart(), save(), and csv().  When
	 * changing 'proc', be sure to change the field offsets built into the code.
	 * It contains the process' registers, memory map, accounting, and message
	 * send/receive information.
	 */
	struct proc {
		/* The following items pertain to the 3 scheduling queues. */
		enum PROC_STATUS {
			RUNNING			 =0,
			/* Bits for p_flags in proc[].  A process is runnable iff p_flags == 0 */
			P_SLOT_FREE      =001,	/* set when slot is not in use */
			NO_MAP           =002,	/* keeps unmapped forked child from running */
			SENDING          =004,	/* set when process blocked trying to send */
			RECEIVING        =010,	/* set when process blocked trying to recv */
		};
		enum QINDEX {
			TASK_Q = 0,	/* ready tasks are scheduled via queue 0 */
			SERVER_Q,           	/* ready servers are scheduled via queue 1 */
			USER_Q,             	/* ready users are scheduled via queue 2 */
			NQ                 	/* # of scheduling queues */
		};
		uint32_t p_reg[NR_REGS];	/* process' registers */
		volatile uint32_t *p_sp;			/* stack pointer */
		struct pc_psw p_pcpsw;		/* pc and psw as pushed by interrupt */
		PROC_STATUS p_flags;			/* P_SLOT_FREE, SENDING, RECEIVING, etc. */
		struct mem_map p_map[NR_SEGS];/* memory map */
		int *p_splimit;		/* lowest legal stack value */
		int p_pid;			/* process id passed in from MM */

		time_t user_time;		/* user time in ticks */
		time_t sys_time;		/* sys time in ticks */
		time_t child_utime;	/* cumulative user time of children */
		time_t child_stime;	/* cumulative sys time of children */
		time_t p_alarm;		/* time of next alarm in ticks, or 0 */

		proc *p_callerq;	/* head of list of procs wishing to send */
		proc *p_sendlink;	/* link to next proc wishing to send */
		message p_mess;	// why use the datasegment when we can put this in the proc structure?
		message *p_messbuf;   /* pointer to message buffer */
		int p_getfrom;		/* from whom does process want to receive? */

		proc *p_nextready;	/* pointer to next ready process */
		int p_pending;		/* bit map for pending signals 1-16 */

		// static part, more ore less the kernel
		static proc procs[NR_TASKS+NR_PROCS];


	  	static proc *proc_ptr;	/* &proc[cur_proc] */
	  	static proc *bill_ptr;	/* ptr to process to bill for clock ticks */
	  	static proc *rdy_head[NQ];	/* pointers to ready list headers */
	  	static proc *rdy_tail[NQ];	/* pointers to ready list tails */
	  	static bitops::bitmap_t<NR_TASKS+1> busy_map;		/* bit map of busy tasks */
	  	static message *task_mess[NR_TASKS+1];	/* ptrs to messages for busy tasks */

	  	constexpr static inline proc* proc_addr(pid_t n) { return &procs[NR_TASKS + n]; }
	    constexpr static proc* NIL_PROC = (struct proc *)nullptr;
	private:
	    static int mini_send(pid_t caller, pid_t dest, message* m_ptr);
		static int mini_rec(pid_t caller, pid_t src, message* m_ptr);
		static void inform(pid_t proc_nr);
		static void ready(proc* p);
		static void unready(proc *p);
		static void interrupt(pid_t task, message* m_ptr);
		static void sched();
		static void pick_proc();
	} ;


	/* System calls. */

	enum sys_function {
		SEND		   =1,	/* function code for sending messages */
		RECEIVE		   =2,	/* function code for receiving messages */
		BOTH		   =3,	/* function code for SEND + RECEIVE */
		ANY   =(NR_PROCS+100),	/* receive(ANY, buf) accepts from any source */
	};
	/* Task numbers, function codes and reply codes. */
	constexpr static pid_t HARDWARE     =     -1;	/* used as source on interrupt generated msgs */
	int sys_call(sys_function function, pid_t caller, pid_t src_dest, message* m_ptr);






} /* namespace mimx */

#endif /* MIMIX_CPP_PROC_HPP_ */
