#ifndef INCLUDE_BUF_H
#define INCLUDE_BUF_H

#include "types.h"
#include "param.h"
#include <sys\queue.h>

enum buf_flags {
	B_BUSY  =BIT(0),  // buffer is locked by some process
	B_VALID =BIT(1),  // buffer has been read from disk
	B_DIRTY =BIT(2),  // buffer needs to be written to disk
	B_DELAY =BIT(3),  // delay write till release
};


struct buf {
	enum buf_flags 	flags;
    dev_t      		dev;
    daddr_t    		sector;
    TAILQ_ENTRY(buf) hash;
   // struct buf *	qnext; 	// disk queue
    size_t			size;	// size of the buffer
    uint8_t*		data;	// buffer address
};


typedef int (*bdrvfunc)(dev_t, struct buf*);
// bio.c
void            binit(void);
void		    bdriver(dev_t dev, bdrvfunc read, bdrvfunc write);
struct buf*     bread(dev_t dev, daddr_t sector);
void            brelse(struct buf* bp);
void            bwrite(struct buf* bp);


#endif
