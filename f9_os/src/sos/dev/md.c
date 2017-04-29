#include <stm32f7xx.h>
#include <stm32746g_discovery.h>
#include "../v7_bio.h"



/* V7/x86 source code: see www.nordier.com/v7x86 for details. */
/* Copyright (c) 2007 Robert Nordier.  All rights reserved. */

/*
 * Memory disk driver
 */


static caddr_t mdmem = NULL;/* memory to use */
static size_t mdsz = 0;		/* size in blocks */

struct buf rmdbuf;

void mdopen(dev_t dev, void* base, size_t size) {
	mdmem = (caddr_t)mdmem;
	mdsz = size;
}
void mdclose(dev_t dev) {
	mdmem = (caddr_t)0;
	mdsz = 0;
}
void mdstrategy(struct buf *bp)
{
	size_t sz = (bp->b_bcount + BMASK) >> BSHIFT;
	if (!mdmem  || (bp->b_blkno + sz) > mdsz) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	//bp->av_forw = NULL;
	daddr_t x = bp->b_blkno << BSHIFT;
	if (bp->b_flags & B_READ)
		memcpy(bp->b_un.b_addr, mdmem + x, bp->b_bcount);
	else
		memcpy(mdmem + x, bp->b_un.b_addr,  bp->b_bcount);
	bp->b_resid = 0;
	iodone(bp);
}

void mdread(dev_t dev)
{
	physio(mdstrategy, &rmdbuf, dev, B_READ);
}

void mdwrite(dev_t dev)
{
	physio(mdstrategy, &rmdbuf, dev, B_WRITE);
}
struct bdevsw md_bdevsw = CREATE_BDEVSW(mdopen,mdclose,mdstrategy);
