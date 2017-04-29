#ifndef ARM_INCLUDE
#define ARM_INCLUDE



// got the idea for the enums here
// https://embdev.net/topic/153828
#define REGS_NAME(name)		ARM_##name
// trap frame: in ARM, there are seven modes. Among the 16 regular registers,
// r13 (sp), r14(lr), r15(pc) are banked in all modes.
// 1. In xv6_a, all kernel level activities (e.g., Syscall and IRQ) happens
// in the SVC mode. CPU is put in different modes by different events. We
// switch them to the SVC mode, by shoving the trapframe to the kernel stack.
// 2. during the context switched, the banked user space registers should also
// be saved/restored.
//
// Here is an example:
// 1. a user app issues a syscall (via SWI), its user-space registers are
// saved on its kernel stack, syscall is being served.
// 2. an interrupt happens, it preempted the syscall. the app's kernel-space
// registers are again saved on its stack.
// 3. interrupt service ended, and execution returns to the syscall.
// 4. kernel decides to reschedule (context switch), it saves the kernel states
// and switches to a new process (including user-space banked registers)
#ifndef __ASSEMBLER__


#define REGS_BEGIN 				typedef enum reg_offsets {
#define REGS_VAL(name) 			REGS_NAME(name),
#define REGS_END  				CTX_FRAME_SIZE, } reg_offsets_t;


#else
// asm header

.syntax unified
.cpu cortex-m7
.fpu softvfp
.thumb

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

.macro reg_val name
.equiv \name, last_enum_reg_value
.set last_enum_reg_value, last_enum_reg_value + 1
.endm

#define REGS_BEGIN  	.set last_enum_reg_value, 0
#define REGS_END		.equiv CTX_FRAME_SIZE, last_enum_reg_value
#define REGS_VAL(name)	reg_val REGS_NAME(name)



#endif
// sets up the constant offsets
#include "reg.h"


#ifndef __ASSEMBLER__
#include <stdint.h>
// gives us the best of both worlds
// a rege array we can use the constants in
// and a structure I can poke at in gdb
struct trapframe {

#define REGS_BEGIN  union { uintptr_t regs[CTX_FRAME_SIZE]; struct {
#define REGS_VAL(name)	uint32_t name;
#define REGS_END }; };

#include "reg.h"
};


#define splx(x) ({ \
	int __rbasepri; \
	__asm volatile( \
			"mrs %0, basepri\n" \
		    "msr basepri, %1\n" \
			: "=r"(__rbasepri) : "r"(x) : "memory" ); \
			__rbasepri; })
#define irq_save() ({ int __s; __asm volatile("mrs %0, basepri" :"=r"(__s) :: "memory"); __s})
#define irq_restore(x) __asm volatile("msr basepri, %0" : : r(x) : "memory")
#define spl0() splx(15)
#define spl1() splx(10)
#define spl2() splx(8)
#define spl3() splx(6)
#define spl4() splx(4)
#define spl5() splx(2)
#define spl6() splx(1)
#define spl7() splx(0)

/*
 * irq_save()
 *
 * Saves {r4-r11}, msp, psp
 */
#define __irq_save(ctx)		\
	__asm__ __volatile__ ("mov r0, %0": : "r" ((ctx)->regs) : "r0");			\
	__asm__ __volatile__ ("stm r0, {r4-r11}");			\
	__asm__ __volatile__ ("and r4, lr, 0xf":::"r4");		\
	__asm__ __volatile__ ("teq r4, #0x9");				\
	__asm__ __volatile__ ("ite eq");				\
	__asm__ __volatile__ ("mrseq r0, msp"::: "r0");			\
	__asm__ __volatile__ ("mrsne r0, psp"::: "r0");			\
	__asm__ __volatile__ ("mov %0, r0" : "=r" ((ctx)->sp));	\
	__asm__ __volatile__ ("mov %0, lr" : "=r" ((ctx)->ret)); \
	__asm__ __volatile__ ("mrs %0, basepri" : "=r" ((ctx)->basepri));

#ifdef CONFIG_FPU
#define irq_save(ctx)							\
	__asm__ __volatile__ ("cpsid i");				\
	(ctx)->fp_flag = 0;						\
	__irq_save(ctx);						\
	__asm__ __volatile__ ("tst lr, 0x10");				\
	__asm__ __volatile__ ("bne no_fp");				\
	__asm__ __volatile__ ("mov r3, %0"				\
			: : "r" ((ctx)->fp_regs) : "r3");		\
	__asm__ __volatile__ ("vstm r3!, {d8-d15}"			\
			::: "r3");					\
	__asm__ __volatile__ ("mov r4, 0x1": : :"r4");			\
	__asm__ __volatile__ ("stm r3, {r4}");				\
	__asm__ __volatile__ ("no_fp:");
