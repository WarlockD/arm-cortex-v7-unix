#ifndef ARCH_CORTEX_M7_HPP
#define  ARCH_CORTEX_M7_HPP

#include <cstdint>
#include <cstddef>
#include <stm32f7xx.h>


namespace cortex_m7 {
	namespace config {
	/* Cortex-M specific definitions. */
	#ifdef __NVIC_PRIO_BITS
		/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
			static constexpr uint32_t PRIO_BITS   =    		__NVIC_PRIO_BITS;
	#else
			static constexpr uint32_t PRIO_BITS  =     		4;       /* 15 priority levels */
	#endif
		/* The lowest interrupt priority that can be used in a call to a "set priority"
		function. */
			static constexpr uint32_t LIBRARY_LOWEST_INTERRUPT_PRIORITY  = 0xf;
			/* The highest interrupt priority that can be used by any interrupt service
			routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
			INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
			PRIORITY THAN THIS! (higher priorities are lower numeric values. */
			static constexpr uint32_t LIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY =5;
			/* Interrupt priorities used by the kernel port layer itself.  These are generic
			to all Cortex-M ports, and do not rely on any particular library functions. */
			static constexpr uint32_t KERNEL_INTERRUPT_PRIORITY  = ( LIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - PRIO_BITS) );
			static constexpr uint32_t MAX_SYSCALL_INTERRUPT_PRIORITY  = ( LIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - PRIO_BITS) );


	}
	/* Scheduler utilities. */
	__attribute__((always_inline)) static inline void yield() {

		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		/* Barriers are normally not required but do ensure the code is completely
		within the specified behaviour for the architecture. */
		__DSB();
		__ISB();
	}
	__attribute__((always_inline)) static inline uint32_t isr_number() {
		return __get_IPSR();
	}
	__attribute__((always_inline)) static inline bool in_isr() { return __get_IPSR() != 0;}
	__attribute__((always_inline)) static inline void RaiseIrqPriority() {
		__disable_irq();
		__set_BASEPRI(config::MAX_SYSCALL_INTERRUPT_PRIORITY);
		__ISB();
		__DSB();
		__enable_irq();
	}
	__attribute__((always_inline)) static inline uint32_t RaiseIrqPriorityAndReturn() {
		uint32_t ret = __get_BASEPRI();
		__disable_irq();
		__set_BASEPRI(config::MAX_SYSCALL_INTERRUPT_PRIORITY);
		__ISB();
		__DSB();
		__enable_irq();
		return ret;
	}
	__attribute__((always_inline)) static inline void SetIrqPriority(uint32_t v) { __set_BASEPRI(v); }
	/* Critical section management. */
	void EnterCritical( void );
	void ExitCritical( void );

	__attribute__((always_inline)) static inline void SET_INTERRUPT_MASK_FROM_ISR()	{ RaiseIrqPriority(); }
	__attribute__((always_inline)) static inline void CLEAR_INTERRUPT_MASK_FROM_ISR(uint32_t x)	{SetIrqPriority(x); }

	__attribute__((always_inline)) static inline void DISABLE_INTERRUPTS()	{ RaiseIrqPriority(); }
	__attribute__((always_inline)) static inline void ENABLE_INTERRUPTS()	{ SetIrqPriority(0); }
	__attribute__((always_inline)) static inline void ENTER_CRITICAL()		{ EnterCritical();}
	__attribute__((always_inline)) static inline void EXIT_CRITICAL()		{ ExitCritical(); }
	// stack order, important
	 enum class reg {
			 sp,
			 basepri,
			 r4,
			 r5,
			 r6,
			 r7,
			 r8,
			 r9,
			 r10,
			 r11,
			 ex_lr,
			 r0,
			 r1,
			 r2,
			 r3,
			 r12,
			 pc,
			 lr,
		 };
		class context_t {
		protected:
			uint32_t* _sw_stack;
			uint32_t* _hw_stack;
			void init_stack(bool enable_fpu, bool handler_mode,uintptr_t pxTopOfStack, uintptr_t pc,  uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);
		public:
			context_t(): _sw_stack(nullptr), _hw_stack(nullptr) {}
			explicit context_t(uint32_t* sw_stack) : _sw_stack(sw_stack), _hw_stack(reinterpret_cast <uint32_t*>(sw_stack[0])) {}
			explicit context_t(uintptr_t sw_stack) : context_t(reinterpret_cast <uint32_t*>(sw_stack)) { }
			inline volatile uint32_t& at(reg i) {
			  return i>=reg::sp && i<=reg::ex_lr ?
					  _sw_stack[static_cast<uint32_t>(i)] :
					  _hw_stack[static_cast<uint32_t>(i)];
			}
			inline uint32_t at(reg i) const {
			  return i>=reg::sp && i<=reg::ex_lr ?
					  _sw_stack[static_cast<uint32_t>(i)] :
					  _hw_stack[static_cast<uint32_t>(i)];
			}
			volatile uint32_t& operator[](reg i) { return at(i); }
			uint32_t operator[](reg i) const { return at(i); }
		};
		static constexpr uintptr_t THUMB_ADDRES_MASK = 0xfffffffeUL;
		void EnableVFP( void ) __attribute__ (( naked ));
			void PendSV_Handler() __attribute__((naked)); // function for handling this type of context.  Needs to be installed as pendsv
}





#endif
