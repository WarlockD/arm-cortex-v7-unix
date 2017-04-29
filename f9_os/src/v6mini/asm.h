
#ifndef _ASM_H_
#define _ASM_H_


/* Automatically saved registers */
#define REG_OFFSET_ARM_cpsr		17
#define REG_OFFSET_ARM_pc		16
#define REG_OFFSET_ARM_lr		15
#define REG_OFFSET_ARM_ip		14
#define REG_OFFSET_ARM_r3		13
#define REG_OFFSET_ARM_r2		12
#define REG_OFFSET_ARM_r1		11
#define REG_OFFSET_ARM_r0		10
/* saved by the exception entry code */
#define REG_OFFSET_ARM_EXC_lr	9
#define REG_OFFSET_ARM_sp		8
#define REG_OFFSET_ARM_fp		7
#define REG_OFFSET_ARM_r10		6
#define REG_OFFSET_ARM_r9		5
#define REG_OFFSET_ARM_r8		4
#define REG_OFFSET_ARM_r7		3
#define REG_OFFSET_ARM_r6		2
#define REG_OFFSET_ARM_r5		1
#define REG_OFFSET_ARM_r4		0
#define REG_OFFSET_ARM_ORIG_r0	18
#define REG_OFFSET_ALIGNMENT	1
#define REG_OFFSET_COUNT		(20+REG_OFFSET_ALIGNMENT) /* total regesters saved */
#define REG_OFFSET_BYTE_SIZE	4

/*
 * We use bit 30 of the preempt_count to indicate that kernel
 * preemption is occurring.  See <asm/hardirq.h>.
 */
#define PREEMPT_ACTIVE	0x40000000

/*
 * thread information flags:
 *  TIF_SYSCALL_TRACE	- syscall trace active
 *  TIF_SIGPENDING	- signal pending
 *  TIF_NEED_RESCHED	- rescheduling necessary
 *  TIF_NOTIFY_RESUME	- callback before returning to user
 *  TIF_USEDFPU		- FPU was used by this task this quantum (SMP)
 *  TIF_POLLING_NRFLAG	- true if poll_idle() is polling TIF_NEED_RESCHED
 */
#define TIF_SIGPENDING		0
#define TIF_NEED_RESCHED	1
#define TIF_NOTIFY_RESUME	2	/* callback before returning to user */
#define TIF_SYSCALL_TRACE	8
#define TIF_POLLING_NRFLAG	16
#define TIF_USING_IWMMXT	17
#define TIF_MEMDIE		18
#define TIF_FREEZE		19
#define TIF_RESTORE_SIGMASK	20

#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_POLLING_NRFLAG	(1 << TIF_POLLING_NRFLAG)
#define _TIF_USING_IWMMXT	(1 << TIF_USING_IWMMXT)
#define _TIF_FREEZE		(1 << TIF_FREEZE)
#define _TIF_RESTORE_SIGMASK	(1 << TIF_RESTORE_SIGMASK)

/*
 * Change these and you break ASM code in entry-common.S
 */
#define _TIF_WORK_MASK		0x000000ff

#ifdef __ASEMBLY__


#if __thumb__
# define FUNC(name) .type name, %function; .thumb_func; name:
# define SET .thumb_set
#else
# define FUNC(name) .type name, %function; name:
# define SET .set
#endif
#define GLOBAL(name) .global name; FUNC(name)
#define SIZE(name) .size name, .-name
#define END(PROC) SIZE(PROC)
#define	ENDPROC(PROC) .type PROC, function; END(PROC)

