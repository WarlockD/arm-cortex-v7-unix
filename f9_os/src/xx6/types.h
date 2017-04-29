#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <sys\types.h>
#include <sys\time.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <stdarg.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __KERNEL__
#define major(dev) (dev_t)(((dev) >> 8) & 0xFF)
#define minor(dev) (dev_t)((dev) & 0xFF)
//#include "macros.h"
#define	SET(t,f)	(t) |= (f)
#define	CLR(t,f)	(t) &= ~(f)
#define	ISSET(t,f)	(((t) & (f))==(f))
#define MIN(A,B)	((A)<((B) ? (A) : (B)))
#else


#endif

#define NELM(X) (sizeof((X))/sizeof((X)[0]))
#define TELM(X)  __typeof__((X)[0])
#define PTELM(X)  __typeof__(&(X)[0])
#define for_each_array(NAME,ARRAY) \
	for( __typeof__(&ARRAY[0]) NAME = &ARRAY[0]; NAME != &ARRAY[NELM(ARRAY)]; NAME++)

#define __ALLIGNED(X) __attribute__((aligned(X)))
#define __ALLIGNED32 __ALLIGNED(sizeof(uint32_t))
#define BIT(x) (1<<(x))
#define __NAKED __attribute__((naked))

#define CAST(type, x) ((type)(x)) /* because there are just to many () in this thing */
#define CASTTO(FROM,TO) CAST(__typeof__(FROM), (TO))
#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

#define ALIGNP(x,a)   ((__typeof__(x))ALIGN(((uintptr_t)(x)),(a)))
// http://www.hudku.com/blog/essential-macros-for-c-programming/
// Aligns the supplied size to the specified PowerOfTwo
#define ALIGN_SIZE( sizeToAlign, PowerOfTwo ) (((sizeToAlign) + ((CASTTO(sizeToAlign, PowerOfTwo)) - 1)) & ~((CASTTO(sizeToAlign, PowerOfTwo)) - 1))

// Checks whether the supplied size is aligned to the specified PowerOfTwo
#define IS_SIZE_ALIGNED( sizeToTest, PowerOfTwo ) (((sizeToTest) & ((PowerOfTwo) - 1)) == 0)

#define IS_BETWEEN( numToTest, numLow, numHigh ) \
       (((numToTest) >= CASTTO(numToTest,numLow)) && ((numToTest) <= CASTTO(numToTest,numHigh)))
#endif
