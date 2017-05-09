#include "irq.hpp"
#include <stm32f7xx.h>
#if 0

// notes for svc handler
void __attribute__ (( naked )) sv_call_handler(void)
{
    asm volatile(
      "movs r0, #4\t\n"
      "mov  r1, lr\t\n"
      "tst  r0, r1\t\n" /* Check EXC_RETURN[2] */
      "beq 1f\t\n"
      "mrs r0, psp\t\n"
      "ldr r1,=sv_call_handler_c\t\n"
      "bx r1\t\n"
      "1:mrs r0,msp\t\n"
      "ldr r1,=sv_call_handler_c\t\n"
      : /* no output */
      : /* no input */
      : "r0" /* clobber */
  );
}
sv_call_handler_c(unsigned int * hardfault_args)
{
    unsigned int stacked_r0;
    unsigned int stacked_r1;
    unsigned int stacked_r2;
    unsigned int stacked_r3;
    unsigned int stacked_r12;
    unsigned int stacked_lr;
    unsigned int stacked_pc;
    unsigned int stacked_psr;
    unsigned int svc_parameter;

    //Exception stack frame
    stacked_r0 = ((unsigned long) hardfault_args[0]);
    stacked_r1 = ((unsigned long) hardfault_args[1]);
    stacked_r2 = ((unsigned long) hardfault_args[2]);
    stacked_r3 = ((unsigned long) hardfault_args[3]);

    stacked_r12 = ((unsigned long) hardfault_args[4]);
    stacked_lr  = ((unsigned long) hardfault_args[5]);
    stacked_pc  = ((unsigned long) hardfault_args[6]);
    stacked_psr = ((unsigned long) hardfault_args[7]);

    svc_parameter = ((char *)stacked_pc)[-2]; /* would be LSB of PC is 1. */

    switch(svc_parameter){
    // each procesure call for the parameter
    }
}
#endif

static uint32_t* do_no_switch(uint32_t* stack) { return stack; } // so pendsv can be called and nothing happens
irq::switch_callback irq_context_switch_hook=do_no_switch;
/*
 * PendSV is used to perform a context switch. This is a recommended method for Cortex-M.
 * This is because the Cortex-M automatically saves half of the processor context
 * on any exception, and restores same on return from exception.  So only saving of R4-R11
 * and fixing up the stack pointers is required.  Using the PendSV exception this way means
 * that context saving and restoring is identical whether it is initiated from a thread
 * or occurs due to an interrupt or exception.
 *
 * Since PendSV interrupt has the lowest priority in the system (set by os_start() below),
 * we can be sure that it will run only when no other exception or interrupt is active, and
 * therefore safe to assume that context being switched out was using the process stack (PSP).
 */
