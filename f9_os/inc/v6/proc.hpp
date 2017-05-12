#ifndef V6_PROC_HPP_
#define V6_PROC_HPP_

#include <os\types.hpp>
//#include "arch.h"
#include <os\list.hpp>
#include <os\tailq.hpp>
#include <sys\signal.h>
#include <sys\time.h>
#include  <csetjmp>
#include <csignal>

namespace v6_mini {
	// tunable variables
	static constexpr size_t NBUF	=6;		/* size of buffer cache */
	static constexpr size_t NINODE	=50;		/* number of in core inodes */
	static constexpr size_t NFILE	=50;		/* number of in core file structures */
	static constexpr size_t NMOUNT	=2;		/* number of mountable file systems */
	static constexpr size_t SSIZE	=20;		/* initial stack size (*64 bytes) */
	static constexpr size_t NOFILE	=15;		/* max open files per process */
	static constexpr size_t CANBSIZ	=256;		/* max size of typewriter line */
	static constexpr size_t NCALL	=4;		/* max simultaneous time callouts */
	static constexpr size_t NPROC	=13;		/* max number of processes */
	static constexpr size_t NCLIST	=100;		/* max total clist size */
	static constexpr size_t HZ	=60	;	/* Ticks/second of the clock */
	// configuration dependent variables
	static constexpr size_t UCORE	=(16*32);
	static constexpr size_t TOPSYS	=12*2048;
	static constexpr size_t TOPUSR	=28*2048;
	static constexpr size_t ROOTDEV =0;
	static constexpr size_t SWAPDEV =0;
	static constexpr size_t SWPLO	=4000;
	static constexpr size_t NSWAP	=872;
	static constexpr size_t SWPSIZ	=66;
	// fundamental constants , cannot be changed
	static constexpr size_t USIZE	=16;		/* size of user block (*64) */
	static constexpr short NODEV	=(-1);
	static constexpr size_t ROOTINO	=1;		/* i number of all roots */
	static constexpr size_t DIRSIZ	=14;		/* max characters per directory */



// priorities, probably should not be altered too much
	enum priorities {
		PSWP	=-100,
		PINOD	=-90,
		PRIBIO	=-50,
		PPIPE	=1,
		PWAIT	=40,
		PSLEP	=90,
		PUSER	=100,
	};

	enum proc_status { /* stat codes */
		SINIT=0,
		SSLEEP,			/* awaiting an event */
		SWAIT,			/* (abandoned state) */
		SRUN,			/* running */
		SIDL,			/* intermediate state in process creation */
		SZOMB,			/* intermediate state in process termination */
	};
	enum proc_flags {
		/* flag codes */
		SLOAD	=01,		/* in core */
		SSYS	=02,		/* scheduling process */
		SLOCK	=04,		/* process cannot be swapped */
		SSWAP	=010,		/* process is being swapped out */
	};

	enum bio_flags {
		/*
		 * These flags are kept in b_flags.
		 */
		B_WRITE	=0,	/* non-read pseudo-flag */
		B_READ	=01,	/* read when I/O occurs */
		B_DONE	=02,	/* transaction finished */
		B_ERROR	=04,	/* transaction aborted */
		B_BUSY	=010,	/* not on av_forw/back list */
		B_WANTED =0100,	/* issue wakeup when BUSY goes off */
		B_RELOC	=0200,	/* no longer used */
		B_ASYNC	=0400,	/* don't wait for I/O completion */
		B_DELWRI =01000,	/* don't write till block leaves available list */
	};
	/*
	 * The callout structure is for
	 * a routine arranging
	 * to be called by the clock interrupt
	 * (clock.c) with a specified argument,
	 * in a specified amount of time.
	 * Used, for example, to time tab
	 * delays on teletypes.
	 */
	struct	callo
	{
		int		c_time;		/* incremental time */
		int		c_arg;		/* argument to routine */
		void	(*c_func)(void*);	/* routine */
	};




	/*
	* The user structure.
	* One allocated per process.
	* Contains all per process data
	* that doesn't need to be referenced
	* while the process is swapped.
	* The user block is USIZE*64 bytes
	* long; resides at virtual kernel
	* loc 140000; contains the system
	* stack per user; is cross referenced
	* with the proc structure for the
	* same process.
	*/
	struct proc;
	typedef struct ::sigaction sigaction_t;
	struct user
	{
		std::jmp_buf	u_rsav;		/* save r5,r6 when exchanging stacks */
		std::jmp_buf	u_qsav;		/* label variable for quits and interrupts */
		std::jmp_buf	u_ssav;		/* label variable for swapping */

