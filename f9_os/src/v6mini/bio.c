#include "buf.h"
#include "user.h"
#include "proc.h"
#include "conf.h"


/*
 * This is the set of buffers proper, whose heads
 * were declared in buf.h.  There can exist buffer
 * headers not pointing here that are used purely
 * as arguments to the I/O routines to describe
 * I/O to be done-- e.g. swbuf, just below, for
 * swapping.
 */
static char	buffers[NBUF][BSIZE];
static struct buf buf[NBUF];

static struct buf bfreelist;
static void geterror(struct	buf	*rbp);

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */
static void geterror(struct	buf	*bp)
{
	if (bp->b_flags&B_ERROR)
		if ((u.u_error = bp->b_error)==0)
			u.u_error = EIO;
}

/*
 * The following several routines allocate and free
 * buffers with various side effects.  In general the
 * arguments to an allocate routine are a device and
 * a block number, and the value is a pointer to
 * to the buffer header; the buffer is marked "busy"
 * so that no on else can touch it.  If the block was
 * already in core, no I/O need be done; if it is
 * already busy, the process waits until it becomes free.
 * The following routines allocate a buffer:
 *	getblk
 *	bread
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	bawrite
 *	brelse
 */

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf * bread(dev_t dev, daddr_t blkno)
{
	struct buf *rbp = getblk(dev, blkno);
	if (rbp->b_flags&B_DONE)
		return(rbp);
	rbp->b_flags |= B_READ;
	rbp->b_wcount = -256;
	bdevsw[major(dev)].d_strategy(rbp);
	iowait(rbp);
	return(rbp);
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
void bwrite(struct buf *rbp)
{
	int flag = rbp->b_flags;
	rbp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	rbp->b_wcount = -256;
	bdevsw[major(rbp->b_dev)].d_strategy(rbp);
	if ((flag&B_ASYNC) == 0) {
		iowait(rbp);
		brelse(rbp);
	} else if ((flag&B_DELWRI)==0)
		geterror(rbp);
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 */
void bdwrite(struct buf *rbp)
{
	rbp->b_flags |= B_DELWRI | B_DONE;
	brelse(rbp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
void bawrite(struct buf *rbp)
{
	rbp->b_flags |= B_ASYNC;
	bwrite(rbp);
}

/*
 * release the buffer, with no I/O implied.
 */
void brelse(struct buf *rbp)
{
	if (rbp->b_flags&B_WANTED)
		kwakeup(rbp);
	if (bfreelist.b_flags&B_WANTED) {
		bfreelist.b_flags &= ~B_WANTED;
		kwakeup(&bfreelist);
	}
	if (rbp->b_flags&B_ERROR)
		rbp->b_dev = NODEV;  /* no assoc. on error */
	struct buf **backp = &bfreelist.av_back;
	int sps = spl6();
	rbp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC);
	(*backp)->av_forw = rbp;
	rbp->av_back = *backp;
	*backp = rbp;
	rbp->av_forw = &bfreelist;
	splx(sps);
}

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 * When a 512-byte area is wanted for some random reason
 * (e.g. during exec, for the user arglist) getblk can be called
 * with device NODEV to avoid unwanted associativity.
 */
struct buf *getblk(dev_t dev, daddr_t blkno)
{
	struct buf *bp;
	struct buf *dp;

	if(major(dev)>= nblkdev)
		kpanic("getblk");

    loop:
	if (dev < 0)
		dp = &bfreelist;
	else {
		dp = bdevsw[major(dev)].d_tab;
		if(dp == NULL)
			kpanic("getblk");
		for (bp=dp->b_forw; bp != dp; bp = bp->b_forw) {
			if (bp->b_blkno!=blkno || bp->b_dev!=dev)
				continue;
			spl6();
			if (bp->b_flags&B_BUSY) {
				bp->b_flags |= B_WANTED;
				ksleep(bp, PRIBIO);
				spl0();
				goto loop;
			}
			spl0();
			notavail(bp);
			return(bp);
		}
	}
	spl6();
	if (bfreelist.av_forw == &bfreelist) {
		bfreelist.b_flags |= B_WANTED;
		ksleep(&bfreelist, PRIBIO);
		spl0();
		goto loop;
	}
	spl0();
	notavail(bp = bfreelist.av_forw);
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags |= B_ASYNC;
		bwrite(bp);
		goto loop;
	}
	bp->b_flags = B_BUSY | B_RELOC;
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_dev = dev;
	bp->b_blkno = blkno;
	return(bp);
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
void iowait(struct buf *rbp)
{
	spl6();
	while ((rbp->b_flags&B_DONE)==0)
		ksleep(rbp, PRIBIO);
	spl0();
	geterror(rbp);
}

/*
 * Unlink a buffer from the available list and mark it busy.
 * (internal interface)
 */
void notavail(struct buf *rbp)
{
	int sps = spl6();
	rbp->av_back->av_forw = rbp->av_forw;
	rbp->av_forw->av_back = rbp->av_back;
	rbp->b_flags |= B_BUSY;
	splx(sps);
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
void iodone(struct buf *rbp)
{
	rbp->b_flags |= B_DONE;
	if (rbp->b_flags&B_ASYNC)
		brelse(rbp);
	else {
		rbp->b_flags &= ~B_WANTED;
		kwakeup(rbp);
	}
}

/*
 * Zero the core associated with a buffer.
 */
void clrbuf(struct buf *bp)
{
	memset(bp->b_addr,0,BSIZE);
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
void binit()
{
	struct buf *bp;
	struct buf *dp;
	int i;
	struct bdevsw *bdp;

	bfreelist.b_forw = bfreelist.b_back =
	    bfreelist.av_forw = bfreelist.av_back = &bfreelist;
	for (i=0; i<NBUF; i++) {
		bp = &buf[i];
		bp->b_dev = -1;
		bp->b_addr = buffers[i];
		bp->b_back = &bfreelist;
		bp->b_forw = bfreelist.b_forw;
		bfreelist.b_forw->b_back = bp;
		bfreelist.b_forw = bp;
		bp->b_flags = B_BUSY;
		brelse(bp);
	}
	i = 0;
	nblkdev= 0;
	for (bdp = bdevsw; bdp->d_open; bdp++) {
		dp = bdp->d_tab;
		if(dp) {
			dp->b_forw = dp;
			dp->b_back = dp;
		}
		i++;
	}
	nblkdev = i;
}

/*
 * swap I/O
 */

#define USTACK	((TOPSYS)-sizeof(struct context))
#define PTOU(V) ((uintptr_t)(V))

int swap(struct proc *pp, int rdflg) {
#if 0
	static struct buf swbuf;
	//struct proc *rp
	uintptr_t p1;
	uintptr_t p2;
	//pp = rp;
	if(rdflg == B_WRITE) {
		// ok, what we do is we cpoy the stack into alligned user space
		// intresting, fix this!  We cna make this work!
		p1 = PTOU(USTACK);
		p2 = PTOU(TOPSYS) + (u.u_dsize<<6) + (p1&077);
		if(p2 <= p1) {
			pp->p_size = u.u_dsize + USIZE +
			    ((PTOU(TOPUSR)>>6)&01777) - ((PTOU(p1)>>6)&01777);
			while(p1 < PTOU(TOPUSR))
				*p2++ = *p1++;
		} else
			pp->p_size = SWPSIZ<<3;

	}
	spl6();
	swbuf.b_flags = B_BUSY | rdflg;
	swbuf.b_dev = swapdev;
	swbuf.b_wcount = -(((pp->p_size+7)&~07)<<5);	/* 32 words per block */
	swbuf.b_blkno = SWPLO+rp->p_pid*SWPSIZ;
	swbuf.b_addr = &u;	/* 64 b/block */
	bdevsw[major(swapdev)].d_strategy(&swbuf);
	spl6();
	while((swbuf.b_flags&B_DONE)==0)
		idle();
	spl0();
	if(rdflg == B_READ) {
		p1 = TOPUSR;
		p2 = (pp->p_size<<6) + TOPSYS - (USIZE<<6);
		if(p2 <= p1)
			while(p1 >= **USTACK)
				*--p1 = *--p2;
	}
	swbuf.b_flags =& ~(B_BUSY|B_WANTED);
#endif
	return 1; //( swbuf.b_flags&B_ERROR);
}

/*
 * make sure all write-behind blocks
 * on dev (or NODEV for all)
 * are flushed out.
 * (from umount and update)
 */
void bflush(dev_t dev)
{
	struct buf *bp;
loop:
	spl6();
	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
		if ((bp->b_flags&B_DELWRI) && (dev == NODEV||dev==bp->b_dev)) {
			bp->b_flags |= B_ASYNC;
			notavail(bp);
			bwrite(bp);
			goto loop;
		}
	}
	spl0();
}


