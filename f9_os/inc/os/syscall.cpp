/*
 * syscall.cpp
 *
 *  Created on: Jun 3, 2017
 *      Author: warlo
 */

#include "syscall.hpp"
#include <diag\Trace.h>


namespace os {
#if 0
static void dispatch_syscall(void) naked_function;
static void dispatch_syscall(void)
{
  __asm__ __volatile__
  (
    " sub sp, sp, #16\n"           /* Create a stack frame to hold 3 parms + lr */
    " str r4, [sp, #0]\n"          /* Move parameter 4 (if any) into position */
    " str r5, [sp, #4]\n"          /* Move parameter 5 (if any) into position */
    " str r6, [sp, #8]\n"          /* Move parameter 6 (if any) into position */
    " str lr, [sp, #12]\n"         /* Save lr in the stack frame */
    " ldr ip, =g_stublookup\n"     /* R12=The base of the stub lookup table */
    " ldr ip, [ip, r0, lsl #2]\n"  /* R12=The address of the stub for this syscall */
    " blx ip\n"                    /* Call the stub (modifies lr) */
    " ldr lr, [sp, #12]\n"         /* Restore lr */
    " add sp, sp, #16\n"           /* Destroy the stack frame */
    " mov r2, r0\n"                /* R2=Save return value in R2 */
    " mov r0, #3\n"                /* R0=SYS_syscall_return */
    " svc 0"                       /* Return from the syscall */
  );
}
#endif


}
#if 0
enum register_stack_t {
	/* Saved by hardware */
	REG_R0,
	REG_R1,
	REG_R2,
	REG_R3,
	REG_R12,
	REG_LR,
	REG_PC,
	REG_xPSR
};

#define RESERVED_STACK \
	(8 * sizeof(uint32_t))

static void dispatch_syscall() __attribute((naked));
static void dispatch_syscall(uint32_t* caller) __attribute((naked)){
	uint32_t svc_num = ((char *) caller[REG_PC])[-2];
}
	void syscall_init(uint8_t nbr, uintptr_t call){
		assert(nbr < MAX_SYSCALLS);
		caller = call;
	}
}
template<uintptr_t FROM, uintptr_t TO> static inline void copy_stack(){
	__asm volatile(
		"ldr r12, [sp, %0]\n"
		"str r12, [sp, %1]\n"
	: "i"(FROM), "i"(TO) ::"r12");
}
__attribute((always_inline) )static inline void copy_memory(uintptr from, uintptr_t to)
__attribute((always_inline) )static inline void copy_stack() {
	__asm__ __volatile__ ("push {r12 }sub sp, #(8*4)\n");
	copy_stack<REG_R0+8, REG_R0>();
}
#endif
//extern "C" void SVC_Handler() __attribute((naked)) ;
#if 0
extern "C" void SVC_Handler() {
	assert(0);

}
#endif




