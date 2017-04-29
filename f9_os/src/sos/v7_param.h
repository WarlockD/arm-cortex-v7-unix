#ifndef _V7_PARM_H_
#define _V7_PARM_H_

// dir.h
#define	DIRSIZ	14
// param.h
// tunables
#define	NBUF	29		/* size of buffer cache */
#define	NINODE	200		/* number of in core inodes */
#define	NFILE	175		/* number of in core file structures */
#define	NMOUNT	8		/* number of mountable file systems */
#define BDEV	10		/* amount of devices*/

// carfule with these
#define	NICINOD	100		/* number of superblock inodes */
#define	NICFREE	50		/* number of superblock free blocks */
#define	NINDIR	(BSIZE/sizeof(daddr_t))
#define	ROOTINO	((ino_t)2)	/* i number of all roots */
#define	SUPERB	((daddr_t)1)	/* block number of the super block */
#define	NMASK	0177		/* NINDIR-1 */
#define	NSHIFT	7		/* LOG2(NINDIR) */
// buffer settings
#define	BSIZE	512		/* size of secondary block (bytes) */
#define	BMASK	(BSIZE-1)	/* BSIZE-1 */
#define	BSHIFT	9		/* LOG2(BSIZE) */

// proc stuff

#define NPROC        64  // maximum number of processes
#define PAGE		1024 // pages size,   n2 please
#define KSTACKSIZE 4*PAGE  // size of per-process kernel stack
#define NDEV         10  // maximum major device number
#define KPAGESHIFT 12
#define KPAGESIZE (1<<KPAGESHIFT)
#define KPAGEMASK (KPAGESIZE-1)

/*
 * priorities
 * probably should not be
 * altered too much
 */
#define	PSWP	0
#define	PINOD	10
#define	PRIBIO	20
#define	PZERO	25
#define	NZERO	20
#define	PPIPE	26
#define	PWAIT	30
#define	PSLEP	40
#define	PUSER	50



#endif
