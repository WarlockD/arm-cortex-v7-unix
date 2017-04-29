#include "v7_param.h"
#include "v7_bio.h"
#include <stdatomic.h>
#include "v7_internal.h"
#include <errno.h>

//static char buffers[NBUF][BSIZE];	// raw buffer pool
static struct buf buf[NBUF];		/* The buffer pool itself */
static b_flags_t bfreelistflags = 0;
static struct buf_tq bfreelist = TAILQ_HEAD_INITIALIZER(bfreelist);		/* head of available list */
struct bdevsw bdevsw[BDEV];
struct cdevsw cdevsw[BDEV];
daddr_t rablock=0; // read ahead block in bio.c
//#define BUFNUM(BP)  ((size_t)((((caddr_t)BP) - ((caddr_t)buf)))/sizeof(struct buf))
#define TAILQ_HEAD_DEV(DEV) (&bdevsw[major(DEV)].d_tab)
#define FOREACH_BUF_DEV(VAR,DEV) TAILQ_FOREACH(VAR, TAILQ_HEAD_DEV(DEV), b_tab)


// have some safetys
#if 0

#else


static inline void do_strat(struct buf* bp) {
	if(!bp || bdevsw[major(bp->b_dev)].d_strategy == NULL)
		panic("No strat on dev %d\n",bp->b_dev);
	bdevsw[major(bp->b_dev)].d_strategy(bp);
}


#endif


//#define	DISKMON	0

#ifdef	DISKMON
struct {
	int	nbuf;
	long	nread;
	long	nreada;
	long	ncache;
	long	nwrite;
	long	bufcount[NBUF];
} io_info;
#endif

void binit() {
	bfreelistflags = 0;
	memset(&buf,0,sizeof(buf));
	TAILQ_INIT(&bfreelist);
	foreach_array(bp, buf) { TAILQ_INSERT_HEAD(&bfreelist, bp, b_aval); bp->b_dev = NODEV; }
	foreach_array(bd, bdevsw) TAILQ_INIT(&bd->d_tab);
}
void install_blockdev(dev_t dev, struct bdevsw* drv){
	if(dev < 0 || dev > BDEV) panic("install_blockdev: dev out of range");
	bdevsw[major(dev)] = *drv;
	TAILQ_INIT(&bdevsw[major(dev)].d_tab);
}

/*
 * The following several routines allocate and free
 * buffers with various side effects.  In general the
 * arguments to an allocate routine are a device and
 * a block number, and the value is a pointer to
 * to the buffer header; the buffer is marked "busy"
 * so that no one else can touch it.  If the block was
 * already in core, no I/O need be done; if it is
 * already busy, the process waits until it becomes free.
 * The following routines allocate a buffer:
 *	getblk
 *	bread
 *	breada
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	bawrite
 *	brelse
 */


/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 */
static int incore (dev_t dev, daddr_t blkno)
{
	struct buf *bp;
	FOREACH_BUF_DEV(bp,dev)  {
		if (bp->b_blkno==blkno && bp->b_dev==dev)
			return(1);
	}
	return(0);
}
/*
 * Unlink a buffer from the available list and mark it busy.
 * (internal interface)
 */
