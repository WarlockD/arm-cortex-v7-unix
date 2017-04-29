// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses three state flags internally:
// * B_BUSY: the block has been returned from bread
//     and has not been passed back to brelse.
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.
#define __KERNEL__
#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "buf.h"
#include "device.h"


struct {
    struct spinlock lock;
    uint8_t      data[NBUF][512];
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // head.next is most recently used.
    TAILQ_HEAD(,buf) free;
    TAILQ_HEAD(,buf) used;
} bcache;
#define BUF_FREE_REMOVE(elm) TAILQ_REMOVE(&bcache.free, elm, hash)
#define BUF_FREE_INSERT(elm) TAILQ_INSERT_HEAD(&bcache.free,elm,hash)
#define BUF_USED_REMOVE(elm) TAILQ_REMOVE(&bcache.used, elm, hash)
#define BUF_USED_INSERT(elm) TAILQ_INSERT_HEAD(&bcache.used,elm,hash)


void binit (void)
{
    initlock(&bcache.lock, "bcache");
    TAILQ_INIT(&bcache.free);
    TAILQ_INIT(&bcache.used);
    int i = 0;
    for_each_array(b, bcache.buf) {
    	BUF_FREE_INSERT(b);
    	b->dev = -1;
    	b->data = bcache.data[i++];
    	b->size = BSIZE;
    }
}

// Look through buffer cache for sector on device dev.
// If not found, allocate fresh block.
// In either case, return B_BUSY buffer.
static struct buf* bget (dev_t dev, daddr_t sector)
{
    struct buf *b;
    acquire(&bcache.lock);
    TAILQ_FOREACH(b,&bcache.used,hash) {
    	if (b->dev == dev && b->sector == sector) {
    		while(ISSET(b->flags,B_BUSY))
    			xv6_sleep(b, &bcache.lock);
			SET(b->flags,B_BUSY);
    		return b;
    	}
    }
	TAILQ_FOREACH(b,&bcache.free,hash) {
		if (ISSET(b->flags,B_BUSY)) continue;
		SET(b->flags,B_BUSY);
		BUF_FREE_REMOVE(b);
		BUF_USED_INSERT(b);
		b->dev = dev;
		b->sector = sector;
		return b;
	}
    panic("bget: no buffers");
    return NULL;
    // no return
}

// Return a B_BUSY buf with the contents of the indicated disk sector.
struct buf* bread (dev_t dev, daddr_t sector)
{
    struct buf *b;

    b = bget(dev, sector);

    if (!(b->flags & B_VALID)) {
    	bdevsw[major(dev)].d_strategy(b);
    }

    return b;
}

// Write b's contents to disk.  Must be B_BUSY.
void bwrite (struct buf *b)
{
    if(!ISSET(b->flags,B_BUSY)) {
    	panic("bwrite");
    }
    b->flags |= B_DIRTY;
    bdevsw[major(b->dev)].d_strategy(b);
}

// Release a B_BUSY buffer.
// Move to the head of the MRU list.
void brelse (struct buf *b)
{
    if (!ISSET(b->flags, B_BUSY)) {
        panic("brelse");
    }
    if(ISSET(b->flags,B_DELAY|B_DIRTY)){
        bdevsw[major(b->dev)].d_strategy(b);
    }
    acquire(&bcache.lock);
    BUF_USED_REMOVE(b);
    BUF_FREE_INSERT(b);
    CLR(b->flags, B_BUSY);
    release(&bcache.lock);
    xv6_wakeup(b);


}

