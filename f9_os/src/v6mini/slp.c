#include "param.h"
#include "proc.h"
#include "user.h"
#include "systm.h"
#include "file.h"
#include "inode.h"
#include "buf.h"
#include <sys\signal.h>

// combinging this all under proc.c
static struct proc proc[NPROC];
int		mpid=1;			/* generic for unique process id's */
int		runrun=0;			/* scheduling flag */
int		curpri=0;			/* more scheduling */
int 	swapflag;
struct proc *current;

// slp.c
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
void ksleep(void* chan, int pri)
{
	int s = spl6();
	struct proc *rp = u.u_procp;

	rp->p_stat = SSLEEP;
	rp->p_wchan = chan;
	rp->p_pri = pri;
	spl0();
	if(pri > 0) {
		if(issig())
			goto psig;
		swtch();
		if(issig())
			goto psig;
	} else
		swtch();
	splx(s);
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
	retu(u.u_qsav);
}

/*
 * Wake up all processes sleeping on chan.
 */
void kwakeup(void* c)
{
	for_each_array(p, proc) {
		if(p->p_wchan == c) setrun(p);
	}
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
void setrun(struct proc *rp)
{
	rp->p_wchan = 0;
	rp->p_stat = SRUN;
	if(rp->p_pri < 0)
		rp->p_pri = PSLEP;
	if(rp->p_pri < curpri)
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
	static struct proc *p;
	struct proc *rp;
	struct proc *np = NULL;

	if(p == NULL) current = p = &proc[0];
	/*
	 * Search for highest-priority runnable process
	 */
	while(np == NULL){
		if(runrun && swapflag == 0) {
			for_each_array(cp, proc) {
				if(cp->p_stat==SRUN) {
					np = cp;
					runrun = 0;
					break;
				}
			}
		} else {
			if(rp->p_stat == SRUN) np = p;
		}
		// If no process is runnable, idle.
		if(np == NULL)idle();
	}
	current = p = np;
	curpri = np->p_pri;
/*
	if(hp > &hbuf[36])
		hp = hbuf;
	*hp++ = n;
	*hp++ = u.u_procp;
	*hp++ = time[1];
*/
	if(p != u.u_procp && swapflag == 0) {
	/*
	 * Save stack of current user and
	 * and use system stack.
	 */
		swapflag++;
		savu(u.u_ssav);
		retu(u.u_rsav);
		u.u_procp->p_flag &= ~SLOAD;
		if(u.u_procp->p_stat != SZOMB)
			swap(u.u_procp, B_WRITE);
		swap(rp, B_READ);
		rp->p_flag |= SLOAD;
		retu(u.u_ssav);
		u.u_procp = p;
		swapflag--; // swap completed
	}
	/*
	 * The value returned here has many subtle implications.
	 * See the newproc comments.
	 */
	return (1);
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
int newproc()
{
	struct proc *p;
	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
retry:
	p = NULL;
	if(++mpid >= NPROC) mpid = 1;
	for_each_array(np, proc) {
		if(np->p_stat == 0 && p==NULL)
			p = np;
		if (np->p_pid==mpid)
			goto retry;
	}
	if (p==NULL) kpanic("newproc");

	/*
	 * make proc entry for new proc
	 */

	struct proc * up = u.u_procp;
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
	for_each_array(f, u.u_ofile) {
		if(*f !=NULL) (*f)->f_count++;
	}
	u.u_cdir->i_count++;
	/*
	 * Partially simulate the environment
	 * of the new process so that when it is actually
	 * created (by copying) it will look right.
	 */
	u.u_procp = p;
	up->p_stat = SIDL;
	savu(u.u_ssav);
	swap(up, B_WRITE);	/* swap out child */
	up->p_flag |= SSWAP;
	up->p_stat = SRUN;
	u.u_procp = up;
	return (0);	/* return to parent */
}

// sig.c



/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 * Called by tty.c for quits and
 * interrupts.
 */
void kgsignal(int pgrp, sigset_t sig)
{
	if(!pgrp) return;

	for_each_array(p, proc) {
		if(p->p_pgrp == pgrp)
			kpsignal(p, sig);
	}
}

/*
 * Send the specified signal to
 * the specified process.
 */
void kpsignal(struct proc* rp, sigset_t sig)
{
	if(sig >= NSIG) return;
	if(rp->p_sig != SIGKILL) rp->p_sig = (1<<sig);
	if(rp->p_stat == SSLEEP && rp->p_pri>0)
		setrun(rp);
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
int issig()
{
	struct proc *p= u.u_procp;
	if(p->p_sig !=0) {
		int n = __builtin_ctz(p->p_sig);
		if(u.u_signal[n]) return n;
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
	struct proc* rp = u.u_procp;
	sigset_t sig = rp->p_sig;
	rp->p_sig = 0;
	while(sig){
		int n = __builtin_ctz(sig);
		sig &= (1<<n); // clear it
		const struct sigaction* s = u.u_signal[n];
		if(s && s->sa_handler) s->sa_handler(n);
		else {
			// default actions
			switch(n) {
			case SIGIOT:
			case SIGKILL:
			case SIGBUS:
			case SIGFPE:
			case SIGTRAP:
			case SIGILL:
			case SIGQUIT:
			case SIGSEGV:
			case SIGSYS:
			case SIGTERM:
				if(core()) n =+ 0200;
				//u.u_arg->r0 = n;
				break;
			}

		}
	}
	//u.u_arg[0] = (u.u_ar0[R0]<<8) | n;
	//exit();
}

/*
 * Create a core image on the file "core"
 * If you are looking for protection glitches,
 * there are probably a wealth of them here
 * when this occurs to a suid command.
 *
 * It writes USIZE block of the
 * user.h area followed by the entire
 * data+stack segments.
 */
int core()
{
	struct inode *ip;
	u.u_error = 0;
	u.u_dirp = "core";
	ip = namei(1);
	if(ip == NULL) {
		if(u.u_error)
			return(0);
		ip = maknode(0666);
		if (ip==NULL)
			return(0);
	}
	if(!iaccess(ip, IWRITE) &&
			(ip->i_mode&IFMT) == 0 &&
	   u.u_uid == u.u_ruid) {
		itrunc(ip);
		u.u_offset = 0;
		u.u_base = &u;
		u.u_count = (USIZE+UCORE)*64;
		writei(ip);
	}
	iput(ip);
	return(u.u_error==0);
}
