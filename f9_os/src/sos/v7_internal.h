#ifndef _V7_INTERNAL_H_
#define _V7_INTERNAL_H_

#include <errno.h>
#include "v7_types.h"


#undef errno
extern int errno; // for returning errno



#define __pointer(T)  typeof(T *)
#define __array(T, N) typeof(T [N])
// helper for going though a static array
// just saves some fingers
#define NELM(X)  (sizeof(X)/sizeof(X[0]))
#define foreach_array(NAME,ARRAY) \
	for(__typeof__(&ARRAY[0]) NAME = &ARRAY[0]; \
	NAME < &ARRAY[NELM(ARRAY)]; NAME++)


/*
 * Some macros for units conversion
 */
/* Core clicks (64 bytes) to segments and vice versa */
//#define	ctos(x)	((x+127)/128)
//#define stoc(x) ((x)*128)

/* Core clicks (64 bytes) to disk blocks */
//#define	ctod(x)	((x+7)>>3)
// bytes to diskblock
#define btod(x) (daddr_t)(((x)+(BSIZE-1))>>BSHIFT)
/* inumber to disk address */
#define	itod(x)	(daddr_t)((((unsigned)x+15)>>3))

/* inumber to disk offset */
#define	itoo(x)	(int)((x+15)&07)

/* clicks to bytes */
//#define	ctob(x)	(x<<6)

extern time_t current_time; // current time

#define max(a,b)                    \
    ({                              \
    __typeof__ (a) _a = (a);        \
    __typeof__ (b) _b = (b);         \
    _a > _b ? _a : _b; })

#define min(a,b)                    \
    ({                              \
    __typeof__ (a) _a = (a);        \
    __typeof__ (b) _b = (b);         \
    _a < _b ? _a : _b; })

#define	timerclear(tvp)		((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	    ((tvp)->tv_usec cmp (uvp)->tv_usec) :			\
	    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#define	timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)


#define SPL_PRIORITY_MAX 15
#define SPL_PRIORITY_MIN 1		/* 0 is hw clock and mabye systick? */

#define splx(X) ({ int ret; __asm volatile ( \
		"mrs %1, basepri\n" \
		"msr basepri, %0\n" \
		: "=r"(ret) : "r"(X) ); ret; })

#define spl0() splx(SPL_PRIORITY_MAX)
#define spl6() splx(SPL_PRIORITY_MIN)
#define splsoftclock() splx(SPL_PRIORITY_MIN)
#define splnet() splx(SPL_PRIORITY_MIN+1)
#define splbio() splx(SPL_PRIORITY_MIN+3)
#define splimp() splx(SPL_PRIORITY_MIN+3)
#define spltty() splx(SPL_PRIORITY_MIN+3)
#define splclock() splx(SPL_PRIORITY_MIN+4)
#define splhigh() splx(MAX_SPL_PRIORITY)
#define spl0() splx(SPL_PRIORITY_MAX)

#endif
