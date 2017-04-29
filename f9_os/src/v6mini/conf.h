#ifndef _MXSYS_CONF_H_
#define _MXSYS_CONF_H_

#include "types.h"

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct buf;
struct tty;
struct	bdevsw
{
	int	(*d_open)(dev_t dev, int flags);
	int	(*d_close)(dev_t dev, int flags);
	int	(*d_strategy)(struct buf*);
	struct buf	*d_tab;
} ;
extern struct bdevsw bdevsw[];
/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in binit/bio.c by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */
extern int	nblkdev;

/*
 * Character device switch.
 */
struct	cdevsw
{
	int	(*d_open)(dev_t dev, int flags);
	int	(*d_close)(dev_t dev, int flags);
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_sgtty)();
	int	(*d_ioctl)(int,int,mode_t,va_list); // dev, cmd, file flag, args
	int	(*d_stop)(struct tty *tp);
};

extern struct cdevsw cdevsw[];
/*
 * Number of character switch entries.
 * Set by cinit/tty.c
 */
extern int	nchrdev;
extern int swapdev; // swap dev
extern int rootdev;
void nodev(); 	// subr.c
void nulldev(); // subr.c
#endif
