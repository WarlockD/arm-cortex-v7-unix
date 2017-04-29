#ifndef _MXSYS_USER_H_
#define _MXSYS_USER_H_

#include "param.h"
#include "types.h"



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
struct inode;
struct file;
struct tty;
struct user
{
	struct context u_rsav;  /* save r5,r6 when exchanging stacks */
	uint32_t	u_fsav[32];	/* save fp registers */
							/* rsav and fsav must be first in structure */
	char	u_segflg;		/* flag for IO; user or kernel space */
	char	u_error;		/* return error code */
	uid_t	u_uid;			/* effective user id */
	gid_t	u_gid;			/* effective group id */
	uid_t	u_ruid;			/* real user id */
	gid_t	u_rgid;			/* real group id */
	struct proc*	u_procp;/* pointer to proc structure */
	void*	 u_base;		/* base address for IO */
	size_t	 u_count;		/* bytes remaining for IO */
	off_t    u_offset;		/* offset in file for IO */
	struct inode* u_cdir;		/* pointer to inode of current directory */
	struct inode* u_rdir;		/* pointer to inode of current directory */
	//	char	u_dbuf[DIRSIZ];		/* current pathname component */
	const char* u_dirp;		/* current pointer to inode */
	struct	{			/* current directory entry */
		uint16_t		u_ino;
		char	u_name[DIRSIZ];
	} u_dent;
	struct inode* u_pdir;		/* inode of parent directory of dirp */
	int	u_uisa[16];		/* prototype of segmentation addresses */
	int	u_uisd[16];		/* prototype of segmentation descriptors */
	struct file* u_ofile[NOFILE];	/* pointers to file structures of open files */
	size_t	u_tsize;		/* text size (*64) */
	size_t	u_dsize;		/* data size (*64) */
	size_t	u_ssize;		/* stack size (*64) */
	int	u_sep;			/* flag for I and D separation */
	int	u_qsav[2];		/* label variable for quits and interrupts */
	int	u_ssav[2];		/* label variable for swapping */


	const struct sigaction* u_signal[NSIG];		/* disposition of signals */
	time_t	u_utime;		/* this process user time */
	time_t	u_stime;		/* this process system time */
	time_t	u_cutime;		/* sum of childs' utimes */
	time_t	u_cstime;		/* sum of childs' stimes */
	struct context* u_ar0;		/* address of users saved R0 */
	int	u_prof[4];		/* profile arguments */
	char	u_intflg;		/* catch intr from sys */
	struct tty*	u_ttyp;			/* controlling tty pointer */
	dev_t	u_ttyd;			/* controlling tty dev */
					/* kernel stack per user
					 * extends from u + USIZE*64
					 * backward not to reach here
					 */
} __ALIGNTOCLICK;
#define	USIZE (sizeof(struct user))	/* size of user block (*64) */

extern struct user u;
void ksleep(void* chan, int prio);
void kwakeup(void* chan);
void kpanic(const char* msg);
void kprint(const char* msg, int i);
void ktimeset(time_t t);
time_t ktimeget();
//int passc(char c); // subr.c
//int cpass(); // subr.c




#endif