		int	u_fsav[25];		/* save fp registers */
						/* rsav and fsav must be first in structure */
		char	u_segflg;		/* flag for IO; user or kernel space */
		char	u_error;		/* return error code */
		char	u_uid;			/* effective user id */
		char	u_gid;			/* effective group id */
		char	u_ruid;			/* real user id */
		char	u_rgid;			/* real group id */
		proc*	u_procp;	/* pointer to proc structure */
		char	*u_base;		/* base address for IO */
		char	*u_count;		/* bytes remaining for IO */
		char	*u_offset[2];		/* offset in file for IO */
		int		*u_cdir;		/* pointer to inode of current directory */
		char	u_dbuf[DIRSIZ];		/* current pathname component */
		char	*u_dirp;		/* current pointer to inode */
		struct	{			/* current directory entry */
			int	u_ino;
			char	u_name[DIRSIZ];
		} u_dent;
		int	*u_pdir;		/* inode of parent directory of dirp */
		int	u_uisa[16];		/* prototype of segmentation addresses */
		int	u_uisd[16];		/* prototype of segmentation descriptors */
		int	u_ofile[NOFILE];	/* pointers to file structures of open files */
		int	u_arg[5];		/* arguments to current system call */
		int	u_tsize;		/* text size (*64) */
		int	u_dsize;		/* data size (*64) */
		int	u_ssize;		/* stack size (*64) */
		int	u_sep;			/* flag for I and D separation */

		sigaction_t u_signal[NSIG];		/* disposition of signals */
		time_t	u_utime;		/* this process user time */
		time_t	u_stime;		/* this process system time */
		time_t	u_cutime;		/* sum of childs' utimes */
		time_t	u_cstime;		/* sum of childs' stimes */
		int	*u_ar0;			/* address of users saved R0 */
		int	u_prof[4];		/* profile arguments */
		char	u_intflg;		/* catch intr from sys */
		int	u_ttyp;			/* controlling tty pointer */
		int	u_ttyd;			/* controlling tty dev */
						/* kernel stack per user
						 * extends from u + USIZE*64
						 * backward not to reach here
						 */
	} ;

	/*
		 * One structure allocated per active
		 * process. It contains all data needed
		 * about the process while the
		 * process may be swapped out.
		 * Other per process data (user.h)
		 * is swapped with the process.
		 */
		struct	proc
		{
			user u;
			uint32_t* p_stack;	// stack location
			// hack till I figure out swaping.  Hummm

		//	proc_status	p_stat;
		//	proc_flags	p_flag;
			int	p_stat;
			int	p_flag;
			//priorities	p_pri;		/* priority, negative is high */
			int 	p_pri;		/* priority, negative is high */
			char	p_sig;		/* signal number sent to this process */
			char	p_uid;		/* user id, used to direct tty signals */
			char	p_time;		/* resident time for scheduling */
			char	p_cpu;		/* cpu usage for scheduling */
			char	p_nice;		/* nice for cpu usage */
			int		p_pgrp;		/* name of process group leader */
			int		p_pid;		/* unique process id */
			int		p_ppid;		/* process id of parent */
			void*	p_wchan;	/* event process is awaiting */
			time_t	p_clktim;	/* time to alarm clock signal */
			int		p_size;		/* process size (for swap) */


		} ;
	extern user u;
	extern callo callout[NCALL];
	extern proc procs[NPROC];

	//check

	void panic();
	// slp.c
	void sleep(void*chan, int pri);
	void wakeup(void*chan);
	void setrun(proc* p);
	int swtch();
	proc*  newproc();

	// swaping out processes

	// hack for right now
	void swap(proc* p, int mode) {
		if(mode == B_READ) {
			u = p->u;
		} else if(mode == B_WRITE){
			p->u=u;
		}
	}// we dont do anything here as its not supported yet

	// sig.c
	void signal(int apgrp, int sig);
	void psignal(proc* p, int sig);
	int issig();
	void psig();
	void sendsig(void* p,int signo); // sends a signal using the current process stack
	//bool core();
};





#endif
