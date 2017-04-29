#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "fs.h"
#include "buf.h"

// Simple logging. Each system call that might write the file system
// should be surrounded with begin_trans() and commit_trans() calls.
//
// The log holds at most one transaction at a time. Commit forces
// the log (with commit record) to disk, then installs the affected
// blocks to disk, then erases the log. begin_trans() ensures that
// only one system call can be in a transaction; others must wait.
//
// Allowing only one transaction at a time means that the file
// system code doesn't have to worry about the possibility of
// one transaction reading a block that another one has modified,
// for example an i-node block.
//
// Read-only system calls don't need to use transactions, though
// this means that they may observe uncommitted data. I-node and
// buffer locks prevent read-only calls from seeing inconsistent data.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing sector #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged sector #s before commit.
struct logheader {
    int n;
    int sector[LOGSIZE];
};

struct xv6_log {
    struct spinlock lock;
    int start;
    int size;
    int busy; // a transaction is active
    int dev;
    struct logheader lh;
};
struct xv6_log fs_log;

static void recover_from_log(void);

void initlog(void)
{
    struct superblock sb;

    if (sizeof(struct logheader) >= BSIZE) {
        panic("initlog: too big logheader");
    }

    initlock(&fs_log.lock, "log");
    readsb(ROOTDEV, &sb);
    fs_log.start = sb.size - sb.nlog;
    fs_log.size = sb.nlog;
    fs_log.dev = ROOTDEV;
    recover_from_log();
}

// Copy committed blocks from log to their home location
static void install_trans(void)
{
    int tail;
    struct buf *lbuf;
    struct buf *dbuf;

    for (tail = 0; tail < fs_log.lh.n; tail++) {
        lbuf = bread(fs_log.dev, fs_log.start+tail+1); // read log block
        dbuf = bread(fs_log.dev, fs_log.lh.sector[tail]); // read dst

        memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst

        bwrite(dbuf);  // write dst to disk
        brelse(lbuf);
        brelse(dbuf);
    }
}

// Read the log header from disk into the in-memory log header
static void read_head(void)
{
    struct buf *buf;
    struct logheader *lh;
    int i;

    buf = bread(fs_log.dev, fs_log.start);
    lh = (struct logheader *) (buf->data);
    fs_log.lh.n = lh->n;

    for (i = 0; i < fs_log.lh.n; i++) {
        fs_log.lh.sector[i] = lh->sector[i];
    }

    brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void write_head(void)
{
    struct buf *buf;
    struct logheader *hb;
    int i;

    buf = bread(fs_log.dev, fs_log.start);
    hb = (struct logheader *) (buf->data);

    hb->n = fs_log.lh.n;

    for (i = 0; i < fs_log.lh.n; i++) {
        hb->sector[i] = fs_log.lh.sector[i];
    }

    bwrite(buf);
    brelse(buf);
}

static void recover_from_log(void)
{
    read_head();
    install_trans(); // if committed, copy from log to disk
    fs_log.lh.n = 0;
    write_head(); // clear the log
}

void begin_trans(void)
{
    acquire(&fs_log.lock);

    while (fs_log.busy) {
    	xv6_sleep(&fs_log, &fs_log.lock);
    }

    fs_log.busy = 1;
    release(&fs_log.lock);
}

void commit_trans(void)
{
    if (fs_log.lh.n > 0) {
        write_head();    // Write header to disk -- the real commit
        install_trans(); // Now install writes to home locations
        fs_log.lh.n = 0;
        write_head();    // Erase the transaction from the log
    }

    acquire(&fs_log.lock);
    fs_log.busy = 0;
    xv6_wakeup(&fs_log);
    release(&fs_log.lock);
}

// Caller has modified b->data and is done with the buffer.
// Append the block to the log and record the block number,
// but don't write the log header (which would commit the write).
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void log_write(struct buf *b)
{
    struct buf *lbuf;
    int i;

    if (fs_log.lh.n >= LOGSIZE || fs_log.lh.n >= fs_log.size - 1) {
        panic("too big a transaction");
    }

    if (!fs_log.busy) {
        panic("write outside of trans");
    }

    for (i = 0; i < fs_log.lh.n; i++) {
        if (fs_log.lh.sector[i] == b->sector) { // log absorbtion?
            break;
        }
    }

    fs_log.lh.sector[i] = b->sector;
    lbuf = bread(b->dev, fs_log.start+i+1);

    memmove(lbuf->data, b->data, BSIZE);
    bwrite(lbuf);
    brelse(lbuf);

    if (i == fs_log.lh.n) {
        fs_log.lh.n++;
    }

    b->flags |= B_DIRTY; // XXX prevent eviction
}

//PAGEBREAK!
// Blank page.

