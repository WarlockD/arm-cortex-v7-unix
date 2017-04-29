/*
 * buf.cpp
 *
 *  Created on: Apr 10, 2017
 *      Author: warlo
 */

#if 0
#include "buf.h"
#include <cstring>
#include <cerrno>

#define	BUFHSZ	16	/* must be power of 2 */
#define	BUFHASH(dev,blkno)	(&buckets[((long)(dev) + blkno) & ((long)(BUFHSZ - 1))])
#define PRIBIO irq_prio::bio
namespace v6 {
class _internal_buf {
public:
#define ibuf block_buf::_ibuf
	//using ibuf = block_buf::_ibuf;
	std::atomic_flag lock;
	typedef TAILQ_HEAD(_buf_head,block_buf::_ibuf) buf_head_t;
	uint8_t block_buffers[block_buf::BUFCONT][block_buf::BKSIZE];
	ibuf buf_array[BUFHSZ];
	block_buf::block_buf_strat buf_strats[block_buf::BUFSTRATCOUNT];
	buf_head_t freelist;
	buf_head_t buckets[BUFHSZ];
	buf_flags_t freelistflags;
	inline buf_head_t* bucket(dev_t dev, daddr_t blkno){ return BUFHASH(dev,blkno); }
	_internal_buf() {
		lock.test_and_set();
		freelistflags = B_DONE;
		::memset(&buf_array,0,sizeof(buf_array));
		::memset(&buf_strats,0,sizeof(buf_strats));
		TAILQ_INIT(&freelist);
		for(buf_head_t& bucket : buckets) TAILQ_INIT(&bucket);
		for(ibuf& br : buf_array){
			TAILQ_INSERT_HEAD(&freelist, &br, hash);
			br.dev = NODEV;
			br.flags = B_DONE;
		}
		lock.clear();
	}
	/*
	 * See if the block is associated with some buffer
	 * (mainly to avoid getting hung up on a wait in breada)
	 */
	bool incore(dev_t dev,daddr_t blkno)
	{
		struct ibuf *bp;
		TAILQ_FOREACH(bp, BUFHASH(dev, blkno), hash)
			if (bp->blkno==blkno && bp->dev==dev) return true;
		return false;
	}
	/*
	 * Wait for I/O completion on the buffer; return errors
	 * to the user.
	 */
	void check_error(ibuf* bp) {
		if(ISSET(bp->flags,B_ERROR)) seterrno(EIO);
	}
	void iowait (ibuf *bp)
	{
		splbio();
		while (ISSET(bp->flags,B_DONE)==0)
			sleep(bp, PRIBIO);
		splnormal();
		check_error(bp);
	}
	/*
	 * Unlink a buffer from the available list and mark it busy.
	 * (internal interface)
	 */
	void notavail (ibuf *bp)
	{
		splclocklck;
		TAILQ_REMOVE(&freelist, bp, hash);
		bp->flags |= B_BUSY;
	}
	/*
	 * Read in (if necessary) the block and return a buffer pointer.
	 */
	ibuf * bread (dev_t dev, daddr_t blkno)
	{
		ibuf *bp= getblk(dev, blkno);
		if (ISSET(bp->flags,B_DONE)) {
	#ifdef	DISKMON
			io_info.ncache++;
	#endif
			return(bp);
		}
		bp->flags |= B_READ;
		bp->tcount = bp->size;
		block_buf d(bp);
		buf_strats[major(dev)](d);
	#ifdef	DISKMON
		io_info.nread++;
	#endif
		iowait(bp);
		return(bp);
	}
	/*
	 * Find a buffer which is available for use.
	 * Select something from a free list.
	 * Preference is to AGE list, then LRU list.
	 */
	ibuf *getnewbuf()
	{
		ibuf *bp, *dp;
		int s;

	loop:
	do {
		int s = splbio();
		while(TAILQ_EMPTY(&freelist)) {
			freelistflags |= B_WANTED;
			sleep(&freelist, PRIBIO+1);
		}
		bp = TAILQ_FIRST(&freelist);

		if (bp->flags & B_DELWRI) {
			bawrite(bp);
			continue;
		}
		notavail(bp);
	} while(0);

		if (bp->b_flags & B_DELWRI) {
			bawrite(bp);
			goto loop;
		}

		splx(s);
		iowait
		for (dp = &bfreelist[BQ_AGE]; dp > bfreelist; dp--)
			if (dp->av_forw != dp)
				break;
		if (dp == bfreelist) {		/* no free blocks */
			dp->b_flags |= B_WANTED;
			sleep((caddr_t)dp, PRIBIO+1);
			splx(s);
			goto loop;
		}
		splx(s);
		bp = dp->av_forw;

		if (bp->b_flags & B_DELWRI) {
			bawrite(bp);
			goto loop;
		}
		if(bp->b_flags & (B_RAMREMAP|B_PHYS)) {
			register memaddr paddr;	/* click address of real buffer */
			extern memaddr bpaddr;

	#ifdef DIAGNOSTIC
			if ((bp < &buf[0]) || (bp >= &buf[nbuf]))
				panic("getnewbuf: RAMREMAP bp addr");
	#endif
			paddr = bpaddr + btoc(DEV_BSIZE) * (bp - buf);
			bp->b_un.b_addr = (caddr_t)(paddr << 6);
			bp->b_xmem = (paddr >> 10) & 077;
		}
		trace(TR_BRELSE);
		bp->b_flags = B_BUSY;
		return (bp);
	}

	/*
		 * get an empty block,
		 * not assigned to any particular device
		 */
	struct ibuf * geteblk (dev_t dev=NODEV, daddr_t blkno=0)
	{

	loop:
		splclock();
		while(TAILQ_EMPTY(&bfreelist)) {
			bfreelistflags |= B_WANTED;
			ksleep(&bfreelist, PRIBIO+1);
		}
		splnormal();
		notavail(bp = TAILQ_FIRST(&bfreelist));
		if (bp->b_flags & B_DELWRI) {
			bp->b_flags |= B_ASYNC;
			bwrite(bp);
			goto loop;
		}
		bp->flags = B_BUSY;
		//  BSIZE;
		bp->b_un.b_addr = bp->b_data;
		bp->b_size = sizeof(bp->b_data);
		if(bp->b_dev != NODEV) TAILQ_REMOVE(TAILQ_HEAD_DEV(bp->b_dev), bp, b_tab);
		bp->b_dev = NODEV;
		return(bp);
	}
	ibuf * getblk (dev_t dev, daddr_t blkno){
		struct ibuf* bp;
		TAILQ_FOREACH(bp, bucket(dev,blkno), hash){
			if (bp->blkno==blkno || bp->dev==dev) return bp;// sleep is handled somewhere else
		}
		while(lock.test_and_set()); // simple locking for now

		// not found get a free one
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
	}
};
static _internal_buf g_buf;


void block_buf::install_dev(dev_t dev, block_buf::block_buf_strat func){
	g_buf.buf_strats[major(dev)] = func; // should flush all buffers first
}

block_buf::block_buf(dev_t dev, daddr_t blockno){ // load a buffer at blockno

}
block_buf::block_buf(_ibuf* b){ // special cas for static_buf

}
block_buf::~block_buf(){

}

}

#endif
