/*
 * thread.hpp
 *
 *  Created on: Apr 30, 2017
 *      Author: warlo
 */

#ifndef OS_THREAD_HPP_
#define OS_THREAD_HPP_

#include "types.hpp"
//#include "arch.h"
#include "list.hpp"
#include "irq.hpp"
#include "tailq.hpp"
#include <sys\signal.h>
#include <sys\time.h>


namespace os {
// time helpers so we don't have to use macros
	typedef uint32_t stack_item_t;

	class callo
	{
	public:
	   typedef int	(*callo_func)(void*) ;
	   callo() : time() {}
	   virtual void action() = 0;
	   virtual ~callo() {}
	   static void timeout(callo* call, timeval_t t);
	   static void clock(const timeval_t& delta);
	   bool operator< (const callo& r) const { return time < r.time; }
	protected:
		tailq::entry<callo> link;
		timeval_t	time;			/* incremental time */



		static tailq::prio_head<callo,&callo::link> _callouts;
	};
	namespace hash_internal {
		static constexpr size_t MAGIC = 0x45d9f3b;
		static constexpr inline size_t size_cast(const int v) { return static_cast<size_t>(v); }
		static constexpr inline size_t size_cast(const void* v) { return reinterpret_cast<size_t>(v); }

		static constexpr inline size_t round(size_t x)  { return (x >> 16) ^ x; }
		static constexpr inline size_t round(size_t x,size_t s)  { return round(x) * s; }
		static constexpr inline size_t hash(size_t x)  { return round(round(round(x,MAGIC),MAGIC)); }
	};

	template<typename T> //, typename = std::enable_if<std::is_arithmetic<T>::value>::type>
	static constexpr inline  size_t hash_number(T v)  {
		// to make this constexpr had to do some recursive magic
		//x = ((x >> 16) ^ x) * 0x45d9f3b;
		//x = ((x >> 16) ^ x) * 0x45d9f3b;
		//x = (x >> 16) ^ x;
		return hash_internal::hash(hash_internal::size_cast(v));
	}

	enum class  proc_state_t {
		unused,
		embryo,
		sleeping, /* Process debugging or suspension. */
		runnable,
		running,
		zombie,/* Awaiting collection by parent. */
		stopped, /* Sleeping on an address. */
   };

	namespace reg {
		static constexpr size_t REG_START = 0;
		enum reg_e {
			//	R13  =REG_START           ,  /* R13 = SP at time of interrupt */
	#ifdef CONFIG_ARMV7M_USEBASEPRI
	//#  define BASEPRI,        /* BASEPRI */
	#else
	//#  define PRIMASK,       (1)  /* PRIMASK */
	#endif
				R4 = REG_START,               /* R4 */
				R5,               /* R5 */
				R6,              /* R6 */
				R7,               /* R7 */
				R8,               /* R8 */
				R9,                /* R9 */
				R10,              /* R10 */
				R11,             /* R11 */
		#ifdef CONFIG_BUILD_PROTECTED
				EXC_RETURN    ,	/* EXC_RETURN */
		#define _SW_XCPT_REGS      EXC_RETURN
		#else
		#define _SW_XCPT_REGS      R11
		#endif
				R0,               /* R0 */
				R1,               /* R1 */
				R2 ,              /* R2 */
				R3 ,             /* R3 */
				R12,            /* R12 */
				R14 ,            /* R14 = LR */
				R15,            /* R15 = PC */
				XPSR ,         /* xPSR */
		};
		static constexpr reg_e LR = R14;
		static constexpr reg_e PC = R15;

		static constexpr size_t hw_regs_count = 8; // xpsr - r0
		static constexpr size_t hw_regs_size = 8*sizeof(uint32_t);
		static constexpr size_t sw_regs_count = _SW_XCPT_REGS; // xpsr - r0

