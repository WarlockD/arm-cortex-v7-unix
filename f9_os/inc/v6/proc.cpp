#include "proc.hpp"
#include <os\irq.hpp>

namespace {
	static time_t 	lbolt=0;		/* time of day in 60th not in time */
	static time_t 	current_time=0;	/* time in sec from 1970 */
	static time_t 	tout=0;			/* time of day of next sleep */
	static int		mpid=0;			/* generic for unique process id's */
	static char		runrun=0;		/* scheduling flag */
	static char		curpri=0;		/* more scheduling */
	static int		rootdev=0;		/* dev of root see conf.c */
	static int		swapdev=0;		/* dev of swap see conf.c */
	template<typename T, size_t S> size_t NITEMS(T(&)[S]){return S;}
};

namespace v6_mini {
	user u;
	callo callout[NCALL];
	proc procs[NPROC];

	void panic() {
		printk("V6_MINI PANIC!\r\n");
		assert(0); // for now
	}
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
	void sleep(void* chan,int  pri)
	{
		proc *rp;
		auto s = irq::spl6();

		rp = u.u_procp;

		rp->p_stat = SSLEEP;
		rp->p_wchan = chan;
		rp->p_pri = pri;
		irq::spl0();
		if(pri > 0) {
			if(issig())
				goto psig;
			swtch();
			if(issig())
				goto psig;
		} else
			swtch();
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
		rp->p_stat = SRUN;
		//std::longjmp(u.u_qsav,1);
		std::longjmp(rp->u_qsav,1);
	}

	/*
	 * Wake up all processes sleeping on chan.
	 */
	void wakeup(void* c)
	{
		for(auto& p: procs) {
			if(p.p_wchan == c) setrun(&p);
		}
	}

	/*
	 * Set the process running;
	 * arrange for it to be swapped in if necessary.
	 */
	void setrun(proc* p)
	{
		p->p_wchan = 0;
		p->p_stat = SRUN;
		if(p->p_pri < 0)
			p->p_pri = PSLEP;
		if(p->p_pri < curpri)
			runrun++;
	}

	/*
	 * This routine is called to reschedule the CPU.
	 * if the calling process is not in RUN state,
	 * arrangements for it to restart must have
	 * been made elsewhere, usually by calling via sleep.
	 */

	/*
	int hbuf[40];
	int *hp hbuf;
	*/


	int swtch()
	{
		static int swapflag = 0;
		static  proc *p = &procs[0];
		int i, n;
		proc *rp;

		//if(p == nullptr) p = &proc[0];

	loop:
		rp = p;
		p = NULL;
		n = 128;
		/*
		 * Search for highest-priority runnable process
		 */
		i = NPROC;
		if(runrun && swapflag == 0) {
			do {
				rp++;
				if(rp >= &procs[NITEMS(procs)])
					rp = &procs[0];
				if(rp->p_stat==SRUN) {
					p = rp;
					n = rp->p_pri;
					runrun = 0;
					break;
				}
			} while(--i);
		} else {
			if(rp->p_stat == SRUN) {
				p = rp;
				n = rp->p_pri;
			}
		}
		/*
		 * If no process is runnable, idle.
		 */
		if(p == nullptr) {
			p = rp;
			irq::idle();
			goto loop;
		}
		curpri = n;
		rp = p;
	/*
		if(hp > &hbuf[36])
			hp = hbuf;
		*hp++ = n;
		*hp++ = u.u_procp;
		*hp++ = time[1];
	*/
		if(rp != u.u_procp && swapflag++ == 0) {
		/*
		 * Save stack of current user and
		 * and use system stack.
		 */
			setjmp(u.u_ssav);
			std::longjmp(u.u_rsav,1);
			u.u_procp->p_flag &= ~SLOAD;
			if(u.u_procp->p_stat != SZOMB)
				swap(u.u_procp, B_WRITE);
			swap(rp, B_READ);
			rp->p_flag |= SLOAD;
			std::longjmp(u.u_ssav,1);
		}
		--swapflag;
		/*
		 * The value returned here has many subtle implications.
		 * See the newproc comments.
		 */
		return(1);
	}

