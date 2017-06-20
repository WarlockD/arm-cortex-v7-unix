#include "cortex_m7.hpp"
#include <cassert>

#ifndef __VFP_FP__
	#error This port can only be used when the project options are configured to enable hardware floating point support.
#endif

namespace os {
	uintptr_t switch_thread(uintptr_t from);
}
namespace {
	static constexpr uint32_t NVIC_PENDSV_PRI		= cortex_m7::config::KERNEL_INTERRUPT_PRIORITY  << 16UL ;
	static constexpr uint32_t NVIC_SYSTICK_PRI		= cortex_m7::config::KERNEL_INTERRUPT_PRIORITY  << 24UL ;
	/* EXC_RETURN_BASE: Bits that are always set in an EXC_RETURN value. */

	#define EXC_RETURN_BASE          0xffffffe1

	/* EXC_RETURN_PROCESS_STACK: The exception saved (and will restore) the hardware
	 * context using the process stack pointer (if not set, the context was saved
	 * using the main stack pointer)
	 */

	#define EXC_RETURN_PROCESS_STACK (1 << 2)

	/* EXC_RETURN_THREAD_MODE: The exception will return to thread mode (if not set,
	 * return stays in handler mode)
	 */

	#define EXC_RETURN_THREAD_MODE   (1 << 3)

	/* EXC_RETURN_STD_CONTEXT: The state saved on the stack does not include the
	 * volatile FP registers and FPSCR.  If this bit is clear, the state does include
	 * these registers.
	 */

	#define EXC_RETURN_STD_CONTEXT   (1 << 4)

	/* EXC_RETURN_HANDLER: Return to handler mode. Exception return gets state from
	 * the main stack. Execution uses MSP after return.
	 */

	#define EXC_RETURN_HANDLER       0xfffffff1
/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16 	( 0xE000E3F0 )
#define portAIRCR_REG						( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE					( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE					( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS				( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT					( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK					( 0xFFUL )

/* Constants required to manipulate the VFP. */
#define portFPCCR					( ( volatile uint32_t * ) 0xe000ef34 ) /* Floating point context control register. */
#define portASPEN_AND_LSPEN_BITS	( 0x3UL << 30UL )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR			( 0x01000000 )
#define portINITIAL_EXEC_RETURN		( 0xfffffffd )

/* The systick is a 24-bit counter. */
#define portMAX_24_BIT_NUMBER				( 0xffffffUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR			( 45UL )

/* For strict compliance with the Cortex-M spec the task start address should
have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK		( ( uint32_t ) 0xfffffffeUL )

/* Let the user override the pre-loading of the initial LR with the address of
prvTaskExitError() in case it messes up unwinding of the stack in the
debugger. */
#ifdef configTASK_RETURN_ADDRESS
	#define portTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define portTASK_RETURN_ADDRESS	TaskExitError
#endif

}
static void TaskExitError() {

}
namespace cortex_m7 {
	static uint32_t uxCriticalNesting = 0;


	void EnterCritical( void )
	{
		DISABLE_INTERRUPTS();
		uxCriticalNesting++;

		/* This is not the interrupt safe version of the enter critical function so
		assert() if it is being called from an interrupt context.  Only API
		functions that end in "FromISR" can be used in an interrupt.  Only assert if
		the critical nesting count is 1 to protect against recursive calls if the
		assert function also uses a critical section. */
		if( uxCriticalNesting == 1 )
		{
			assert( !in_isr());
		}
	}
	/*-----------------------------------------------------------*/

	void ExitCritical( void )
	{
		assert( uxCriticalNesting );
		uxCriticalNesting--;
		if( uxCriticalNesting == 0 ) ENABLE_INTERRUPTS();
	}
	void EnableVFP( void )  {
		__asm volatile
		(
			"	ldr.w r0, =0xE000ED88		\n" /* The FPU enable bits are in the CPACR. */
			"	ldr r1, [r0]				\n"
			"								\n"
			"	orr r1, r1, #( 0xf << 20 )	\n" /* Enable CP10 and CP11 coprocessors, then save back. */
			"	str r1, [r0]				\n"
			"	bx r14						"
		);
	}

