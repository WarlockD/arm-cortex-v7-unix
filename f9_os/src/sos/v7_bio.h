#ifndef _V7_BIO_H_
#define _V7_BIO_H_

#include "v7_types.h"



/*
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * the device with which it is currently associated (always)
 * and also on a list of blocks available for allocation
 * for other use (usually).
 * The latter list is kept in last-used order, and the two
 * lists are doubly linked to make it easy to remove
 * a buffer from one list when it was found by
 * looking through the other.
 * A buffer is on the available list, and is liable
 * to be reassigned to another disk block, if and only
 * if it is not marked BUSY.  When a buffer is busy, the
 * available-list pointers can be used for other purposes.
 * Most drivers use the forward ptr as a link in their I/O
 * active queue.
 * A buffer header contains all the information required
 * to perform I/O.
 * Most of the routines which manipulate these things
 * are in bio.c.
 */


/*
 * These flags are kept in b_flags.
 */
#if 0
typedef enum  {
	B_WRITE		=0,	/* non-read pseudo-flag */
	B_READ		=01,	/* read when I/O occurs */
	B_DONE		=02,	/* transaction finished */
	B_ERROR		=04	,/* transaction aborted */
	B_BUSY		=010,	/* not on av_forw/back list */
	B_PHYS		=020,	/* Physical IO potentially using UNIBUS map */
	B_MAP		=040,	/* This block has the UNIBUS map allocated */
	B_WANTED 	=0100,	/* issue wakeup when BUSY goes off */
	B_AGE		=0200,	/* delayed write for correct aging */
	B_ASYNC		=0400,	/* don't wait for I/O completion */
	B_DELWRI 	=01000,	/* don't write till block leaves available list */
	B_TAPE 		=02000,	/* this is a magtape (no bdwrite) */
	B_PBUSY		=04000,
	B_PACK		=010000
}b_flags_t;
#else
#define B_WRITE		0	/* non-read pseudo-flag */
#define B_READ		01	/* read when I/O occurs */
#define B_DONE		02	/* transaction finished */
#define B_ERROR		04	/* transaction aborted */
#define B_BUSY		010	/* not on av_forw/back list */
#define B_PHYS		020	/* Physical IO potentially using UNIBUS map */
#define B_MAP		040	/* This block has the UNIBUS map allocated */
#define B_WANTED 	0100	/* issue wakeup when BUSY goes off */
#define B_AGE		0200	/* delayed write for correct aging */
#define B_ASYNC		0400	/* don't wait for I/O completion */
#define B_DELWRI 	01000	/* don't write till block leaves available list */
#define B_TAPE 		02000	/* this is a magtape (no bdwrite) */
#define B_PBUSY		04000
#define B_PACK		010000
typedef int b_flags_t;
#endif

TAILQ_HEAD(buf_tq, buf);
struct buf
{
	b_flags_t	b_flags;		/* see defines below */
	//LIST_ENTRY(buf)  b_hash;	// hash list for lookup agesnt dev/blockno
	TAILQ_ENTRY(buf) b_tab;	// owning list, free, in the driver etc
	TAILQ_ENTRY(buf) b_aval;	// position on the free list
	dev_t	b_dev;			/* major+minor device name */
	size_t 	b_bcount;		/* transfer count */
	size_t  b_size;			// size of b_addr, usally block size but diffrent if map
	daddr_t	b_blkno;		/* block # on device */
	char	b_error;		/* returned after I/O */
	unsigned int b_resid;		/* words not transferred after error */
	union {
	    caddr_t b_addr;		/* low order core address */
	    struct filsys *b_filsys;	/* superblocks */
	    struct dinode *b_dino;	/* ilist */
	    struct dirent* b_dirent; /* dir records */
	    daddr_t *b_daddr;		/* indirect block */
	} b_un;
	char b_data[BSIZE]; // the buffer data, can be used for extra stuff
};



void binit();
void install_blockdev(dev_t dev, struct bdevsw* drv);
struct buf *getblk (dev_t dev, daddr_t blkno);
struct buf *geteblk ();
struct buf *bread (dev_t dev, daddr_t blkno);
struct buf *breada (dev_t dev, daddr_t blkno, daddr_t rablkno);
void bwrite (struct buf *bp);
void bdwrite (struct buf *bp);
void bawrite (struct buf *bp);
void brelse (struct buf *bp);
struct buf* getblk(dev_t dev, daddr_t blkno);
void iowait (struct buf *bp);
void iodone (struct buf *bp);
void bflush (dev_t dev); // NODEV is all
void physio (void (*strat)(struct buf*), struct buf *bp, dev_t dev, int rw);
void clrbuf(struct buf* bp);
#endif
