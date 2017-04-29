#include "device.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "proc.h"

#include "mpu.h"
#include "memide.h"

#include <errno.h>
#undef errno
extern int errno;
#define CASTASSIGN(FIELD,VALUE) .d_##FIELD = (bdevsw_##FIELD)(VALUE)
static int nodev() { errno = ENODEV; return -1; }
static int nulldev() { return -1; }
#define BDEVSW_DEVICE(OPEN,CLOSE,READ,WRITE,STRAT,PSIZE) \
		{   CASTASSIGN(open,OPEN) , CASTASSIGN(close,CLOSE)  , CASTASSIGN(read,READ) , CASTASSIGN(write,WRITE) , \
			CASTASSIGN(strategy,STRAT), CASTASSIGN(psize, PSIZE) }

#define BDEVSW_NODEV BDEVSW_DEVICE(nodev,nodev,nodev,nodev,nodev,nodev)
#define BDEVSW_DEVICE_NAME(NAME) BDEVSW_DEVICE(NAME##_open,NAME##_close, NAME##_read, NAME##_write, NAME##_strategy, NAME##_psize)
#define BDEVSW_DEVICES_BEGIN struct	bdevsw	bdevsw[] = {
#define BDEVSW_DEVICES_END BDEVSW_DEVICE(NULL,NULL,NULL,NULL,NULL,NULL) };





BDEVSW_DEVICES_BEGIN
	BDEVSW_DEVICE_NAME(sdram),		// root device
	BDEVSW_NODEV,
	BDEVSW_NODEV,
	BDEVSW_NODEV,
BDEVSW_DEVICES_END