		static constexpr size_t sw_regs_size = sw_regs_count*sizeof(uint32_t); // xpsr - r0
		static constexpr size_t regs_count = hw_regs_count + sw_regs_count;
		static constexpr size_t regs_size = regs_count*sizeof(uint32_t);
	#undef _SW_XCPT_REGS
	};
	class proc {
	public:
		int pid() const { return _pid; }
		proc();
		proc(stack_item_t * StackPoolEnd , void (*exec)());
        void init_stack_frame(stack_item_t * StackPoolEnd , void (*exec)());
    	// takes the current proc stack state and makes a copy

    protected:
       stack_item_t *     _stack;
	    sigset_t 	_sigmask;         /* Current signal mask. */
	    sigset_t 	_sigignore;       /* Signals being ignored. */
	    sigset_t 	_sigcatch;        /* Signals being caught by user. */

	    uint8_t  	_priority;         /* Process priority. */
	    uint8_t  	_usrpri;           /* User-priority based on p_cpu and p_nice. */

		int	_pid;		/* unique process id */
		int	_ppid;		/* process id of parent */
		proc_state_t _state;

        volatile uint32_t _timeout;

        const void* _wchan;           /* Sleep address. */

    	char	p_flag;
    	char	_pri;		/* priority, negative is high */
    	char	_time;		/* resident time for scheduling */
    	char    _clktime;
    	char	_cpu;		/* cpu usage for scheduling */
    	char	_nice;		/* nice for cpu usage */
    	short	_sig;		/* signals pending to this process */
    	short	_uid;		/* user id, used to direct tty signals */
    	short	_pgrp;		/* name of process group leader */
    	//short	_pid;		/* unique process id */
    	//short	_ppid;		/* process id of parent */
    	short	_addr;		/* address of swappable image */
    	size_t	_size;		/* size of swappable image (bytes) */
    	uint32_t _signal;
    	struct text *p_textp;	/* pointer to text structure */
    	struct proc *p_link;	/* linked list of running processes */
    	int	p_clktim;	/* time to alarm clock signal */




        tailq::entry<proc> _runq;
        list::entry<proc> _link;
        list::entry<proc> _hash; // hash of all
        list::entry<proc> _sibling;
        list::head<proc,&proc::_sibling> _children;
#ifdef OSDEBUG
        const stack_item_t * const _stack_pool;
        const size_t _stack_size; // as number of stack_item_t items
#endif
        friend class kernel;
	};

	class kernel {
	public:
		enum  prio_t {
			PSWP	=0,
			PINOD	=10,
			PRIBIO	=20,
			PZERO	=25,
			NZERO	=20,
			PPIPE	=26,
			PWAIT	=30,
			PSLEP	=40,
			PUSER	=50,
			MAXPRI = 127  ,       /* Priorities range from 0 through MAXPRI. */

			PRIMASK =0x0ff,
			PCATCH  =0x100,       /* OR'd with pri for tsleep to check signals */
		};
		// stolen from litebsd
		/*
		 * Scale factor for scaled integers used to count %cpu time and load avgs.
		 *
		 * The number of CPU `tick's that map to a unique `%age' can be expressed
		 * by the formula (1 / (2 ^ (FSHIFT - 11))).  The maximum load average that
		 * can be calculated (assuming 32 bits) can be closely approximated using
		 * the formula (2 ^ (2 * (16 - FSHIFT))) for (FSHIFT < 15).
		 *
		 * For the scheduler to maintain a 1:1 mapping of CPU `tick' to `%age',
		 * FSHIFT must be at least 11; this gives us a maximum load avg of ~1024.
		 */
		static constexpr uint32_t FSHIFT  =11;      /* bits to right of fixed binary point */
		static constexpr uint32_t FSCALE  =(1<<FSHIFT);
		/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
		static constexpr uint32_t loadfactor(uint32_t loadav) { return   (2 * (loadav)); }
		static constexpr uint32_t ccpu = 0.95122942450071400909d * FSCALE;     /* exp(-1/20) */
		static constexpr uint32_t decay_cpu(uint32_t loadfac, uint32_t cpu) { return  (((loadfac) * (cpu)) / ((loadfac) + FSCALE)); }
		/*
		 * If `ccpu' is not equal to `exp(-1/20)' and you still want to use the
		 * faster/more-accurate formula, you'll have to estimate CCPU_SHIFT below
		 * and possibly adjust FSHIFT in "param.h" so that (FSHIFT >= CCPU_SHIFT).
		 *
		 * To estimate CCPU_SHIFT for exp(-1/20), the following formula was used:
		 *  1 - exp(-1/20) ~= 0.0487 ~= 0.0488 == 1 (fixed pt, *11* bits).
		 *
		 * If you dont want to bother with the faster/more-accurate formula, you
		 * can set CCPU_SHIFT to (FSHIFT + 1) which will use a slower/less-accurate
		 * (more general) method of calculating the %age of CPU used by a process.
		 */
		static constexpr uint32_t CCPU_SHIFT = 11;

