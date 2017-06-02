/*
 * cortex_m.h
 *
 *  Created on: Jun 2, 2017
 *      Author: Paul
 */

#ifndef MIMIX_CPP_CORTEX_M_H_
#define MIMIX_CPP_CORTEX_M_H_

#include <cstdint>
#include <cstddef>
#include <stm32f7xx.h>
#include <os\casting.hpp>

namespace chip {
	__attribute__((always_inline)) 	static  inline void request_schedule(){
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
	static inline uint32_t systick_now() { return SysTick->VAL; }
	static inline void systick_disable() { SysTick->CTRL = 0x00000000; }
	static inline uint32_t systick_flag_count() { return (SysTick->CTRL & (1 << 16)) >> 16; }
	static inline uint32_t freq_to_ticks(uint32_t coreclock, uint32_t hz) { return coreclock / hz; }

	static inline void init_systick(uint32_t tick_reload, uint32_t tick_next_reload)
	{ 		/* 250us at 168Mhz */
		SysTick->LOAD = tick_reload - 1;
		SysTick->VAL = 0;
		SysTick->CTRL = 0x00000007;

		if (tick_next_reload)
			SysTick->LOAD  = tick_next_reload - 1;
	}
	// from nuttx
	/************************************************************************************
	 * Pre-processor Definitions
	 ************************************************************************************/

	/* The processor saves an EXC_RETURN value to the LR on exception entry. The
	 * exception mechanism relies on this value to detect when the processor has
	 * completed an exception handler.
	 *
	 * Bits [31:28] of an EXC_RETURN value are always 1.  When the processor loads a
	 * value matching this pattern to the PC it detects that the operation is a not
	 * a normal branch operation and instead, that the exception is complete.
	 * Therefore, it starts the exception return sequence.
	 *
	 * Bits[4:0] of the EXC_RETURN value indicate the required return stack and eventual
	 * processor mode.  The remaining bits of the EXC_RETURN value should be set to 1.
	 */

	/* EXC_RETURN_BASE: Bits that are always set in an EXC_RETURN value. */
	static inline uint32_t EXC_RETURN_BASE = 0xffffffe1; // these bits are aalways set

	/* EXC_RETURN_PROCESS_STACK: The exception saved (and will restore) the hardware
	 * context using the process stack pointer (if not set, the context was saved
	 * using the main stack pointer)
	 */
	static inline uint32_t EXC_RETURN_PROCESS_STACK =(1 << 2);

/* EXC_RETURN_THREAD_MODE: The exception will return to thread mode (if not set,
 * return stays in handler mode)
 */

	static inline uint32_t EXC_RETURN_THREAD_MODE   =(1 << 3);

/* EXC_RETURN_STD_CONTEXT: The state saved on the stack does not include the
 * volatile FP registers and FPSCR.  If this bit is clear, the state does include
 * these registers.
 */

	static inline uint32_t EXC_RETURN_STD_CONTEXT   =(1 << 4);

/* EXC_RETURN_HANDLER: Return to handler mode. Exception return gets state from
 * the main stack. Execution uses MSP after return.
 */

	static inline uint32_t EXC_RETURN_HANDLER       =0xfffffff1;

/* EXC_RETURN_PRIVTHR: Return to privileged thread mode. Exception return gets
 * state from the main stack. Execution uses MSP after return.
 */
	static inline bool return_has_fp_context(uint32_t exec) { return (exec & EXC_RETURN_STD_CONTEXT) == 0; }
	static inline uint32_t HW_REGS_COUNT = 8;
	static inline uint32_t HW_REGS_SIZE = HW_REGS_COUNT * sizeof(uint32_t);
	static inline uint32_t HW_FP_COUNT = 16;// 15 regs and fpcr
	static inline uint32_t HW_FP_SIZE = HW_FP_COUNT * sizeof(uint32_t);
	static inline uintptr_t after_hw_context(uintptr_t ctx, uint32_t lr_ret) { return return_has_fp_context(lr_ret) ? ctx + (16 * sizeof(uint32_t)) : ctx; }
	template<typename T>
	static inline T after_hw_context(T ctx, uint32_t lr_ret) { return after_hw_context(ptr_to_int(ctx),lr_ret); }

}
#if 0
}
	/* Memory mapping of Cortex-M4 Hardware */
	static constexpr uint32_t SCS_BASE           = (0xE000E000UL);                            /*!< System Control Space Base Address */
	static constexpr uint32_t ITM_BASE           = (0xE0000000UL);                            /*!< ITM Base Address */
	static constexpr uint32_t DWT_BASE           = (0xE0001000UL);                            /*!< DWT Base Address */
	static constexpr uint32_t TPI_BASE           = (0xE0040000UL);                            /*!< TPI Base Address */
	static constexpr uint32_t CoreDebug_BASE     = (0xE000EDF0UL);                            /*!< Core Debug Base Address */
	static constexpr uint32_t SysTick_BASE       = (SCS_BASE +  0x0010UL);                    /*!< SysTick Base Address */
	static constexpr uint32_t NVIC_BASE          = (SCS_BASE +  0x0100UL);                    /*!< NVIC Base Address */
	static constexpr uint32_t SCB_BASE           = (SCS_BASE +  0x0D00UL);                    /*!< System Control Block Base Address */

#define SCnSCB              ((SCnSCB_Type    *)     SCS_BASE      )   /*!< System control Register not in SCB */
#define SCB                 ((SCB_Type       *)     SCB_BASE      )   /*!< SCB configuration struct */
#define SysTick             ((SysTick_Type   *)     SysTick_BASE  )   /*!< SysTick configuration struct */
#define NVIC                ((NVIC_Type      *)     NVIC_BASE     )   /*!< NVIC configuration struct */
#define ITM                 ((ITM_Type       *)     ITM_BASE      )   /*!< ITM configuration struct */
#define DWT                 ((DWT_Type       *)     DWT_BASE      )   /*!< DWT configuration struct */
#define TPI                 ((TPI_Type       *)     TPI_BASE      )   /*!< TPI configuration struct */
#define CoreDebug           ((CoreDebug_Type *)     CoreDebug_BASE)   /*!< Core Debug configuration struct */
}


#endif

#endif /* MIMIX_CPP_CORTEX_M_H_ */