extern "C" __attribute__((naked)) void PendSV_Handler()
{
#if (defined __ARM_ARCH_6M__)   // Cortex-M0(+)/Cortex-M1
    asm volatile (
        "    CPSID   I                 \n"  // Prevent interruption during context switch
        "    MRS     R0, PSP           \n"  // Load process stack pointer to R0
        "    SUB     R0, R0, #32       \n"  // Adjust R0 to point to top of saved context in stack
        "    MOV     R1, R0            \n"  // Preserve R0 (needed for os_context_switch_hook() call)
        "    STMIA   R1!, {R4-R7}      \n"  // Save low portion of remaining registers (r4-7) on process stack
        "    MOV     R4, R8            \n"  // Move high portion of remaining registers (r8-11) to low registers
        "    MOV     R5, R9            \n"
        "    MOV     R6, R10           \n"
        "    MOV     R7, R11           \n"
        "    STMIA   R1!, {R4-R7}      \n"  // Save high portion of remaining registers (r8-11) on process stack

        // At this point, entire context of process has been saved
        "    PUSH    {LR}              \n" // we must save LR (exc_return value) until exception return
        "    LDR     R1, =irq_context_switch_hook  \n"   // call os_context_switch_hook();
        "    BLX     R1                \n"

        // R0 is new process SP;
        "    ADD     R0, R0, #16       \n" // Adjust R0 to point to high registers (r8-11)
        "    LDMIA   R0!, {R4-R7}      \n" // Restore r8-11 from new process stack
        "    MOV     R8, R4            \n" // Move restored values to high registers (r8-11)
        "    MOV     R9, R5            \n"
        "    MOV     R10, R6           \n"
        "    MOV     R11, R7           \n"
        "    MSR     PSP, R0           \n" // R0 at this point is new process SP
        "    SUB     R0, R0, #32       \n" // Adjust R0 to point to low registers
        "    LDMIA   R0!, {R4-R7}      \n" // Restore r4-7
        "    CPSIE   I                 \n"
        "    POP     {PC}              \n" // Return to saved exc_return. Exception return will restore remaining context
        : :
    );
#else  // #if (defined __ARM_ARCH_6M__)

#if (defined __SOFTFP__)
    // M3/M4 cores without FPU
    asm volatile (
        "    CPSID   I                 \n" // Prevent interruption during context switch
        "    MRS     R0, PSP           \n" // PSP is process stack pointer
        "    STMDB   R0!, {R4-R11}     \n" // Save remaining regs r4-11 on process stack

        // At this point, entire context of process has been saved
        "    PUSH    {LR}              \n" // we must save LR (exc_return value) until exception return
        "    LDR     R1, =irq_context_switch_hook  \n"   // call os_context_switch_hook();
        "    BLX     R1                \n"

        // R0 is new process SP;
        "    LDMIA   R0!, {R4-R11}     \n" // Restore r4-11 from new process stack
        "    MSR     PSP, R0           \n" // Load PSP with new process SP
        "    CPSIE   I                 \n"
        "    POP     {PC}              \n" // Return to saved exc_return. Exception return will restore remaining context
        : :
    );

#else // #if (defined __SOFTFP__)
    // Core with FPU (cortex-M4F)
    asm volatile (
        "    CPSID     I                 \n" // Prevent interruption during context switch
        "    MRS       R0, PSP           \n" // PSP is process stack pointer
        "    TST       LR, #0x10         \n" // exc_return[4]=0? (it means that current process
        "    IT        EQ                \n" // has active floating point context)
        "    VSTMDBEQ  R0!, {S16-S31}    \n" // if so - save it.
        "    STMDB     R0!, {R4-R11, LR} \n" // save remaining regs r4-11 and LR on process stack

        // At this point, entire context of process has been saved
        "    LDR     R1, =irq_context_switch_hook  \n"   // call os_context_switch_hook();
        "    BLX     R1                \n"

        // R0 is new process SP;
        "    LDMIA     R0!, {R4-R11, LR} \n" // Restore r4-11 and LR from new process stack
        "    TST       LR, #0x10         \n" // exc_return[4]=0? (it means that new process
        "    IT        EQ                \n" // has active floating point context)
        "    VLDMIAEQ  R0!, {S16-S31}    \n" // if so - restore it.
        "    MSR       PSP, R0           \n" // Load PSP with new process SP
        "    CPSIE     I                 \n"
        "    BX        LR                \n" // Return to saved exc_return. Exception return will restore remaining context
        : :
    );
#endif  // #if (defined __SOFTFP__)
#endif  // #if (defined __ARM_ARCH_6M__)
}
// weak alias to support old name of PendSV_Handler.
#pragma weak PendSVC_ISR = PendSV_Handler



#define SCMRTOS_USE_CUSTOM_TIMER 0

#define SYSTICKFREQ     200000000UL
#define SYSTICKINTRATE  1000
#if 0
//------------------------------------------------------------------------------
// Define number of priority bits implemented in hardware.
//
#define CORE_PRIORITY_BITS  4