		/* decay 95% of `p_pctcpu' in 60 seconds; see CCPU_SHIFT before changing */


		/*
		 * If `ccpu' is not equal to `exp(-1/20)' and you still want to use the
		 * faster/more-accurate formula, you'll have to estimate CCPU_SHIFT below
		 * and possibly adjust FSHIFT in "param.h" so that (FSHIFT >= CCPU_SHIFT).
		 *
		 * To estimate CCPU_SHIFT for exp(-1/20), the following formula was used:
		 *  1 - exp(-1/20) ~= 0.0487 ~= 0.0488 == 1 (fixed pt, *11* bits).
		 *
		 * If you dont want to bother with the faster/more-accurate formula, you
		 * can set CCPU_SHIFT to (FSHIFT + 1) which will use a slower/less-accurate
		 * (more general) method of calculating the %age of CPU used by a process.
		 */

	public:

	    static inline uint16_t new_pid() {
			static int last_pid = 1;
			do {
				auto pid = ++last_pid;
				if(pid  < 0) pid = 1;
				return pid; // need to check if pid exists
			} while(1);
		}
	   struct lookup_entry_equals {
			constexpr bool operator()(const proc&l, const proc&r) const {
				return operator()(l,r._pid);
			}
			constexpr bool operator()(const proc&l, int value) const {
				return l._pid == value;
			}
		};
		struct lookup_entry_hasher {
			constexpr inline size_t operator()(const int x) const {
				return os::hash_number(x);
			}
			constexpr inline size_t operator()(const proc&l) const {
				return operator()(l._pid);
			}
		};
		struct wait_entry_hasher {
			constexpr size_t operator()(const void* ptr) const {
				return os::hash_number(ptr);
			}
			constexpr size_t operator()(const proc&l) const {
				return operator()(l._wchan);
			}
		};
	   struct wait_entry_equals {
			constexpr bool operator()(const proc&l, const proc&r) const {
				return operator()(l,r._wchan);
			}
			constexpr bool operator()(const proc&l, const void* const value) const {
				return reinterpret_cast<size_t>(l._wchan) == reinterpret_cast<size_t>(value);
			}
		};
		/*
		 * The callout structure is for
		 * a routine arranging
		 * to be called by the clock interrupt
		 * (clock.c) with a specified argument,
		 * in a specified amount of time.
		 * Used, for example, to time tab
		 * delays on typewriters.
		 */
		using proc_list_t = list::head<proc,&proc::_link> ;
		using proc_queue_t = tailq::head<proc,&proc::_runq> ;
		using proc_lookup_hash_t = list::hash<proc,&proc::_hash,16,lookup_entry_hasher,lookup_entry_equals>;
		using proc_wait_hash_t = list::hash<proc,&proc::_hash,16,wait_entry_hasher,wait_entry_equals>;

		static proc_lookup_hash_t _pid_lookup;
		static proc_wait_hash_t _wait_lookup;
		static list::head<proc,&proc::_link> _queue;
		static tailq::head<proc,&proc::_runq> _runq;

		static proc_list_t _proc; // list of all procs, not nessarly in order
		static struct ::sigaction _sigactions[NSIG];
		static int _errno;
		static int _curpri;
		static int _runrun;
		static int _runout ;
		static proc* _curproc;	// static?
		static proc* _schedproc;	// static?
		static int cold; // humm
		static const char* panicstr ;
		static void panic(const char* msg){
			panicstr = msg;
			kpanic(msg);
			while(1);
		}

