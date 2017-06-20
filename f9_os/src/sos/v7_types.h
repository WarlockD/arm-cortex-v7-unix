#ifndef _V6_TYPES_H_
#define _V6_TYPES_H_




#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <sys\types.h>
#include <sys\time.h>
#include <sys\queue.h>
#include <sys\stat.h>
#include <signal.h>
#include <string.h> // for memset

#include "v7_param.h"

struct inode;
struct buf;
struct filsys;
struct user;
struct dir;
struct tty;

// buffer driver, aka block driver
typedef void (*bdevsw_fuc_dev)(dev_t, ...);
typedef void (*bdevsw_buf_dev)(struct buf *, ...);

struct bdevsw
{
	bdevsw_fuc_dev d_open;
	bdevsw_fuc_dev d_close;
	bdevsw_buf_dev d_strategy;
	TAILQ_HEAD(,buf) d_tab;	// extra buffer information?
};

#define CREATE_BDEVSW(OPEN,CLOSE,STRAT) \
{ .d_open = ((bdevsw_fuc_dev)(OPEN)), .d_close = ((bdevsw_fuc_dev)(CLOSE)), \
	.d_strategy = ((bdevsw_buf_dev)(STRAT)) }
/*
 * special redeclarations for
 * the head of the queue per
 * device driver.
 */
#define	b_actf	av_forw
#define	b_actl	av_back
#define	b_active b_bcount
#define	b_errcnt b_resid

/*
 * Character device switch.
 */
struct cdevsw
{
	bdevsw_fuc_dev d_open;
	bdevsw_fuc_dev d_close;
	bdevsw_fuc_dev d_read;
	bdevsw_fuc_dev d_write;
	bdevsw_fuc_dev d_ioctl;
	bdevsw_fuc_dev d_stop;
	struct tty *d_ttys;
};

typedef uint16_t baddr_t; // block address * BSIZE == daddr_t
typedef	uint32_t	label_t[23];	/* regs 2-7 */

typedef atomic_flag spinlock_t;
typedef atomic_uint ref_t ;
#define inc_rec(REF) atomic_fetch_add_explicit((REF), 1, memory_order_relaxed)
#define dec_rec(REF) atomic_fetch_sub_explicit((REF), 1, memory_order_relaxed)


// check some stuff
_Static_assert (sizeof(off_t) == 4, "Needs to be 32 bit?");
_Static_assert (sizeof(daddr_t) == 4, "Needs to be 32 bit?");
_Static_assert (sizeof(time_t) == 4, "Needs to be 32 bit?");

#if 1
static inline void channel_sleep(void* chan, int piro) { (void)chan; (void)piro; }
static inline  void channel_wakeup(void* chan){ (void)chan;  }
// renamed because of sleep defined
extern void ksleep(void* chan, int prio);
extern void kwakeup(void* chan);
#else
extern void channel_sleep(void* chan, int piro);
extern void channel_wakeup(void* chan);
#endif

#define aquire(PTR) while(atomic_flag_test_and_set(PTR))
#define release(PTR) atomic_flag_clear(PTR)


void trace_printf(const char*,...);
void panic(const char*,...);

#define NODEV ((dev_t)-1)
#define	major(x)  	(int)((((dev_t)(x))>>8))
#define	minor(x)  	(int)(((dev_t)(x))&0377)
#define	makedev(x,y)	(dev_t)((x)<<8|(y))


#endif