namespace {
	enum {
	#if (defined __SOFTFP__)
		CONTEXT_SIZE = 16 * sizeof(os::tack_item_t)   // Cortex-M0/M0+/M3/M4 context size
	#else
		CONTEXT_SIZE = 17 * sizeof(os::stack_item_t)   // Cortex-M4F initial context size (without FPU context)
	#endif
	};

	template<uintptr_t addr, typename type = uint32_t>
	struct ioregister_t
	{
		type operator=(type value) { *(volatile type*)addr = value; return value; }
		void operator|=(type value) { *(volatile type*)addr |= value; }
		void operator&=(type value) { *(volatile type*)addr &= value; }
		operator type() { return *(volatile type*)addr; }
	};

	template<uintptr_t addr, class T>
	struct iostruct_t
	{
		volatile T* operator->() { return (volatile T*)addr; }
	};

	// PendSV and SysTick priority registers

	#if (defined __ARM_ARCH_6M__)
	// Cortex-M0 system control registers are only accessible using word transfers.
	static ioregister_t<0xE000ED20UL, uint32_t> SHPR3;
	#define SHP3_WORD_ACCESS
	#else
	// Cortex-M3/M4 system control registers allows byte transfers.
	static ioregister_t<0xE000ED22UL, uint8_t> PendSvPriority;
	#if (SCMRTOS_USE_CUSTOM_TIMER == 0)
	static ioregister_t<0xE000ED23UL, uint8_t> SysTickPriority;
	#endif
	#endif

	#if (SCMRTOS_USE_CUSTOM_TIMER == 0)
	// SysTick stuff
	struct systick_t
	{
		uint32_t       CTRL;
		uint32_t       LOAD;
		uint32_t       VAL;
		uint32_t const CALIB;
	};

	enum
	{
		NVIC_ST_CTRL_CLK_SRC = 0x00000004,       // Clock Source.
		NVIC_ST_CTRL_INTEN   = 0x00000002,       // Interrupt enable.
		NVIC_ST_CTRL_ENABLE  = 0x00000001        // Counter mode.
	};

	static iostruct_t<0xE000E010UL, systick_t> SysTickRegisters;
	#endif

	#if (!defined __SOFTFP__)
	// Floating point context control register stuff
	static ioregister_t<0xE000EF34UL> FPCCR;
	enum
	{
		ASPEN =   (0x1UL << 31),  // always save enable
		LSPEN =   (0x1UL << 30)   // lazy save enable
	};
	#endif
	/*
	 * Default system timer initialization.
	 *
	 * Sets SysTick timer interrupt priority a bit higher than lowest one.
	 * Configures SysTick timer to interrupt with frequency SYSTICKINTRATE.
	 */
	#if (!defined CORE_PRIORITY_BITS)
	#    define CORE_PRIORITY_BITS        8
	#endif

	enum { SYS_TIMER_PRIORITY = ((0xFEUL << (8-(CORE_PRIORITY_BITS))) & 0xFF) };

};
#endif
static irq::timer_callback systick_callback = nullptr;
static size_t _ticks=0;

extern "C" void SysTick_Handler()
{
	_ticks++;
	systick_callback();
}

extern "C" __attribute__((naked))
static void exec_call(void* arg, void(*exec)(void*)){
    asm volatile (
    		"push { "
    		"blx r1\n"		// call the exec function
    		"pop { r0- r3, r12, lr, pc, psr }\n" // restore basic context
    		//"bx r4\n"		// return using the sent lr
    :::);
    __builtin_unreachable(); // suppress compiler warning "'noreturn' func does return"
}
 extern "C"
 static void no_exit() {
	printk("process existed without exit function\r\n!");
	while(1);
}
namespace irq {
	void raise_context_switch() { *((volatile uint32_t*)0xE000ED04) |= 0x10000000; }
	void init_system_timer(timer_callback callback, size_t hz){
		NVIC_SetPriority(SysTick_IRQn, 1);// one higher than 0
		NVIC_SetPriority(PendSV_IRQn, 0xFF);// highest it can ever go
		  SysTick->LOAD  = (uint32_t)(ticks - 1UL);                         /* set reload register */
		  NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */
		                                             /* Load the SysTick Counter Value */
		  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
		                   SysTick_CTRL_TICKINT_Msk   |
		                   SysTick_CTRL_ENABLE_Msk;

		SysTick->LOAD = SYSTICKFREQ/hz-1;
		SysTick->VAL   = 0UL;
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	}

