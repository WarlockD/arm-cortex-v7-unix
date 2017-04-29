#ifndef _MEMIDE_H_
#define _MEMIDE_H_

#include "types.h"
#include "device.h"

// sdram disk device

int sdram_open(dev_t dev,mode_t mode,va_list va);
int sdram_close(dev_t d);
int sdram_read(dev_t dev, char* data, off_t o, size_t n);
int sdram_write(dev_t dev, char* data, off_t o, size_t n);
void sdram_strategy(struct buf *b);
size_t sdram_psize(struct buf *b);

#endif
