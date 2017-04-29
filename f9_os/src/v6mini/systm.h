#ifndef _MXSYS_SYSTM_H_
#define _MXSYS_SYSTM_H_

#include "types.h"
#include "param.h"

/*
 * Random set of variables
 * used by more than one
 * routine.
 */
struct inode;

extern struct inode* rootdir;		/* pointer to inode of root directory */
extern int	lbolt;			/* time of day in 60th not in time */
//int	time[2];		/* time in sec from 1970 */
extern time_t tout;		/* time of day of next sleep */
/*
 * The callout structure is for
 * a routine arranging
 * to be called by the clock interrupt
 * (clock.c) with a specified argument,
 * in a specified amount of time.
 * Used, for example, to time tab
 * delays on teletypes.
 */
struct	callo
{
	time_t	c_time;		/* incremental time */
	void*	c_arg;		/* argument to routine */
	void	(*c_func)(void*);	/* routine */
};
extern struct callo callout[NCALL];
/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */

extern int	mpid;			/* generic for unique process id's */
extern int	runrun;			/* scheduling flag */
extern int	curpri;			/* more scheduling */
extern int	rootdev;		/* dev of root see conf.c */
extern int	swapdev;		/* dev of swap see conf.c */
extern int	updlock;		/* lock for sync */
extern char	regloc[];		/* locs. of saved user registers (trap.c) */

#endif
