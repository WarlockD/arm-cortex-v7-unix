#ifndef _MXSYS_PARAM_H_
#define _MXSYS_PARAM_H_
/*
 * tunable variables
 */

#define	NBUF	6		/* size of buffer cache */
#define	NINODE	50		/* number of in core inodes */
#define	NFILE	50		/* number of in core file structures */
#define	NMOUNT	2		/* number of mountable file systems */
#define	SSIZE	20		/* initial stack size (*64 bytes) */
#define	NOFILE	15		/* max open files per process */
#define	CANBSIZ	256		/* max size of typewriter line */
#define	NCALL	4		/* max simultaneous time callouts */
#define	NPROC	13		/* max number of processes */
#define	NCLIST	100		/* max total clist size */
#define	HZ	60			/* Ticks/second of the clock */
#define DEVMAX 10		/* max number of devices */
#define	CBSIZE	6		/* number of chars in a clist block */
#define	CROUND	07		/* clist rounding: sizeof(int *) + CBSIZE - 1*/

#define NODEV ((dev_t)-1)
#define	major(x)  	(int)((((dev_t)(x))>>8))
#define	minor(x)  	(int)(((dev_t)(x))&0377)
#define	makedev(x,y)	(dev_t)((x)<<8|(y))
/*
 * priorities
 * probably should not be
 * altered too much
 */

#define	PSWP	-100
#define	PINOD	-90
#define	PRIBIO	-50
#define	PPIPE	1
#define	PWAIT	40
#define	PSLEP	90
#define	PUSER	100

/*
 * signals
 * dont change
 */
// defined in signal.h

/*
 * fundamental constants
 * cannot be changed
 */
#define CLICKSIZE 64
#define __ALIGNTOCLICK __attribute__((aligned(CLICKSIZE)))
#define	ROOTINO	1			/* i number of all roots */
#define	DIRSIZ	14			/* max characters per directory */
#define BSHIFT 9   		/* shift for block size */
#define BSIZE (1<<BSHIFT) /* 512 */
#define BMASK (BSIZE-1)

/* inumber to disk address */
#define	itod(x)	(daddr_t)((((unsigned)x+15)>>3))
//  ldiv(ip->i_number+31, 16)

/* inumber to disk offset */
#define	itoo(x)	(int)((x+15)&07)
// 32*lrem(ip->i_number+31, 16)
// byte offset to block onumber
#define otob(x)  (daddr_t)((((unsigned)(x)) << 9))
/*
 * Certain processor registers
 */
//#define PS	0177776
//#define KL	0177560
//#define SW	0177570

/*
 * configuration dependent variables
 */

#define UCORE	(16*32)
//#define TOPSYS	12*2048
//#define TOPUSR	28*2048
#define ROOTDEV 0
#define SWAPDEV 0
#define SWPLO	4000
#define NSWAP	872
#define SWPSIZ	66

#endif
