#define __KERNEL__
#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "buf.h"
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>
#include <os\printk.h>

typedef int	(*bdevsw_open)(dev_t,int,...);
typedef int	(*bdevsw_close)(dev_t);
typedef int	(*bdevsw_read)(dev_t, char*, off_t,size_t);
typedef int	(*bdevsw_write)(dev_t, char*, off_t,size_t);
typedef int	(*bdevsw_strategy)(struct buf*);
typedef int	(*bdevsw_psize)(dev_t);

static int disksize;
static uint8_t *diskmem=NULL;

static void sdram_init() {
	BSP_SDRAM_Init();
	diskmem = (uint8_t*)SDRAM_DEVICE_ADDR;
	disksize = SDRAM_DEVICE_SIZE/BSIZE;
	memset(diskmem,0,1024*1024); // clear the first meg of ram
}
int sdram_open(dev_t dev,mode_t mode,va_list va) {
	sdram_init();
	return 0;
}
int sdram_close(dev_t d) {
	// we can't relaly close
	return 0;
}
int sdram_read(dev_t dev, uint8_t* data, off_t o, size_t n) {
	if(o <= 0 || o >= SDRAM_DEVICE_SIZE) return 0;
	if(!diskmem) {
		printk("uint sdram_read\r\n");
		sdram_init();
	}
	uint8_t* p = diskmem + o;
	n = MIN(SDRAM_DEVICE_SIZE-o, n);
    memmove(data, p, n);
    return n;
}
int sdram_write(dev_t dev, const uint8_t* data, off_t o, size_t n) {
	if(o <= 0 || o >= SDRAM_DEVICE_SIZE) return 0;
	if(!diskmem) {
		printk("uint sdram_write\r\n");
		sdram_init();
	}
	uint8_t* p = diskmem + o;
	n = MIN(SDRAM_DEVICE_SIZE-o, n);
    memmove(p, data, n);
    return n;
}
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void sdram_strategy(struct buf *b)
{
	if(!diskmem) {
		printk("uint sdram_strategy\r\n");
		sdram_init();
	}
    if(b->sector >= disksize)  panic("iderw: sector out of range");
	daddr_t o = b->sector * BSIZE;
	if(ISSET(b->flags, B_DIRTY)) {
		sdram_read(b->dev,b->data,o,BSIZE);
		CLR(b->flags, B_DIRTY);
	} else if(!ISSET(b->flags, B_VALID)){
		sdram_write(b->dev,b->data,o,BSIZE);
	}
    b->flags |= B_VALID;
}
size_t sdram_psize(struct buf *b){
	return SDRAM_DEVICE_SIZE;
}
