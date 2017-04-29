#ifndef _MXSYS_BUF_H_
#define _MXSYS_BUF_H_

#include "types.h"

struct proc;

enum buf_flags {
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

struct uio {
	void* base;
	size_t count;
	off_t offset;
};
int uio_put(int c, struct uio* ui);
int uio_get(struct uio* ui);
int uio_read(struct uio* ui, void* output, size_t count);
int uio_write(struct uio* ui, const void* input, size_t count);
#define UIO(BASE,SIZE) { .base = (void)(BASE), .count = (SIZE), .offset = 0 }

struct buf
{
	enum buf_flags	b_flags;		/* see defines below */
	struct	 buf *b_forw;		/* headed by devtab of b_dev */
	struct	 buf *b_back;		/*  "  */
	struct	 buf *av_forw;		/* position on free list, */
	struct	 buf *av_back;		/*     if not BUSY*/
	dev_t	 b_dev;			/* major+minor device name */
	size_t	 b_wcount;		/* transfer count (usu. words) */
	caddr_t	 b_addr;		/* low order core address */
	daddr_t	 b_blkno;		/* block # on device */
	uint16_t b_error;		/* returned after I/O */
	uint16_t b_resid;		/* words not transferred after error */
};

#define d_actf av_forw		/* head of I/O queue */
#define d_actl av_back		/* tail of I/O queue */
struct 	buf *d_actl;
/*
 * Each block device has a devtab, which contains private state stuff
 * and 2 list heads: the b_forw/b_back list, which is doubly linked
 * and has all the buffers currently associated with that major
 * device; and the d_actf/d_actl list, which is private to the
 * device but in fact is always used for the head and tail
 * of the I/O queue for the device.
 * Various routines in bio.c look at b_forw/b_back
 * (notice they are the same as in the buf structure)
 * but the rest is private to each device driver.
 */
#if 0
struct devtab
{
	char	d_active;		`/* busy flag */
	char	d_errcnt;		/* error count (for recovery) */
	struct	buf *b_forw;		/* first buffer for this dev */
	struct	buf *b_back;		/* last buffer for this dev */
	struct	buf *d_actf;		/* head of I/O queue */
	struct 	buf *d_actl;		/* tail of I/O queue */
};
#endif

void binit();
struct buf * getblk(dev_t dev, daddr_t blkno);
struct buf * bread(dev_t dev, daddr_t blkno);
void bwrite(struct buf *bp);
void bdwrite(struct buf *rbp);
void bawrite(struct buf *rbp);
void brelse(struct buf *bp);
void iowait(struct buf *rbp);
void notavail(struct buf *rbp);
void iodone(struct buf *rbp);
void bflush(dev_t dev);
int swap(struct proc *pp, int rdflg);
void clrbuf(struct buf *bp);


#endif
