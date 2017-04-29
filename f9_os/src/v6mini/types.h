#ifndef _MXSYS_TYPES_H_
#define _MXSYS_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <sys\types.h>
#include <sys\time.h>
#include <sys\queue.h>
#include <sys\stat.h>
#include <sys\signal.h>
#include <unistd.h>
#include <signal.h>
#include <string.h> // for memset
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#define FPIPE		04 /* funny enough no constant is in newlib for this */
/*
 * Flags that work for fcntl(fd, F_SETFL, FXXXX)
 */
#define	FAPPEND		_FAPPEND
#define	FSYNC		_FSYNC
#define	FASYNC		_FASYNC
#define	FNBIO		_FNBIO
#define	FNONBIO		_FNONBLOCK	/* XXX fix to be NONBLOCK everywhere */
#define	FNDELAY		_FNDELAY

/*
 * Flags that are disallowed for fcntl's (FCNTLCANT);
 * used for opens, internal state, or locking.
 */
#define	FREAD		_FREAD
#define	FWRITE		_FWRITE
#define	FMARK		_FMARK
#define	FDEFER		_FDEFER
#define	FSHLOCK		_FSHLOCK
#define	FEXLOCK		_FEXLOCK

/*
 * The rest of the flags, used only for opens
 */
#define	FOPEN		_FOPEN
#define	FCREAT		_FCREAT
#define	FTRUNC		_FTRUNC
#define	FEXCL		_FEXCL
#define	FNOCTTY		_FNOCTTY

#define NELM(X) (sizeof((X))/sizeof((X)[0]))
#define TELM(X)  __typeof__((X)[0])
#define PTELM(X)  __typeof__(&(X)[0])
#define for_each_array(NAME,ARRAY) \
	for( __typeof__(&ARRAY[0]) NAME = &ARRAY[0]; NAME != &ARRAY[NELM(ARRAY)]; NAME++)
extern int irq_enabled();
// mach defines
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
#define COUNTOF(X)  (sizeof(X)/sizeof(X[0]))

extern caddr_t TOPSYS;
extern caddr_t TOPUSR;
#define irq_enabled() ({ int mask; __asm volatile ("mrs %0, primask" : "=r"(mask) : ); mask;})
#define irq_enable()  do {__asm volatile ("cpsie i" :  : "memory");} while(0)
#define irq_disable() do { __asm volatile ("cpsid i" :  : "memory");} while(0)
//	CPSID i  ; Disable interrupts and configurable fault handlers (set PRIMASK)
//   CPSID f  ; Disable interrupts and all fault handlers (set FAULTMASK)
 //  CPSIE i  ; Enable interrupts and configurable fault handlers (clear PRIMASK)
 //  CPSIE f  ; Enable interrupts and fault handlers (clear FAULTMASK).

#define idle()  do {__asm volatile ("wfi":::"memory");} while(0)

#define  fubyte(PTR) (*((caddr_t)(PTR))) /* figure out latter */
#define  subyte(PTR,VAL)  ((*((caddr_t)(PTR)))=(VAL))
#define  fuword(PTR)  	  (*((int*)(PTR)))
#define  suword(PTR,VAL)  ((*((int*)(PTR)))=(VAL))

#define retu(X) /* load regesters at location */
#define savu(X) /* save regesters to location */

#define copyout(DEST,SRC,SIZE)  /* filler */
#define copyin(DEST,SRC,SIZE) /* filler */
struct context {
	uint32_t basepri;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t ex_lr;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
};

#endif
