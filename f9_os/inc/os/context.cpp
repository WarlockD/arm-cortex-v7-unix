#include "context.hpp"

#include <stm32f7xx.h>



    namespace InterruptHandler
    {
      void
      __attribute__ ((naked))
      PendService(void)
      {
        asm volatile
        (
            // Registers R0-R3 are free to use. LR = R14.
            "       mrs r0, psp                         \n"// Get the main stack in R0
            "                                           \n"
            "       ldr     r3, ppStack                 \n"// Get the address of ms_ppStack
            "       ldr     r2, [r3]                    \n"
            "                                           \n"
#if 0
            "       tst r14, #0x10                      \n" // Is the task using the FPU context?
            "       it eq                               \n"
            "       vstmdbeq r0!, {s16-s31}             \n"// If so, push high vfp registers.
#endif
            "                                           \n"
            "       stmdb r0!, {r4-r11}                 \n" // Save the core registers.
            "                                           \n"
            "       str r0, [r2]                        \n"// Save the new top of stack into the first member of the TCB.
            "                                           \n"
            "       stmdb sp!, {r3, lr}                 \n"
            "                                           \n"
            "       mov r0, %[pri]                      \n"// Disable interrupts allowed to make system calls
            "       msr basepri, r0                     \n"
            "                                           \n"
            "       bl %[csw]                           \n"// bl contextSwitch
            "                                           \n"
            "       mov r0, #0                          \n"// Setting a value of 0 will cancel masking completely,
            "       msr basepri, r0                     \n"// enabling all interrupts
            "                                           \n"
            "       ldmia sp!, {r3, lr}                 \n"
            "                                           \n"
            "       ldr r1, [r3]                        \n"// At *ms_ppStack is the new stack pointer.
            "       ldr r0, [r1]                        \n"
            "                                           \n"
            "       ldmia r0!, {r4-r11}                 \n"// Pop the core registers.
            "                                           \n"
#if 0
            "       tst r14, #0x10                      \n" // Is the task using the FPU context?
            "       it eq                               \n"
            "       vldmiaeq r0!, {s16-s31}             \n"// If so, pop the high vfp registers too.
#endif
            "                                           \n"
            "       msr psp, r0                         \n" // Restore the main stack register
            "                                           \n"
            "       bx lr                               \n"// Branch to LR (R14) to return from exception
            "                                           \n"
            "       .align 2                            \n"
            "ppStack: .word _ZN3hal7cortexm13ThreadContext10ms_ppStackE       \n"

            : /* out */
            : /* in */
            [pri] "i"(configMAX_SYSCALL_INTERRUPT_PRIORITY),
            [csw] "i"(&hal::cortexm::ThreadContext::prepareSwitch)
            //[pps] "m"(&hal::cortexm::ArchitectureImplementation::ms_ppStack)
            : "memory", "cc"
        );
      }

    } // namespace InterruptHandler



namespace os {
ThreadContext::ThreadContext(void) : _stack(nullptr) {}
ThreadContext::~ThreadContext(void){}

ThreadContext ThreadContext::switch_to() const{

	// switches to this context, returning the previous context
}
void ThreadContext::create(uint32_t* pStackBottom,
    size_t stackSizeBytes,
    uint32_t trampolineEntryPoint, void* p1, void* p2,void* p3){
	 if (pStackBottom != nullptr && stackSizeBytes != 0)
	        {

	          uint32_t* pStack;
	          pStack = pStackBottom
	              + stackSizeBytes / sizeof(uint32_t);

	          // Place a few bytes of known values on the bottom of the stack.
	          // This has no functional purpose, it is useful only for debugging.

	          // The value on the right is the offset from the thread stack pointer

	          // This magic should always be present here. If it is not,
	          // someone else damaged the thread stack.
	          *--pStack = 0x12345678;           // magic

	          // To be safe, we need to align the stack frames to 8. In total
	          // we have 16 words to store, so if the current address is not even,
	          // descend an extra word.
	          if (((int) pStack & 7) != 0)
	            {
	              *--pStack = 0x12345678;       // one more magic
	            }

	          // Simulate the Cortex-M exception stack frame, i.e. how the stack
	          // would look after a call to yield().

	          // Thread starts with interrupts enabled.
	          // T bit set
	          *--pStack = 0x01000000;           // xPSR        +15*4=64

	          // The address of the trampoline code will be popped off the stack last,
	          // so place it first.

	          *--pStack = (hal::arch::stackElement_t) trampolineEntryPoint; // PCL     +14*4=60

	          // Create the stack as if after a context save.

	          *--pStack = 0x0;          // LR        +13*4=56
	          *--pStack = 12;           // R12       +12*4=52

	          // According to ARM ABI, first 4 word parameters are passed in R0-R3.
	          // We use only 3.
	          *--pStack = 3;                              // R3   +11*4=48
	          *--pStack = (uint32_t) p3; // R2   +10*4=44
	          *--pStack = (uint32_t) p2; // R1   +9*4=40
	          *--pStack = (uint32_t) p1; // R0   +8*4=36

	          *--pStack = 11;           // R11       +7*4=32
	          *--pStack = 10;           // R10       +6*4=28
	          *--pStack = 9;            // R9        +5*4=24
	          *--pStack = 8;            // R8        +4*4=20
	          *--pStack = 7;            // R7        +3*4=16
	          *--pStack = 6;            // R6        +2*4=12
	          *--pStack = 5;            // R5        +1*4=8
	          *--pStack = 4;            // R4        +0*4=4

	          // Be sure the stack is at least large enough to hold the exception frame.
	          assert(pStack > pStackBottom);

	          // Store the current stack pointer in the context
	          _stack = pStack;
	        }
}
/// \brief Save the current context in the local storage.
///
/// \par Parameters
///    None.
/// \retval true  Context saved, returns for the first time
/// \retval false Context restored, returns for the second time
///    Nothing.
bool ThreadContext::save(){

}
void ThreadContext::restore(){

}




};
