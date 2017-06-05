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