#else	/* ! CONFIG_FPU */

#define ctx_save(ctx)							\
	__asm__ __volatile__ ("cpsid i");				\
	__irq_save(ctx);
#endif

#define __ctx_restore(ctx)						\
	__asm__ __volatile__ ("mov lr, %0" : : "r" ((ctx)->ret));	\
	__asm__ __volatile__ ("mov r0, %0" : : "r" ((ctx)->sp));	\
	__asm__ __volatile__ ("mov r2, %0" : : "r" ((ctx)->ctl));	\
	__asm__ __volatile__ ("and r4, lr, 0xf":::"r4");		\
	__asm__ __volatile__ ("teq r4, #0x9");				\
	__asm__ __volatile__ ("ite eq");				\
	__asm__ __volatile__ ("msreq msp, r0");				\
	__asm__ __volatile__ ("msrne psp, r0");				\
	__asm__ __volatile__ ("mov r0, %0" : : "r" ((ctx)->regs));	\
	__asm__ __volatile__ ("ldm r0, {r4-r11}");			\
	__asm__ __volatile__ ("msr control, r2");			\
	__asm__ __volatile__ ("msr basepri, %0" : "=r" ((ctx)->basepri));

#ifdef CONFIG_FPU
#define ctx_restore(ctx)						\
	__irq_restore(ctx);						\
	if ((ctx)->fp_flag) {						\
		__asm__ __volatile__ ("mov r0, %0" 			\
				: : "r" ((ctx)->fp_regs): "r0");	\
		__asm__ __volatile__ ("vldm r0, {d8-d15}");		\
	}								\
	__asm__ __volatile__ ("cpsie i");
#else	/* ! CONFIG_FPU */
#define ctx_restore(ctx) \
	__irq_restore(ctx);						\
	__asm__ __volatile__ ("cpsie i");
#endif

/* Initial context switches to kernel.
 * It simulates interrupt to save corect context on stack.
 * When interrupts are enabled, it will jump to interrupt handler
 * and then return to normal execution of kernel code.
 */
#define init_ctx_switch(ctx, pc) \
	__asm__ __volatile__ ("mov r0, %0" : : "r" ((ctx)->sp));	\
	__asm__ __volatile__ ("msr msp, r0");				\
	__asm__ __volatile__ ("mov r1, %0" : : "r" (pc));		\
	__asm__ __volatile__ ("cpsie i");				\
	__asm__ __volatile__ ("bx r1");

#define irq_enter()							\
	__asm__ __volatile__ ("push {lr}");

#define irq_return()							\
	__asm__ __volatile__ ("pop {lr}");				\
	__asm__ __volatile__ ("bx lr");

#define context_switch(from, to)					\
	{								\
		__asm__ __volatile__ ("pop {lr}");			\
		irq_save(&(from)->ctx);					\
		thread_switch((to));					\
		irq_restore(&(from)->ctx);				\
		__asm__ __volatile__ ("bx lr");				\
	}

#define schedule_in_irq()						\
	{								\
		register tcb_t *sel;					\
		sel = schedule_select();				\
		if (sel != current)					\
			context_switch(current, sel);			\
	}

#define request_schedule()						\
	do { SCB->ICSR |= SCB_ICSR_PENDSVSET_Mak; } while (0)


#define NO_PREEMPTED_IRQ						\
	(SCB->ICSR & SCB_ICSR_RETTOBASE)


#if 1
// special case cause eveything is fucked


typedef struct context {
	uint32_t sp;
	uint32_t basepri;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t fp;
	uint32_t ret; // ex lr
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t ip;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
}context_t;

#else
typedef struct context {
	uint32_t sp;
	uint32_t ret;
	uint32_t ctl;
	uint32_t basepri;
	union {
		struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t r3;
		uint32_t ip;
		uint32_t lr;
		uint32_t pc;
		uint32_t xpsr;
		};
		uint32_t regs[8];
	};
#ifdef CONFIG_FPU
	uint64_t fp_regs[8];
	uint32_t fp_flag;
#endif
} context_t;
#endif

#endif

#endif
