#ifndef _V7_USER_H_
#define _V7_USER_H_

#include "v7_types.h"
#include "v7_inode.h"


#define	CANBSIZ	256		/* max size of typewriter line */
#define	CMAPSIZ	50		/* size of core allocation area */
#define	SMAPSIZ	50		/* size of swap allocation area */
#define	NCALL	20		/* max simultaneous time callouts */

extern struct timeval g_time;


// Segments in proc->gdt.
// Also known to bootasm.S and trapasm.S
#define SEG_KCODE 1  // kernel code
#define SEG_KDATA 2  // kernel data+stack
#define SEG_KCPU  3  // kernel per-cpu data
#define SEG_UCODE 4
#define SEG_UDATA 5
#define SEG_TSS   6  // this process's task state
#define NSEGS     7

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context must match the code in swtch.S.
struct context {
	uint32_t basepri;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t ex_lr;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
};

/*
 * Text structure.
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */
struct text
{
	struct context  x_context;      // stack backup
	daddr_t			x_daddr;		/* disk address of segment (relative to swplo) */
	caddr_t			x_caddr;		/* core address, if loaded */
	size_t			x_size;			/* size total */
	struct inode *	x_iptr;			/* inode of prototype */
	int				x_count;		/* reference count */
	int				x_ccount;		/* number of loaded references */
	int				x_flag;			//	* traced, written flags */
};

extern struct text text[];

#define	XTRC	01		/* Text may be written, exclusive use */
#define	XWRIT	02		/* Text written into, must swap out */
#define	XLOAD	04		/* Currently being read from file */
#define	XLOCK	010		/* Being swapped in or out */
#define	XWANT	020		/* Wanted for swapping */

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
#if 0
struct map {
	struct map* next;
	struct map* owner;
	size_t size;
};
#endif




struct map
{
	size_t	m_size;
	caddr_t m_addr;
};
struct	proc {
	struct context *context;     // Switch here to run process
	void 		   *p_wchan;	/* event process is awaiting */
	struct map	   *p_mem;	     // link list of memory owned
	TAILQ_ENTRY(proc) p_link; 	/* linked list of running processes */
	TAILQ_ENTRY(proc) p_hash; 	/* for hash lookup */
	struct proc *parent;         // Parent process
	struct context *tf;        // Trap frame for current syscall

	enum procstate state;
	short		p_stat;
	short		p_flag;
	short		p_pri;		/* priority, negative is high */
	short		p_time;		/* resident time for scheduling */
	short		p_cpu;		/* cpu usage for scheduling */
	short		p_nice;		/* nice for cpu usage */
	uid_t		p_uid;		/* user id, used to direct tty signals */
	gid_t		p_pgrp;		/* name of process group leader */
	pid_t		p_pid;		/* unique process id */
	pid_t		p_ppid;		/* process id of parent */
	short		p_addr;		/* address of swappable image */
	short		p_size;		/* size of swappable image (clicks) */
	sigset_t	p_sig;		/* signals pending to this process */
	caddr_t mem;            // Start of process memory (kernel address)
	size_t sz;              // Size of process memory (bytes)
	caddr_t kstack;         // Bottom of kernel stack for this process
	time_t		p_almtim;	/* time to alarm clock signal */
	char name[16];          // Process name (debugging)
};
TAILQ_HEAD(proc_tq,proc);
extern struct proc proc[];	/* the proc table itself */

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
#define	SLOAD	01		/* in core */
#define	SSYS	02		/* scheduling process */
#define	SLOCK	04		/* process cannot be swapped */
#define	SSWAP	010		/* process is being swapped out */
#define	STRC	020		/* process is being traced */
#define	SWTED	040		/* another tracing flag */
#define	SULOCK	0100		/* user settable lock in core */

/*
 * parallel proc structure
 * to replace part with times
 * to be passed to parent process
 * in ZOMBIE state.
 */
struct	xproc {
	char	xp_stat;
	char	xp_flag;
	char	xp_pri;		/* priority, negative is high */
	char	xp_time;	/* resident time for scheduling */
	char	xp_cpu;		/* cpu usage for scheduling */
	char	xp_nice;	/* nice for cpu usage */
	short	xp_sig;		/* signals pending to this process */
	short	xp_uid;		/* user id, used to direct tty signals */
	short	xp_pgrp;	/* name of process group leader */
	short	xp_pid;		/* unique process id */
	short	xp_ppid;	/* process id of parent */
	short	xp_xstat;	/* Exit status for wait */
	time_t	xp_utime;	/* user time, this proc */
	time_t	xp_stime;	/* system time, this proc */
};




// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

// Per-CPU state
struct user {
  struct context *context;     // Switch here to enter scheduler
  struct proc_tq  procs; // running procs, the head of the queue is the current
  volatile uint32_t booted;        // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  int cmask;				/// create mask
  uid_t u_uid;
  gid_t u_gid;
  int errno;				// user error, ugh
  struct inode 		*cdir;  // Current directory
  struct inode 		*rdir;  // root dir
  size_t u_tsize;		/* text size (bytes) */
  size_t u_dsize;		/* data size (bytes) */
  size_t u_ssize;		/* stack size (bytes) */
  caddr_t heap_start;
  caddr_t heap_end;
  caddr_t stack_start;
  // stack start can be the end of the memory
  // stack end is end of psp
  LIST_HEAD(,file) ofile;     // Open files
};
extern struct map coremap[];
extern struct map swapmap[];
extern struct user u;
#define CPROC (TAILQ_FIRST(&u.procs))

// current process


//void kinit(void);
void init_proc();
void* kmalloc (struct map *mp, size_t size);
void kmfree (struct map *mp, size_t size, void* a);
#endif
