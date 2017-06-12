/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012-2016 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/
/*!

    \file   threadport.cpp   

    \brief  ARM Cortex-M4 Multithreading; FPU Enabled.

*/

#include "kerneltypes.h"
#include "mark3cfg.h"
#include "thread.h"
#include "threadport.h"
#include "kernelswi.h"
#include "kerneltimer.h"
#include "timerlist.h"
#include "quantum.h"
#include "f9_context.h"

#include <stm32f7xx.h>
#include <cassert>

//---------------------------------------------------------------------------
#if KERNEL_USE_IDLE_FUNC
# error "KERNEL_USE_IDLE_FUNC not supported in this port"
#endif

//---------------------------------------------------------------------------
static void ThreadPort_StartFirstThread( void ) __attribute__ (( naked ));
extern "C" {
    //void SVC_Handler( void ) __attribute__ (( naked ));
    void PendSV_Handler( void ) __attribute__ (( naked ));
    void SysTick_Handler( void );
}

critical_section::critical_section() :__sr(__get_PRIMASK()) { __disable_irq();}
critical_section::~critical_section() 	{ __set_PRIMASK(__sr);  }
void critical_section::lock() 			{ __disable_irq(); }
void critical_section::release() 		{ __enable_irq(); }




//volatile Thread* g_pclNext=nullptr;
//Thread* g_pclCurrent=nullptr;
//---------------------------------------------------------------------------
/*
    1) Setting up the thread stacks

    Stack consists of 2 separate frames mashed together.
    a) Exception Stack Frame

    Contains the 8 registers the CPU pushes/pops to/from the stack on execution
    of an exception:

    [ XPSR ]
    [ PC   ]
    [ LR   ]
    [ R12  ]
    [ R3   ]
    [ R2   ]
    [ R1   ]
    [ R0   ]

    XPSR - Needs to be set to 0x01000000; the "T" bit (thumb) must be set for
           any thread executing on an ARMv6-m processor
    PC - Should be set with the initial entry point for the thread
    LR - The base link register.  We can leave this as 0, and set to 0xD on
         first context switch to tell the CPU to resume execution using the
         stack pointer held in the PSP as the regular stack.

    This is done by the CPU automagically- this format is part of the
    architecture, and there's nothing we can do to change or modify it.

    b) "Other" Register Context

    [ R11   ]
    ...
    [ R4    ]

    These are the other GP registers that need to be backed up/restored on a
    context switch, but aren't by default on a CM0 exception.  If there were
    any additional hardware registers to back up, then we'd also have to
    include them in this part of the context.

    These all need to be manually pushed to the stack on stack creation, and
    puhsed/pop as part of a normal context switch.
*/
template< size_t AMOUNT,typename T>
constexpr static inline bool is_alligned(T*ptr) { return (reinterpret_cast<intptr_t>(ptr)& static_cast<intptr_t>(-AMOUNT)) == reinterpret_cast<intptr_t>(ptr); }
template< size_t AMOUNT,typename T>
constexpr static inline size_t alligment_offset(T*ptr) { return (reinterpret_cast<intptr_t>(ptr)& ~(AMOUNT-1)); }

template< size_t AMOUNT,typename T>
constexpr static inline T* allign_up(T*ptr) { return reinterpret_cast<T*>((reinterpret_cast<intptr_t>(ptr) + (AMOUNT-1)) & (-AMOUNT)); }
template< size_t AMOUNT,typename T>
constexpr static inline T* allign_down(T*ptr) { return reinterpret_cast<T*>((reinterpret_cast<intptr_t>(ptr) - reinterpret_cast<intptr_t>(ptr) % AMOUNT)); }


