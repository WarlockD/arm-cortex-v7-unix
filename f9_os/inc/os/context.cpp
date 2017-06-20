#include "context.hpp"

#include <stm32f7xx.h>




namespace os {
	static ThreadContext* g_switch_to_thread=nullptr;
	static ThreadContext* g_switch_from_thread=nullptr;

	__attribute__((weak))  uintptr_t switch_thread(uintptr_t from){
		g_switch_from_thread->_set_manaual(from);
		return reinterpret_cast<uintptr_t>(g_switch_to_thread->_sw_stack);
	}

	static int make_id() {
		static int id = 1;
		int ret = id++;
		if(id < 0) id = 1;
		return ret;
	}
	ThreadContext::ThreadContext() : ThreadContext(make_id()) {}
	ThreadContext::ThreadContext(int id): _id(id) {}

	void ThreadContext::thread_switch(ThreadContext& from, ThreadContext& to){
		g_switch_to_thread = &to;
		g_switch_from_thread = &from;
		arch::ENTER_CRITICAL();
		arch::yield();
		arch::EXIT_CRITICAL();
	}
	/* For strict compliance with the Cortex-M spec the task start address should
	have bit-0 clear, as it is loaded into the PC on exit from an ISR. */

	static constexpr uint32_t THUMB_ADDRES_MASK = 0xfffffffeUL;
	void ThreadContext::_set_manaual(uintptr_t from){
		_sw_stack = reinterpret_cast<uint32_t*>(from);
		_hw_stack = reinterpret_cast<uint32_t*>(_sw_stack[0]);
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
	          init_stack(true,false,reinterpret_cast<uintptr_t>(pStack),trampolineEntryPoint,
	        		  reinterpret_cast<uintptr_t>(p1),
					  reinterpret_cast<uintptr_t>(p2),
					  reinterpret_cast<uintptr_t>(p3)
	          );
#if 0
	          // Simulate the Cortex-M exception stack frame, i.e. how the stack
	          // would look after a call to yield().

	          // Thread starts with interrupts enabled.
	          // T bit set
	          *--pStack = 0x01000000;           // xPSR        +15*4=64

	          // The address of the trampoline code will be popped off the stack last,
	          // so place it first.

	          *--pStack = (uint32_t)trampolineEntryPoint; // PCL     +14*4=60

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
#endif
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