# define SYSCALL4(name) \
	GLOBAL(__ ## name); \
	swi #SYS_ ## name; \
	b _set_errno; \
	SIZE(__ ## name)
# define SYSCALL6(name) \
	GLOBAL(__ ## name); \
	push { r4 - r5 }; \
	ldr r4, [sp, #8]; \
	ldr r5, [sp, #12]; \
	swi #SYS_ ## name; \
	pop { r4 - r5 }; \
	b _set_errno; \
	SIZE(__ ## name)
#define SYSCALL0(name) SYSCALL3(name)
#define SYSCALL3(name) SYSCALL4(name)
#define SYSCALL1(name) SYSCALL3(name)
#define SYSCALL2(name) SYSCALL3(name)
#define SYSCALL5(name) SYSCALL6(name)

#define DEFINE(sym, val) sym= val
#define offsetof(X,Y)  (Y)

/* Automatically saved registers */
#define ARM_cpsr	17
#define ARM_pc		16
#define ARM_lr		15
#define ARM_ip		14
#define ARM_r3		13
#define ARM_r2		12
#define ARM_r1		11
#define ARM_r0		10
/* saved by the exception entry code */
#define ARM_EXC_lr	9
#define ARM_sp		8
#define ARM_fp		7
#define ARM_r10		6
#define ARM_r9		5
#define ARM_r8		4
#define ARM_r7		3
#define ARM_r6		2
#define ARM_r5		1
#define ARM_r4		0
#define ARM_ORIG_r0	18
#define S_FRAME_SIZE 20

DEFINE(S_R0,			offsetof(struct pt_regs, ARM_r0));
DEFINE(S_R1,			offsetof(struct pt_regs, ARM_r1));
DEFINE(S_R2,			offsetof(struct pt_regs, ARM_r2));
DEFINE(S_R3,			offsetof(struct pt_regs, ARM_r3));
DEFINE(S_R4,			offsetof(struct pt_regs, ARM_r4));
DEFINE(S_R5,			offsetof(struct pt_regs, ARM_r5));
DEFINE(S_R6,			offsetof(struct pt_regs, ARM_r6));
DEFINE(S_R7,			offsetof(struct pt_regs, ARM_r7));
DEFINE(S_R8,			offsetof(struct pt_regs, ARM_r8));
DEFINE(S_R9,			offsetof(struct pt_regs, ARM_r9));
DEFINE(S_R10,			offsetof(struct pt_regs, ARM_r10));
DEFINE(S_FP,			offsetof(struct pt_regs, ARM_fp));
DEFINE(S_IP,			offsetof(struct pt_regs, ARM_ip));
DEFINE(S_SP,			offsetof(struct pt_regs, ARM_sp));
DEFINE(S_LR,			offsetof(struct pt_regs, ARM_lr));
DEFINE(S_PC,			offsetof(struct pt_regs, ARM_pc));
DEFINE(S_PSR,			offsetof(struct pt_regs, ARM_cpsr));
DEFINE(S_OLD_R0,		offsetof(struct pt_regs, ARM_ORIG_r0));
DEFINE(S_EXC_LR,		offsetof(struct pt_regs, ARM_EXC_lr));
//DEFINE(S_FRAME_SIZE,	sizeof(struct pt_regs));

/*
 * These are the registers used in the syscall handler, and allow us to
 * have in theory up to 7 arguments to a function - r0 to r6.
 *
 * r7 is reserved for the system call number for thumb mode.
 *
 * Note that tbl == why is intentional.
 *
 * We must set at least "tsk" and "why" when calling ret_with_reschedule.
 */
scno	.req	r7		@ syscall number
tbl		.req	r8		@ syscall table pointer
why		.req	r8		@ Linux syscall (!= 0)
tsk		.req	r9		@ current thread_info

#else
#include <stdint.h>



#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a) = (v))
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile unsigned short __force *)(a) = (v))
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a) = (v))

#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a))
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile unsigned short __force *)(a))
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a))

static inline uint32_t get_psp() {
	uint32_t psp;
	__asm volatile("mrs %0, psp" : "=r"(psp) : :);
	return psp;
}
struct pt_regs {
	unsigned long uregs[REG_OFFSET_COUNT];
};


/* Automatically saved registers */
#define ARM_cpsr	uregs[REG_OFFSET_ARM_cpsr]
#define ARM_pc		uregs[REG_OFFSET_ARM_pc]
#define ARM_lr		uregs[REG_OFFSET_ARM_lr]
#define ARM_ip		uregs[REG_OFFSET_ARM_ip]
#define ARM_r3		uregs[REG_OFFSET_ARM_r3]
#define ARM_r2		uregs[REG_OFFSET_ARM_r2]
#define ARM_r1		uregs[REG_OFFSET_ARM_r1]
#define ARM_r0		uregs[REG_OFFSET_ARM_r0]
/* saved by the exception entry code */
#define ARM_EXC_lr	uregs[REG_OFFSET_ARM_EXC_lr]
#define ARM_sp		uregs[REG_OFFSET_ARM_sp]
#define ARM_fp		uregs[REG_OFFSET_ARM_fp]
#define ARM_r10		uregs[REG_OFFSET_ARM_r10]
#define ARM_r9		uregs[REG_OFFSET_ARM_r9]
#define ARM_r8		uregs[REG_OFFSET_ARM_r8]
#define ARM_r7		uregs[REG_OFFSET_ARM_r7]
#define ARM_r6		uregs[REG_OFFSET_ARM_r6]
#define ARM_r5		uregs[REG_OFFSET_ARM_r5]
#define ARM_r4		uregs[REG_OFFSET_ARM_r4]
#define ARM_ORIG_r0	uregs[REG_OFFSET_ARM_ORIG_r0]
#define S_FRAME_SIZE sizeof(struct pt_regs)

#endif

#endif