void ThreadPort::InitStack(Thread *pclThread_)
{
#if 0
	// we need to make sure the stack starts AND end on an 8 byte boundry
	if(!is_alligned<8>(pclThread_->m_pwStack)){
		pclThread_->m_u16StackSize-=alligment_offset<8>(pclThread_->m_pwStack);
		pclThread_->m_pwStack = allign_up<8>(pclThread_->m_pwStack);
	}
#endif
	std::fill_n(pclThread_->m_pwStack,pclThread_->m_u16StackSize / sizeof(uint32_t),0xDEADBEEF);
    uint32_t* pu32Stack=allign_down<8>(pclThread_->m_pwStack + pclThread_->m_u16StackSize/sizeof(uint32_t)-1);
#ifndef __SOFTFP__
    // if float is enabled, add 16 to the stack
    pu32Stack += 16;
#endif
   // PUSH_TO_STACK(pu32Stack,0);
    //-- Simulated Exception Stack Frame --

    uint32_t entry_point =(uint32_t)(pclThread_->m_pfEntryPoint);
    /*
     *    ok, using QEMU it will stick out an error
     *    M profile return from interrupt with misaligned PC is UNPREDICTABLE
     *    Reason for this is the PC must have an LSB of 0 because thats how bx detects
     *    if we are returning to a thumb instruction or not.  So we set it here
     *    so it will return correctly
     *
     */
    entry_point &= (~1);    /* PC with LSB=0 */

    PUSH_TO_STACK(pu32Stack, 0x01000000);    // XSPR
    PUSH_TO_STACK(pu32Stack, entry_point);        // PC
    PUSH_TO_STACK(pu32Stack, 0);             // LR
    PUSH_TO_STACK(pu32Stack, 0x12);
    PUSH_TO_STACK(pu32Stack, 0x3);
    PUSH_TO_STACK(pu32Stack, 0x2);
    PUSH_TO_STACK(pu32Stack, 0x1);
    PUSH_TO_STACK(pu32Stack, (uint32_t)pclThread_->m_pvArg);    // R0 = argument
    pu32Stack++;	// ARG this screwed me over on the push the stack
    //assert(is_alligned<8>(pu32Stack)); // make sure we are alligned
    pclThread_->m_context.sp = reinterpret_cast<uint32_t>(pu32Stack);
    // below is kernel
    pclThread_->m_context.ret = 0xFFFFFFF9;
    pclThread_->m_context.ctl = 0x0;
    // below is user
  // pclThread_->m_context.ret = 0xFFFFFFFD;
   // pclThread_->m_context.ctl = 0x3;
}

//---------------------------------------------------------------------------
void Thread_Switch(void)
{
	g_pclCurrent = const_cast<Thread*>(g_pclNext);
}
extern "C" void printk(const char*,...);
//---------------------------------------------------------------------------
void ThreadPort::StartThreads()
{
    KernelSWI::Config();             // configure the task switch SWI
    KernelTimer::Config();           // configure the kernel timer

    Scheduler::SetScheduler(1);      // enable the scheduler
    Scheduler::Schedule();           // run the scheduler - determine the first thread to run

    Thread_Switch();                 // Set the next scheduled thread to the current thread

    KernelTimer::Start();            // enable the kernel timer
    KernelSWI::Start();              // enable the task switch SWI

#if KERNEL_USE_QUANTUM
    Quantum::RemoveThread();
    Quantum::AddThread(g_pclCurrent);
#endif

#ifndef __SOFTFP__
    SCB->CPACR |= 0x00F00000;        // Enable floating-point

    FPU->FPCCR |= (FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk); // Enable lazy-stacking
    printk("Hard Float Enabled\n");
#else
    printk("Soft Float Enabled\n");
#endif



    ThreadPort_StartFirstThread();     // Jump to the first thread (does not return)
}

