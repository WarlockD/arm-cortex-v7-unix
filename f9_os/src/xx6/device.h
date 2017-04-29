#ifndef _XV6_DEVICE_H_
#define _XV6_DEVICE_H_

#include "types.h"
// devices drivers, ripped from BSD2.11
struct buf;
struct tty;


typedef int	(*bdevsw_open)(dev_t,mode_t,va_list va);
typedef int	(*bdevsw_close)(dev_t);
typedef int	(*bdevsw_read)(dev_t, char*, off_t,size_t);
typedef int	(*bdevsw_write)(dev_t, char*, off_t,size_t);
typedef int	(*bdevsw_strategy)(struct buf*);
typedef size_t	(*bdevsw_psize)(dev_t);

struct bdevsw
{
	bdevsw_open d_open;
	bdevsw_close d_close;
	bdevsw_read d_read;
	bdevsw_write d_write;
	bdevsw_strategy d_strategy; /* XXX root attach routine */
	bdevsw_psize d_psize; 	// block device total size
	int	d_flags;
};
#if defined(__KERNEL__) && !defined(SUPERVISOR)
extern struct	bdevsw bdevsw[];
#endif

/*
 * Character device switch.
 */
typedef int	(*cdevsw_open)(dev_t, mode_t,va_list);
typedef int	(*cdevsw_close)(dev_t);
typedef int	(*cdevsw_read)(dev_t);
typedef int	(*cdevsw_write)(dev_t,int);
typedef int	(*cdevsw_ioctl)(dev_t,int,mode_t,va_list); // command
typedef int	(*cdevsw_stop)(struct tty*);

struct cdevsw
{
	cdevsw_open d_open;
	cdevsw_close d_close;
	cdevsw_read d_read;
	cdevsw_write d_write;
	cdevsw_ioctl d_ioctl;
	cdevsw_stop d_stop;
	struct tty *d_ttys;
};
#if defined(__KERNEL__) && !defined(SUPERVISOR)
extern struct	cdevsw cdevsw[];
#endif

/*
 * tty line control switch.
 */
struct linesw
{
	int (*l_open)(dev_t, struct tty*,va_list va);
	void (*l_close)(struct tty*);
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)(int,struct tty*,va_list va); // command
	int	(*l_rint)();
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)();
	int	(*l_modem)();
};
#if defined(__KERNEL__) && !defined(SUPERVISOR)
extern struct	linesw linesw[];
extern int nldisp;
#endif



#endif
