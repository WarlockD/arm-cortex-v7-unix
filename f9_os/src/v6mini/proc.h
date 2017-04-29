#ifndef _MXSYS_PROC_H_
#define _MXSYS_PROC_H_

#include "types.h"

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */

/* stat codes */
enum proc_stat_flags {
	SSLEEP	=1,		/* awaiting an event */
	SWAIT	=2,		/* (abandoned state) */
	SRUN	=3,		/* running */
	SIDL	=4,		/* intermediate state in process creation */
	SZOMB	=5,		/* intermediate state in process termination */
};

enum proc_flags {
	SLOAD	=01,		/* in core */
	SSYS	=02,		/* scheduling process */
	SLOCK	=04,		/* process cannot be swapped */
	SSWAP	=010,		/* process is being swapped out */
};
/* flag codes */

struct	proc
{
	enum proc_stat_flags 	p_stat;
	enum proc_flags	p_flag;
	short	p_pri;		/* priority, negative is high */
	sigset_t	p_sig;		/* signal number sent to this process */
	uid_t	p_uid;		/* user id, used to direct tty signals */
	time_t	p_time;		/* resident time for scheduling */
	time_t	p_cpu;		/* cpu usage for scheduling */
	int		p_nice;		/* nice for cpu usage */
	int		p_pgrp;		/* name of process group leader */
	pid_t	p_pid;		/* unique process id */
	pid_t	p_ppid;		/* process id of parent */
	void*	p_wchan;	/* event process is awaiting */
	time_t	p_clktim;	/* time to alarm clock signal */
	size_t	p_size;		/* process size (for swap) */
};
// proc[NPROC];

int swtch(); // switch processes
void setrun(struct proc* p);
int newproc();
void kgsignal(int pgrp, sigset_t sig);
void kpsignal(struct proc* p, sigset_t sig);
int issig();
void psig();
int core();

#endif