		static constexpr size_t SQSIZE = 0100;	/* Must be power of 2 */

		static inline proc_queue_t& get_slp_bucket(const  void* ptr){
			static proc_queue_t slpque[SQSIZE]; // hack
			uintptr_t iptr = reinterpret_cast<uintptr_t>(ptr);
			size_t hash = (( iptr >> 5) & (SQSIZE-1));
			return slpque[hash];
		}
		static stack_item_t* context_switch_hook(stack_item_t* sp);
		//static stack_item_t* context_switch_hook(stack_item_t* sp) ;
		// sleep stuff

		static void wakeup (const void* chan)
		{
			auto& bucket = get_slp_bucket(chan);
			auto s = irq::splhigh();
			for(auto p = bucket.begin(); p != bucket.end();){
				if(p->_wchan==chan && p->_state != proc_state_t::zombie) {
					proc *sp = &(*p);
					p=bucket.erase(p);
					sp->_wchan = nullptr;
					setrun(sp);
				} else ++p;
			}
			irq::splx(s);
		}
		static void start_os() ;
		/*
		 * Give up the processor till a wakeup occurs
		 * on chan, at which time the process
		 * enters the scheduling queue at priority pri.
		 * The most important effect of pri is that when
		 * pri<=0 a signal cannot disturb the sleep;
		 * if pri>0 signals will be processed.
		 * Callers of this routine must be prepared for
		 * premature return, and check that the reason for
		 * sleeping has gone away.
		 */
		static void sleep(void* chan, prio_t pri)
		{
			auto s = irq::splhigh();
			proc* p = _curproc;
			p->_state = proc_state_t::sleeping;
			p->_wchan = chan;
			p->_pri = pri;
			irq::spl0();
			if(pri > 0) {
				if(issig())
					goto psig;
				swtch_proc();
				if(issig())
					goto psig;
			} else
				swtch_proc();
			irq::splx(s);
			return;

			/*
			 * If priority was low (>0) and
			 * there has been a signal,
			 * execute non-local goto to
			 * the qsav location.
			 * (see trap1/trap.c)
			 */
		psig:
			p->_state = proc_state_t::running;
			//retu(u.u_qsav);
		}
		/*
		 * when you are sure that it
		 * is impossible to get the
		 * 'proc on q' diagnostic, the
		 * diagnostic loop can be removed.
		 */
		static void setrq (proc *p){
			auto s = irq::splhigh();
			for(auto& q: _runq) {
				if(&q == p) {
					panic("proc on q\r\n");
					assert(0);
					irq::splx(s);
					return;
				}
			}
			_runq.push_front(p);
			irq::splx(s);
		}
		/*
		 * Set the process running;
		 * arrange for it to be swapped in if necessary.
		 */
		static void setrun(proc *p)
		{
			if (p->_state <= proc_state_t::zombie)
				panic("Running a dead proc");
			/*
			 * The assignment to w is necessary because of
			 * race conditions. (Interrupt between test and use)
			 */
			if (p->_wchan!=nullptr) {
				wakeup(p->_wchan);
				return;
			}
			p->_state = proc_state_t::running;
			//setrq(p);
			if(p->_pri < 0) p->_pri = PSLEP;
			if(p->_pri < _curpri) _runrun++;
#if 0
			// loading routine from disk
			if(_runout != 0 && (p->p_flag&SLOAD) == 0) {
				runout = 0;
				wakeup((caddr_t)&runout);
			}
#endif
		}
		/*
		 * Set user priority.
		 * The rescheduling flag (runrun)
		 * is set if the priority is better
		 * than the currently running process.
		 */
		static int setpri ( proc *pp)
		{
			int p;
			p = (pp->_cpu & 0377)/16;
			p += PUSER + pp->_nice - NZERO;
			if(p > MAXPRI)
				p = MAXPRI;
			if(p < _curpri)
				_runrun++;
			pp->_pri = p;
			return(p);
		}
		 static inline sigset_t has_signal(sigset_t sigset) { return sigset != 0; }
		 static inline sigset_t has_signal(sigset_t sigset, sigset_t sig) { return (sigset_t)(sigset & (1<<sig)) ==  (sigset_t)(1<<sig); }


