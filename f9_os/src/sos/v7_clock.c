#include "v7_user.h"
#include <sys\time.h>
#include <sys\times.h>
#include <sys\queue.h>
#include "v7_internal.h"

// atomic get and set the main time
//
extern void arch_gettimeofday(struct timeval * tv);
extern void arch_settimeofday(struct timeval * tv);
struct	callo
{
	struct timeval	c_time;		/* incremental time */
	void*	c_arg;				/* argument to routine */
	void	(*c_func)(void*);	/* routine */
	TAILQ_ENTRY(callo)	c_queue;
};
static struct callo callout[NCALL];
static TAILQ_HEAD(,callo) callo_freelist;
static TAILQ_HEAD(,callo) callo_running;

void v7_clock_init() {
	TAILQ_INIT(&callo_freelist);
	TAILQ_INIT(&callo_running);
	memset(callout,0,sizeof(callout));
	foreach_array(c, callout) {
		TAILQ_INSERT_HEAD(&callo_freelist,c,c_queue);
	}
}
// get
int _gettimeofday(struct timeval * tv, struct timezone* tz) {
	if(tv == NULL) return -1;
	arch_gettimeofday(tv);
	return 0;
}

void v7_task(void(*c_func)(void*), void*	c_arg, struct timeval* pop) {
	int s = spl6(); // stop the clock
	// we should sleep here till one comes up
	while(TAILQ_EMPTY(&callo_freelist)){
		spl0();
		ksleep(&callo_freelist, NZERO);
	}
	spl6();
	struct callo* c = TAILQ_FIRST(&callo_freelist);
	TAILQ_REMOVE(&callo_freelist,c,c_queue);
	timeradd(&g_time, pop, &c->c_time);
	c->c_arg = c_arg;
	c->c_func = c_func;
	struct callo* t;
	TAILQ_FOREACH(t, &callo_running, c_queue) {
		if(timercmp(&t->c_time, &c->c_time, >)){
			TAILQ_INSERT_BEFORE(t,c,c_queue); // sort
			c = NULL;
			break; // found
		}
	}
	if(c != NULL) {
		TAILQ_INSERT_TAIL(&callo_running,c,c_queue);
	}
	// all done
	splx(s);
}
#if 0
void v7_clock (struct trapframe* tf) // interrupt handler each HZ
{
	register struct callo *p1, *p2;
	register struct proc *pp;
	int a;
	extern caddr_t waitloc;

	if(callout[0].c_func!= NULL){
		p2 = &callout[0];
		while(p2->c_time<=0 && p2->c_func!=NULL)
			p2++;
		p2->c_time--;
	}


	if(callout[0].c_func == NULL)
		goto out;


	/*
	 * if ps is high, just return
	 */
	if (BASEPRI(ps))
		goto out;

	/*
	 * callout
	 */

	spl5();
	if(callout[0].c_time <= 0) {
		p1 = &callout[0];
		while(p1->c_func != 0 && p1->c_time <= 0) {
			(*p1->c_func)(p1->c_arg);
			p1++;
		}
		p2 = &callout[0];
		while(p2->c_func = p1->c_func) {
			p2->c_time = p1->c_time;
			p2->c_arg = p1->c_arg;
			p1++;
			p2++;
		}
	}

	/*
	 * lightning bolt time-out
	 * and time of day
	 */
out:
	a = dk_busy&07;
	if (USERMODE(ps)) {
		u.u_utime++;
		if(u.u_prof.pr_scale)
			addupc(pc, &u.u_prof, 1);
		if(u.u_procp->p_nice > NZERO)
			a += 8;
	} else {
		a += 16;
		if (pc == waitloc)
			a += 8;
		u.u_stime++;
	}
	dk_time[a] += 1;
	pp = u.u_procp;
	if(++pp->p_cpu == 0)
		pp->p_cpu--;
	if(++lbolt >= HZ) {
		if (BASEPRI(ps))
			return;
		lbolt -= HZ;
		++time;
		spl1();
		runrun++;
		wakeup((caddr_t)&lbolt);
		for(pp = &proc[0]; pp < &proc[NPROC]; pp++)
		if (pp->p_stat && pp->p_stat<SZOMB) {
			if(pp->p_time != 127)
				pp->p_time++;
			if(pp->p_clktim)
				if(--pp->p_clktim == 0)
					psignal(pp, SIGCLK);
			a = (pp->p_cpu & 0377)*SCHMAG + pp->p_nice - NZERO;
			if(a < 0)
				a = 0;
			if(a > 255)
				a = 255;
			pp->p_cpu = a;
			if(pp->p_pri >= PUSER)
				setpri(pp);
		}
		if(runin!=0) {
			runin = 0;
			wakeup((caddr_t)&runin);
		}
	}
}
/*
 * timeout is called to arrange that
 * fun(arg) is called in tim/HZ seconds.
 * An entry is sorted into the callout
 * structure. The time in each structure
 * entry is the number of HZ's more
 * than the previous entry.
 * In this way, decrementing the
 * first entry has the effect of
 * updating all entries.
 *
 * The panic is there because there is nothing
 * intelligent to be done if an entry won't fit.
 */
int  timeout (int (*fun)(void), caddr_t arg, int tim)
{
	register struct callo *p1, *p2;
	register int t;
	int s;

	t = tim;
	p1 = &callout[0];
	s = spl7();
	while(p1->c_func != 0 && p1->c_time <= t) {
		t -= p1->c_time;
		p1++;
	}
	if (p1 >= &callout[NCALL-1])
		panic("Timeout table overflow");
	p1->c_time -= t;
	p2 = p1;
	while(p2->c_func != 0)
		p2++;
	while(p2 >= p1) {
		(p2+1)->c_time = p2->c_time;
		(p2+1)->c_func = p2->c_func;
		(p2+1)->c_arg = p2->c_arg;
		p2--;
	}
	p1->c_time = t;
	p1->c_func = fun;
	p1->c_arg = arg;
	splx(s);
}




/*
 * Everything in this file is a routine implementing a system call.
 */

/*
 * return the current time (old-style entry)
 */
time_t v7_gtime()
{
	return g_time.tv_sec;
}

/*
 * Set the time
 */

void  v7_stime(time_t time)
{
	if(u.u_uid==0)g_time.tv_sec = time;
}
#endif