	void lock_system_timer()   { SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; }
	void unlock_system_timer() { SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk; }
	void init_context_switch(switch_callback func){
		NVIC_SetPriority(SysTick_IRQn, 1);// one higher than 0
		NVIC_SetPriority(PendSV_IRQn, 0xFF);// highest it can ever go
		irq_context_switch_hook = func;
	}
	uint32_t* init_stack_frame( uint32_t * stack, void (*exec)(), void (*exit_func)()){
	    /*
	     * ARM Architecture Procedure Call Standard [AAPCS] requires 8-byte stack alignment.
	     * This means that we must get top of stack aligned _after_ context "pushing", at
	     * interrupt entry.
	     */
	    uintptr_t sptr = (((uintptr_t)stack - CONTEXT_SIZE) & 0xFFFFFFF8UL) + CONTEXT_SIZE;
	    uint32_t* stackp = (uint32_t*)sptr;
	    if(exit_func == nullptr)  exit_func=no_exit; // in case we are an idiot

	    *(--stackp)  = 0x01000000UL;      // xPSR
	    *(--stackp)  = reinterpret_cast<uint32_t>(exec); // Entry Point, pc
	    *(--stackp)  = reinterpret_cast<uint32_t>(exit_func); // exit Point, lr
	#if (defined __SOFTFP__)    // core without FPU
	    stackp -= 13;
	    //stackp -= 14;                     // emulate "push LR,R12,R3,R2,R1,R0,R11-R4"
	#else                       // core with FPU
	    stackp -= 6;                      // emulate "push LR,R12,R3,R2,R1,R0"
	    *(--stackp)  = 0xFFFFFFFDUL;      // exc_return: Return to Thread mode, floating-point context inactive, execution uses PSP after return.
	    stackp -= 8;                      // emulate "push R4-R11"
	#endif
	    return stackp;
	}
	struct sw_regs {
		uint32_t R4;
		uint32_t R5;
		uint32_t R6;
		uint32_t R7;
		uint32_t R8;
		uint32_t R9;
		uint32_t R10;
		uint32_t R11;
	};
	struct hw_regs {
		uint32_t R0;
		uint32_t R1;
		uint32_t R2;
		uint32_t R3;
		uint32_t R12;
		uint32_t LR;
		uint32_t PC;
		uint32_t PSR;
	}__attribute__((alinged(8)));
	struct regs {
		sw_regs sw;
		hw_regs hw;
	};



	uint32_t* push_exec(uint32_t* stack, void(*exec)(void*), void* arg) {
		// now you could use stack frame, but that emulates an entire
		stack -= (sizeof(regs)/sizeof(uint32_t)); // expand the stack to hold state data
		memcpy(stack, stack+8,sizeof(regs)); // move the context to make room for the function
		volatile regs* ctx =  reinterpret_cast<volatile regs*>(stack);
		volatile hw_regs* exe_ctx =  reinterpret_cast<volatile regs*>(ctx+1);
		exe_ctx = ctx->hw; // copy the regs
		exe_ctx->R0 = reinterpret_cast<uint32_t>(arg);
		exe_ctx->R1 = reinterpret_cast<uint32_t>(exec);

		exe_ctx->LR = ctx->hw.LR;
		exe_ctx->PC = ctx->hw.PC;


		stack[CTX_SIZE+REG::LR]
		uint32_t old_lr = stack[REG::LR];
		uint32_t old_pc = stack[REG:PC];
	}
};