//---------------------------------------------------------------------------
/*
    The same general process applies to starting the kernel as per usual

    We can either:
        1) Simulate a return from an exception manually to start the first
           thread, or..
        2) use a software exception to trigger the first "Context Restore
            /Return from Interrupt" that we have otherwised used to this point.

    For 1), we basically have to restore the whole stack manually, not relying
    on the CPU to do any of this for u16.  That's certainly doable, but not all
    Cortex parts support this (due to other members of the family supporting
    priveleged modes).  So, we will opt for the second option.

    So, to implement a software interrupt to restore our first thread, we will
    use the SVC instruction to generate that exception.

    At the end of thread initialization, we have to do 2 things:

    -Enable exceptions/interrupts
    -Call SVC

    Optionally, we can reset the MSP stack pointer to the top-of-stack.
    Note, the default stack pointer location is stored at address
    0x00000000 on all ARM Cortex M0 parts

    (While Mark3 avoids assembler code as much as possible, there are some
    places where it cannot be avoided.  However, we can at least inline it
    in most circumstances.)
*/
void ThreadPort_StartFirstThread( void )
{
	__enable_irq();
	volatile f9_context_t* ctx = &g_pclCurrent->m_context;
	KernelSWI::context_restore(const_cast<f9_context_t*>(ctx));
#if 0
    ASM (
      //  " mov r0, #0 \n"
    //    " ldr r1, [r0] \n"
    //    " msr msp, r1 \n"
        " cpsie i \n"
        " svc 0 \n"
    );
#endif
}




void PendSV_Handler(void)
{
	irq_enter();
	if(g_pclCurrent != g_pclNext){
		__asm__ __volatile__ ("pop {lr}");
		irq_save(&g_pclCurrent->m_context);
		Thread_Switch();
		irq_restore(&g_pclCurrent->m_context);
		__asm__ __volatile__ ("bx lr");
	}
	irq_return();
}

#if 0
//---------------------------------------------------------------------------
/*
    The SVC Call

    This handler has the job of taking the first thread object's stack, and
    restoring the default state data in a way that ensures that the thread
    starts executing when returning from the call.

    We also keep in mind that there's an 8-byte offset from the beginning of
    the thread object to the location of the thread stack pointer.  This 
    offset is a result of the thread object inheriting from the linked-list
    node class, which has 8-bytes of data.  This is stored first in the 
    object, before the first element of the class, which is the "stack top"
    pointer.

    get_thread_stack:
        ; Get the stack pointer for the current thread
        ldr r0, g_pclCurrent
        ldr r1, [r0]
        add r1, #8
        ldr r2, [r1]         ; r2 contains the current stack-top

    load_manually_placed_context_r11_r4:
        ; Handle the bottom 36-bytes of the stack frame
        ; On cortex m3 and up, we can do this in one ldmia instruction.
        ldmia r2!, {r4-r11, r14}

    set_psp:
        ; Since r2 is coincidentally back to where the stack pointer should be,
        ; Set the program stack pointer such that returning from the exception handler
        msr psp, r2

    ** Note - Since we don't care about these registers on init, we could take a shortcut if we wanted to **
    shortcut_init:
        add r2, #32
        msr psp, r2

    set_lr:
        ; Set up the link register such that on return, the code operates in thread mode using the PSP
        ; To do this, we or 0x0D to the value stored in the lr by the exception hardware EXC_RETURN.
        ; Alternately, we could just force lr to be 0xFFFFFFFD (we know that's what we want from the hardware, anyway)
        mov  r0, #0x0D
        mov  r1, lr
        orr r0, r1

    exit_exception:
        ; Return from the exception handler.  The CPU will automagically unstack R0-R3, R12, PC, LR, and xPSR
        ; for u16.  If all goes well, our thread will start execution at the entrypoint, with the user-specified
        ; argument.
        bx r0

        This code is identical to what we need to restore the context, so
        we'll just make it a macro and be done with it.
*/
// ugh LinkListNode adds 8 to the size of the class
// mabye build in the linklist into the thread? humm
void SVC_Handler(void)
{
    ASM(
    // Get the pointer to the first thread's stack
    " mov r3, %[CURRENT_THREAD]\n "
    " add r3, #8 \n "
    " ldr r2, [r3] \n "
    // Stack pointer is in r2, start loading registers from the "manually-stacked" set
    " ldmia r2!, {r4-r11, r14} \n "
    // After subtracting R2 by #32 due to stack popping, our PSP is where it
    // needs to be when we return from the exception handler
    " msr psp, r2 \n "    
    // Also modify the control register to force use of thread mode as well
    // For CM3 forward-compatibility, keep threads in privileged mode
    " mrs r0, control \n"
    " mov r1, #0x02 \n"
    " orr r0, r1 \n"
    " msr control, r0 \n"    
    " isb \n "
    // Return into thread mode, using PSP as the thread's stack pointer
    " bx lr \n "
    : : [CURRENT_THREAD] "r" (g_pclCurrent)
    );
}