static void  notavail (struct buf *bp)
{
	int s = spl6();
	TAILQ_REMOVE(&bfreelist, bp, b_aval);;
	bp->b_flags |= B_BUSY;
	splx(s);
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */
#undef errno
extern int errno;
static void geterror (register struct buf *bp)
{
	if (bp->b_flags & B_ERROR) {
		errno= bp->b_error == 0 ? EIO : bp->b_error;
	}
}

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *bread (dev_t dev, daddr_t blkno)
{
	struct buf *bp;

	bp = getblk(dev, blkno);
	if (bp->b_flags&B_DONE) {
#ifdef	DISKMON
		io_info.ncache++;
#endif
		return(bp);
	}
	bp->b_flags |= B_READ;
	bp->b_bcount = bp->b_size; //BSIZE;
	do_strat(bp);
#ifdef	DISKMON
	io_info.nread++;
#endif
	iowait(bp);
	return(bp);
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf *breada (dev_t dev, daddr_t blkno, daddr_t rablkno)
{
	struct buf *bp, *rabp;
	bp = NULL;
	if (!incore(dev, blkno)) {
		bp = getblk(dev, blkno);
		if ((bp->b_flags&B_DONE) == 0) {
			bp->b_flags |= B_READ;
			bp->b_bcount = bp->b_size; //BSIZE;
			do_strat(bp);
#ifdef	DISKMON
			io_info.nread++;
#endif
		}
	}
	if (rablkno && !incore(dev, rablkno)) {
		rabp = getblk(dev, rablkno);
		if (rabp->b_flags & B_DONE)
			brelse(rabp);
		else {
			rabp->b_flags |= B_READ|B_ASYNC;
			rabp->b_bcount = bp->b_size; //BSIZE;
			do_strat(rabp);
#ifdef	DISKMON
			io_info.nreada++;
#endif
		}
	}
	if(bp == NULL)
		return(bread(dev, blkno));
	iowait(bp);
	return(bp);
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
void bwrite (register struct buf *bp)
{
	b_flags_t flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI | B_AGE);
	bp->b_bcount = bp->b_size; //BSIZE;
#ifdef	DISKMON
	io_info.nwrite++;
#endif
	do_strat(bp);
	if ((flag&B_ASYNC) == 0) {
		iowait(bp);
		brelse(bp);
	} else if (flag & B_DELWRI)
		bp->b_flags |= B_AGE;
	else
		geterror(bp);
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */
void bdwrite (struct buf *bp)
{
	if(TAILQ_FIRST(TAILQ_HEAD_DEV(bp->b_dev))->b_flags & B_TAPE)
		bawrite(bp);
	else {
		bp->b_flags |= B_DELWRI | B_DONE;
		brelse(bp);
	}
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
void bawrite (struct buf *bp)
{
	bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * release the buffer, with no I/O implied.
 */
void brelse (struct buf *bp)
{
	if (bp->b_flags&B_WANTED)
		kwakeup(bp);
	if (bfreelistflags&B_WANTED) {
		bfreelistflags &= ~B_WANTED;
		kwakeup(&bfreelist);
	}
	if (bp->b_flags&B_ERROR)
		bp->b_dev = NODEV;  /* no assoc. on error */
	int s = spl6();
	if(bp->b_flags & B_AGE) {
		TAILQ_INSERT_HEAD(&bfreelist, bp, b_aval);
	} else {
		TAILQ_INSERT_TAIL(&bfreelist, bp, b_aval);
	}
	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC|B_AGE);
	splx(s);
}



/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
struct buf * getblk (dev_t dev, daddr_t blkno)
{
	struct buf *bp;
#ifdef	DISKMON
	int i;
#endif

	//if(major(dev) >= nblkdev)
	//	panic("blkdev");

    loop:
	spl0();

	if(bdevsw[major(dev)].d_strategy == NULL) panic("devtab");
	FOREACH_BUF_DEV(bp,dev) {
		if (bp->b_blkno!=blkno || bp->b_dev!=dev)
			continue;
		spl6();
		if (bp->b_flags&B_BUSY) {
			bp->b_flags |= B_WANTED;
			ksleep(bp, PRIBIO+1);
			goto loop;
		}
		spl0();
#ifdef	DISKMON
		i = 0;
		dp = bp->av_forw;
		while (dp != &bfreelist) {
			i++;
			dp = dp->av_forw;
		}
		if (i<NBUF)
			io_info.bufcount[i]++;
#endif
		notavail(bp);
		return(bp);
	}
	spl6();
	if(TAILQ_EMPTY(&bfreelist)) {
		bfreelistflags |= B_WANTED;
		ksleep(&bfreelist, PRIBIO+1);
		goto loop;
	}
	spl0();
	notavail(bp = TAILQ_FIRST(&bfreelist));
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags |= B_ASYNC;
		bwrite(bp);
		goto loop;
	}
	bp->b_flags = B_BUSY;
	bp->b_size =  BSIZE;
	bp->b_un.b_addr = bp->b_data;
	if(bp->b_dev != NODEV) TAILQ_REMOVE(TAILQ_HEAD_DEV(bp->b_dev), bp, b_tab);
	if(bp->b_dev != dev) TAILQ_INSERT_HEAD(TAILQ_HEAD_DEV(dev), bp, b_tab);
	bp->b_dev = dev;
	bp->b_blkno = blkno;
	return(bp);
}

/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf * geteblk (void)
{
	struct buf *bp;
loop:
	spl6();
	while(TAILQ_EMPTY(&bfreelist)) {
		bfreelistflags |= B_WANTED;
		ksleep(&bfreelist, PRIBIO+1);
	}
	spl0();
	notavail(bp = TAILQ_FIRST(&bfreelist));
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags |= B_ASYNC;
		bwrite(bp);
		goto loop;
	}
	bp->b_flags = B_BUSY;
	//  BSIZE;
	bp->b_un.b_addr = bp->b_data;
	bp->b_size = sizeof(bp->b_data);
	if(bp->b_dev != NODEV) TAILQ_REMOVE(TAILQ_HEAD_DEV(bp->b_dev), bp, b_tab);
	bp->b_dev = NODEV;
	return(bp);
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
void iowait (struct buf *bp)
{
	spl6();
	while ((bp->b_flags&B_DONE)==0)
		ksleep(bp, PRIBIO);
	spl0();
	geterror(bp);
}



/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
void iodone (struct buf *bp)
{
	// b_map is handled in getblock

#if 0
	if(bp->b_flags&B_MAP) {
		// map just uses a direct pointer
		// we clear the pointer and give it the original buffer block
		bp->b_flags &= ~B_MAP;
		bp->b_flags = B_BUSY;
		bp->b_size =  BSIZE; // reset it to a normal buffer
	}
#endif
	bp->b_flags |= B_DONE;
	if (bp->b_flags&B_ASYNC)
		brelse(bp);
	else {
		bp->b_flags &= ~B_WANTED;
		kwakeup(bp);
	}
}
static size_t btow(size_t bytes) {
	size_t words = bytes>>2;
	if(bytes&2) ++words;
	return words;
}
/*
 * Zero the core associated with a buffer.
 */
void clrbuf (struct buf *bp)
{
	size_t words = btow(bp->b_size);
	uint32_t* w = (uint32_t*)bp->b_un.b_addr;
	while(--words) *w++ = 0;
}

void cpybuf (struct buf *dst, const struct buf* src)
{
	if(dst->b_size != src->b_size) panic("cpybuf: size mismatch");
	size_t words = btow(dst->b_size);
	uint32_t* d = (uint32_t*)dst->b_un.b_addr;
	const uint32_t* s = (const uint32_t*)src->b_un.b_addr;
	while(--words) *d++ = *s++;
}

// we swap from one location in memory to the drive, raw copy
void swap (dev_t dev, daddr_t blkno, caddr_t coreaddr, size_t count, int rdflg)
{
	size_t tcount;
	struct buf swbuf;
	spl6();
	while (count) {
		tcount = count;
		if(tcount > BSIZE) tcount = BSIZE;
		swbuf.b_flags = B_BUSY | B_PHYS | rdflg;
		swbuf.b_dev = dev;
		swbuf.b_bcount = tcount;
		swbuf.b_blkno = blkno;
		swbuf.b_un.b_addr = coreaddr;
		do_strat(&swbuf);
		if (swbuf.b_flags & B_ERROR)
			panic("IO err in swap");
		spl6(); // force back?
		while((swbuf.b_flags&B_DONE)==0)
			ksleep(&swbuf, PSWP);
		count -= tcount;
		coreaddr += tcount;
		blkno += btod(tcount);

	}
	spl0();
}

/*
 * make sure all write-behind blocks
 * on dev (or NODEV for all)
 * are flushed out.
 * (from umount and update)
 */
void bflush (dev_t dev)
{

loop:
	spl6();
	struct buf *bp;//, *pb;
	//TAILQ_FOREACH_SAFE
	// might be able to get rid of loop: with TAILQ_FOREACH_SAFE but if we
	// it might go crazy?
	TAILQ_FOREACH(bp, &bfreelist, b_aval) {
		if ((bp->b_flags&B_DELWRI) && (dev == NODEV||dev==bp->b_dev)) {
			bp->b_flags |= B_ASYNC;
			notavail(bp);
			bwrite(bp);
			goto loop;
		}
	}
	spl0();
}

void physio (void (*strat)(struct buf*), struct buf *bp, dev_t dev, int rw){
	// don't do anything right now

}

#if 0
// don't really need this now do we
/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 */
void  physio(uint32_t* base, size_t size, int (*strat)(struct buf), struct buf *bp, dev_t dev, int rw)
{
	register int nb;
	int ts;

	base = (unsigned)u.u_base;
	/*
	 * Check odd base, odd count, and address wraparound
	 */
	if (base&01 || u.u_count&01 || base>=base+u.u_count)
		goto bad;
	ts = (u.u_tsize+127) & ~0177;
	if (u.u_sep)
		ts = 0;
	nb = (base>>6) & 01777;
	/*
	 * Check overlap with text. (ts and nb now
	 * in 64-byte clicks)
	 */
	if (nb < ts)
		goto bad;
	/*
	 * Check that transfer is either entirely in the
	 * data or in the stack: that is, either
	 * the end is in the data or the start is in the stack
	 * (remember wraparound was already checked).
	 */
	if ((((base+u.u_count)>>6)&01777) >= ts+u.u_dsize
	    && nb < 1024-u.u_ssize)
		goto bad;
	spl6();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO+1);
	}
	bp->b_flags = B_BUSY | B_PHYS | rw;
	bp->b_dev = dev;
	/*
	 * Compute physical address by simulating
	 * the segmentation hardware.
	 */
	ts = (u.u_sep? UDSA: UISA)->r[nb>>7] + (nb&0177);
	bp->b_un.b_addr = (caddr_t)((ts<<6) + (base&077));
	bp->b_xmem = (ts>>10) & 077;
	bp->b_blkno = u.u_offset >> BSHIFT;
	bp->b_bcount = u.u_count;
	bp->b_error = 0;
	u.u_procp->p_flag |= SLOCK;
	(*strat)(bp);
	spl6();
	while ((bp->b_flags&B_DONE) == 0)
		sleep((caddr_t)bp, PRIBIO);
	u.u_procp->p_flag &= ~SLOCK;
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	spl0();
	bp->b_flags &= ~(B_BUSY|B_WANTED);
	u.u_count = bp->b_resid;
	geterror(bp);
	return;
    bad:
	u.u_error = EFAULT;
}
#endif


