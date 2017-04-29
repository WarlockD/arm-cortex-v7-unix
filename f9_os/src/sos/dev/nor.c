#include <stm32f7xx.h>
#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include "../v7_bio.h"

// discovery nor driver

struct buf rnorbuf;
static int nor_open = 0;
static QSPI_Info nor_info;
void noropen(dev_t dev) {
	if(!nor_open){
		BSP_QSPI_Init();
		nor_open = 1;
		BSP_QSPI_GetInfo(&nor_info);
	}
}
void norclose(dev_t dev) {
	if(nor_open){
		BSP_QSPI_DeInit();
		nor_open = 0;
	}
}
void norstrategy(struct buf *bp)
{
	if(!nor_open) noropen(0);
	size_t sz = (bp->b_bcount + BMASK) >> BSHIFT;
	if ((bp->b_blkno + sz) > nor_info.FlashSize) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	//bp->av_forw = NULL;
	uint32_t x = bp->b_blkno << BSHIFT;
	if (bp->b_flags & B_READ)
		BSP_QSPI_Read((uint8_t*)bp->b_un.b_addr, x, bp->b_bcount);
	else
		BSP_QSPI_Write((uint8_t*)bp->b_un.b_addr, x, bp->b_bcount);
	bp->b_resid = 0;
	iodone(bp);
}

void norread(dev_t dev)
{
	physio(norstrategy, &rnorbuf, dev, B_READ);
}

void norwrite(dev_t dev)
{
	physio(norstrategy, &rnorbuf, dev, B_WRITE);
}
struct bdevsw nor_bdevsw = CREATE_BDEVSW(noropen,norclose,norstrategy);