	struct stack_helper {
		uint32_t* _stack;
		uint32_t* top() const { return _stack; }
		stack_helper(uintptr_t top) : _stack(reinterpret_cast <uint32_t*>(top)) {}
		void push() { --_stack; }
		void push(uintptr_t v) { *--_stack = (uintptr_t)v;  }
		template<typename T>
		void push(T* v) { push(reinterpret_cast <uintptr_t>(v)); }
		void skip(int count) { _stack-=count; }
		stack_helper& operator-=(uintptr_t v) { push(v); return *this; }
	};
#define TEST_INITAL_STATE
	void context_t::init_stack(bool enable_fpu, bool handler_mode,uintptr_t pxTopOfStack, uintptr_t pc,  uintptr_t arg0, uintptr_t arg1, uintptr_t arg2){
		/* Simulate the stack frame as it would be created by a context switch interrupt. */
		stack_helper stack(pxTopOfStack);
		stack -= 0x12345678;           // magic
	    if (((int)stack.top() & 7) != 0) stack -= 0x12345678;       // one more magic if not alligned
	    if(enable_fpu) stack.skip(16); // skip the lazy fpu context save

		uint32_t lr_ret = EXC_RETURN_BASE | EXC_RETURN_STD_CONTEXT | EXC_RETURN_THREAD_MODE;
		if(enable_fpu) lr_ret&= ~EXC_RETURN_STD_CONTEXT;
		if(handler_mode) lr_ret&= ~EXC_RETURN_THREAD_MODE;
		stack -= portINITIAL_XPSR;	/* xPSR */
		stack -= ( ( uintptr_t ) pc ) & portSTART_ADDRESS_MASK;	/* PC */
		stack -= ( uintptr_t ) portTASK_RETURN_ADDRESS;	/* LR */
		/* Save code space by skipping register initialisation. */
		stack -= 12; // r12
		stack -= arg2; // r2
		stack -= arg1; // r1
		stack -= arg0; // r0
		_hw_stack = stack.top();
		if(enable_fpu) stack.skip(15); // skip the fpu context save
		/* A save method is being used that requires each task to maintain its
		own exec return value. */
		stack -=  lr_ret; //portINITIAL_EXEC_RETURN;
		stack -= 11;
		stack -= 10;
		stack -= 9;
		stack -= 8;
		stack -= 7;
		stack -= 6;
		stack -= 5;
		stack -= 4;
		stack -= 15;		// base prioity _hw_stack
		stack -= reinterpret_cast<uintptr_t>(_hw_stack);
		_sw_stack = stack.top();
#ifdef TEST_INITAL_STATE
		// check to make sure the state is what we want it at
		assert(at(reg::r0) == arg0);
		assert(at(reg::r1) == arg1);
		assert(at(reg::r2) == arg2);
		assert(at(reg::pc) ==  (( ( uintptr_t ) pc ) & portSTART_ADDRESS_MASK));
		assert(at(reg::lr) == ( uintptr_t ) portTASK_RETURN_ADDRESS);
		assert(at(reg::ex_lr) == lr_ret);
		assert(at(reg::basepri) == 15);
		assert(at(reg::sp) == reinterpret_cast<uintptr_t>(_hw_stack));
#endif
	}
	void PendSV_Handler(){
			__asm volatile
			(
			"	tst r14, #4 						\n"
			"   ite eq								\n"
			"	mrseq r0, msp						\n"
			"	mrsne r0, psp						\n"
			"	mrs   r3, basepri					\n"
			"   mov   r2, r0						\n"
			"	isb									\n"
			"	tst r14, #0x10						\n" /* Is the task using the FPU context?  If so, push high vfp registers. */
			"	it eq								\n"
			"	vstmdbeq r0!, {s16-s31}				\n"
			"	stmdb r0!, {r2-r11, r14}			\n" /* Save the core registers. */
			"	mov  r4, r0							\n" // make a backup
			"										\n"
		//	"	stmdb sp!, {r3}						\n"
			"	mov r3, %0 							\n"
			"	cpsid i								\n" /* Errata workaround. */
			"	msr basepri, r3						\n"
			"	dsb									\n"
			"	isb									\n"
			"	cpsie i								\n" /* Errata workaround. */
			"	bl %1								\n" // calls the thread stack
			"	mov r3, #0							\n"
			"	msr basepri, r3						\n"
		//	"	ldmia sp!, {r3}						\n"
			"										\n"
			"										\n" // r0 has the new stack
			"	ldmia r0!, {r2-r11, r14}			\n" /* Pop the core registers. */
			"										\n"
			"	tst r14, #0x10						\n" /* Is the task using the FPU context?  If so, pop the high vfp registers too. */
			"	it eq								\n"
			"	vldmiaeq r0!, {s16-s31}				\n"
			"										\n"
			"	tst r14, #4 						\n"
			"   ite eq								\n"
			"	msreq msp, r2 						\n"
			"	msrne psp, r2						\n"
			"	msr   basepri, r3 					\n"
			"	isb									\n"
			"										\n"
			#ifdef WORKAROUND_PMU_CM001 /* XMC4000 specific errata workaround. */
				#if WORKAROUND_PMU_CM001 == 1
			"			push { r14 }				\n"
			"			pop { pc }					\n"
				#endif
			#endif
			"										\n"
			"	bx r14								\n"
			"										\n"
			"	.align 4							\n"
			"pxCurrentTCBConst: .word pxCurrentTCB	\n"
			::"i"(config::MAX_SYSCALL_INTERRUPT_PRIORITY), "i"(os::switch_thread)
			);

	}

}