	/*
	 * Create a new process-- the internal version of
	 * sys fork.
	 * It returns 1 in the new process.
	 * How this happens is rather hard to understand.
	 * The essential fact is that the new process is created
	 * in such a way that appears to have started executing
	 * in the same call to newproc as the parent;
	 * but in fact the code that runs is that of swtch.
	 * The subtle implication of the returned value of swtch
	 * (see above) is that this is the value that newproc's
	 * caller in the new process sees.
	 */
	proc* newproc()
	{
		proc *p = nullptr;


		/*
		 * First, just locate a slot for a process
		 * and copy the useful info from this process into it.
		 * The panic "cannot happen" because fork has already
		 * checked for the existence of a slot.
		 */
	retry:

		if(++mpid >= static_cast<decltype(mpid)>(NPROC))
			mpid = 1;
		for(auto& rpp: procs){
			if(rpp.p_stat == SINIT && p == nullptr)
				p = &rpp;
			if(rpp.p_pid==mpid) goto retry;
		}
		if(p == nullptr) panic();

		/// make proc entry for new proc
		proc* up = u.u_procp;
		p->p_stat = SRUN;
		p->p_flag = SLOAD;
		p->p_uid = up->p_uid;
		p->p_pgrp = up->p_pgrp;
		p->p_nice = up->p_nice;
		p->p_pid = mpid;
		p->p_ppid = up->p_pid;

		/*
		 * make duplicate entries
		 * where needed
		 */
#if 0
		for(up = &u.u_ofile[0]; up < &u.u_ofile[NOFILE];)
			if((rpp = *rip++) != NULL)
				rpp->f_count++;
		u.u_cdir->i_count++;
#endif
		/*
		 * Partially simulate the environment
		 * of the new process so that when it is actually
		 * created (by copying) it will look right.
		 */

		u.u_procp = p;
		up->p_stat = SIDL;
		setjmp(u.u_ssav);
		swap(p, B_WRITE);	/* swap out child */
		p->p_flag |= SSWAP;
		up->p_stat = SRUN;
		u.u_procp = up;
		return(0);	/* return to parent */
	}

	/*
	 * Send the specified signal to
	 * all processes with 'pgrp' as
	 * process group.
	 * Called by tty.c for quits and
	 * interrupts.
	 */
	void signal(int pgrp, int sig)
	{
		for(auto& p: procs){
			if(p.p_pgrp == pgrp)
				psignal(&p, sig);
		}
	}

	/*
	 * Send the specified signal to
	 * the specified process.
	 */
	void psignal(proc* p, int sig) {
		if(sig >= NSIG)
			return;
		if(p->p_sig != SIGKILL)
			p->p_sig = sig;
		if(p->p_stat == SSLEEP && p->p_pri>0)
			setrun(p);
	}

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
	bool issig()
	{
		proc * p = u.u_procp;
		int n;
		if((n = p->p_sig)!=0) {
			if(u.u_signal.sa_handler != nullptr)
				return(n);
		}
		return(0);
	}


	/*
	 * Perform the action specified by
	 * the current signal.
	 * The usual sequence is:
	 *	if(issig())
	 *		psig();
	 */
	void psig()
	{
		proc* p = u.u_procp;
		int n = rp->p_sig;
		p->p_sig = 0;
		if((p=u.u_signal[n].sa_handler) != 0) {
			u.u_error = 0;
			if(n != SIGILL)
				u.u_signal[n].sa_handler = 0;
			n = u.u_ar0[R6] - 4;
			suword(n+2, u.u_ar0[RPS]);
			suword(n, u.u_ar0[R7]);
			u.u_ar0[R6] = n;
			u.u_ar0[R7] = p;
			return;
		}
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
		u.u_arg[0] = (u.u_ar0[R0]<<8) | n;
		exit();
	}
};