//---------------------------------------------------------------------------
/*
    Context Switching:
    
    On ARM Cortex parts, there's dedicated hardware that's used primarily to 
    support RTOS (or RTOS-like) funcationlity.  This functionality includes
    the SysTick timer, and the PendSV Exception.  SysTick is used for the 
    kernel timer (I need to learn how to use it to see whether or not I can
    do tickless timers), while the PendSV exception is used for triggering
    context switches.  In reality, it's a "special SVC" call that's designed
    to be lower-overhead, in that it isn't mux'd with a bunch of other system
    or application functionality.
    
    Alright, so how do we go about actually implementing a context switch here?
    There are a lot of different parts involved, but it essentially comes down
    to 3 steps:
    
    1) Save Context
    2) Swap "current" and "next" thread pointers
    3) Restore Context
    
1) Saving the context.

    !!ToDo -- add documentation about how this works on cortex m4f, especially
    in the context of the floating-point registers, lazy stacking, etc.

2)  Swap threads

    This is the easy part - we just call a function to swap in the thread "current" thread    
    from the "next" thread.
    
3)    Restore Context

    This is more or less identical to what we did when restoring the first context. 
    
*/    

#define WORKS_WITH_ECLIPSE
void PendSV_Handler(void)
{    
    ASM(
    // Thread_SaveContext()
    " cpsid i \n "
    " ldr 	r1, CURR_ \n"
    " ldr 	r2, [r1, #8] \n "	// stack top
    " mrs 	r3, psp \n "
    " str 	r3, [r2] \n "		// first save the psp on stack top
    " ldr 	r2, [r1, #12] \n "	// get the start of the stack
    " stmia r2!, {r3-r11, r14} \n "	// save the extra regesters there

#ifndef __SOFTFP__
    " tst r14, #0x10\n "
    " it eq \n "
    " vstmdbeq r2!, {s16-s31} \n "
    // save more if need be
#endif
    // Equivalent of Thread_Swap() -- g_pclNext -> g_pclCurrent
    " ldr r1, CURR_ \n"
    " ldr r0, NEXT_ \n"
    " ldr r0, [r0] \n"
    " str r0, [r1] \n"

    // now we restore the context here
    " ldr r2, [r0, #12] \n "	// get the start of the stack
    " ldmia r2!, {r3-r11, r14} \n "	// restore the saved part of the stack
#ifndef __SOFTFP__
    " tst r14, #0x10\n "
    " it eq \n "
    " vldmiaeq r2!, {s16-s31} \n "
#endif
    // technicly all we have to do is msr psp, r3, but lets use that pointer
    " ldr r2, [r0, #8] \n "	// get the start of the stack
    " msr psp, r2 \n "
    " cpsie i \n "
    // lr contains the proper EXC_RETURN value, we're done with exceptions.
    " bx lr \n "

    // Must be 4-byte aligned.  Also - GNU assembler, I hate you for making me resort to this.
    " NEXT_: .word g_pclNext \n"
    " CURR_: .word g_pclCurrent \n"
    );
}
#endif
//---------------------------------------------------------------------------