		 static inline sigset_t next_signal(sigset_t& sigset)
	    {
	    	uint32_t s=  31 - __builtin_clz(sigset);
	    	if(s) sigset &= (1<<s);
	    	return s;
	    }
	    static inline sigset_t current_signal(sigset_t sigset) { return  sigset == 0 ? 0 : 31 - __builtin_clz(sigset); }
	    static inline void set_signal(sigset_t& sigset, sigset_t sig){ if(sig) sigset |= (1<<sig); }
		/*
		 * Returns true if the current
		 * process has a signal to process.
		 * This is asked at least once
		 * each time a process enters the
		 * system.
		 * A signal does not do anything
		 * directly to a process; it sets
		 * a flag that asks the process to
		 * do something to itself.
		 */
	    static bool issig()
		{
			uint32_t n;
			proc *p = _curproc;
			if((n = current_signal(p->_signal))!=0) {
				if(_sigactions[n].sa_handler != nullptr) return true;
			}
			return(0);
		}
		/*
		 * Send the specified signal to
		 * all processes with 'pgrp' as
		 * process group.
		 * Called by tty.c for quits and
		 * interrupts.
		 */
	    static void signal(int pgrp, sigset_t sig)
		{
			if (pgrp==0) return;
			for(auto& p : _proc){
				if(p._pgrp == pgrp) psignal(&p,sig);
			}
		}
		/*
		 * Send the specified signal to
		 * the specified process.
		 */
	    static void psignal(proc* p, sigset_t sig)
		{
			if(sig >= NSIG) return;
			if(!(has_signal(p->_signal))) set_signal(p->_signal,sig);
			if(p->_state == proc_state_t::sleeping && p->_pri>0)
				setrun(p);
		}
		/*
		 * Perform the action specified by
		 * the current signal.
		 * The usual sequence is:
		 *	if(issig())
		 *		psig();
		 */
	    static void psig()
		{

			uint32_t n;
			proc *p = _curproc;
			if(p->_signal == 0) return; // failsafe
			sigset_t sigs = p->_signal;
			p->_signal = 0;
			while((n  = next_signal(sigs))!= 0){
				if(_sigactions[n].sa_handler != nullptr) {
					_errno = 0;
					_sigactions[n].sa_handler(n);
				}
			}
#if 0
				// handle default signals
				switch(n) {

				case SIGQIT:
				case SIGINS:
				case SIGTRC:
				case SIGIOT:
				case SIGEMT:
				case SIGFPT:
				case SIGBUS:
				case SIGSEG:
				case SIGSYS:
					u.u_arg[0] = n;
					if(core())
						n =+ 0200;
				}
			}
#endif
			// return to process
		}
		static constexpr int SCHMAG	=8/10;
		static void register_process(proc* p);
		// whatever clock tick it is
		static constexpr int HZ	=10;
		static void tick() {
			for(auto& pp : _proc) {
				if(pp._state < proc_state_t::zombie){
					if(pp._time != 127) pp._time++;
					if(pp._clktime)
						if(--pp._clktime ==0) psignal(&pp, SIGALRM);
					int a = (pp._cpu & 0377)*SCHMAG + pp._nice - NZERO;
					if(a < 0) a = 0;
					if(a > 255) a = 255;
					pp._cpu  = a;
					if(pp._pri >= PUSER)setpri(&pp);
				}
				if(_runrun) swtch_proc();
#if 0
				// no disk swap yet
				if(_runin) {
					_runin = false;
					wakeup(&_runin);
				}
#endif
			}
		}
		static void  idle() { __asm volatile("wfi"::); }
		static void context_switch(proc* CurProc, proc*NextProc);

		static int swtch_proc();
};
};
#endif /* OS_THREAD_HPP_ */
