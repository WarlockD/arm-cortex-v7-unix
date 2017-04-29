
// The ARM UART, a memory mapped device
#include "arm.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include <stm32f7xx_hal.h>
#include <stm32f7xx.h>

#include <time.h>
#include <sys\time.h>
#include <sys\times.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static uint32_t tim2_prescaler;
#define HZ_TICK  (uint32_t)( (1000000U/HZ))

time_t					ticks  = 0;
static time_t			sec  = 0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
//void TIM2_IRQHandler() { HAL_TIM_IRQHandler(&TimHandle); }
//void TIM3_IRQHandler() { HAL_TIM_IRQHandler(&TimHandle); }

#define APB1CLKDIV (uint32_t)(RCC->CFGR & RCC_CFGR_PPRE1)
#define APB2CLKDIV ((uint32_t)((RCC->CFGR & RCC_CFGR_PPRE2) >> 3)


void TIM2_IRQHandler() {
	 if((TIM2->SR & TIM_FLAG_UPDATE) != 0 && (TIM2->DIER & TIM_FLAG_UPDATE)!=0){
		 TIM2->SR &= ~TIM_FLAG_UPDATE;
		 ++sec;
	 }
	 if((TIM2->SR & TIM_FLAG_CC1) != 0 && (TIM2->DIER & TIM_IT_CC1)!=0){
		 TIM2->SR &= ~TIM_FLAG_CC1;
		 TIM2->CCR1 +=HZ_TICK;
		 ++ticks;
	 }
	//HAL_TIM_IRQHandler(&TimHandle);
}
static uint32_t CaculatePrescale(uint32_t freq) {
	 RCC_ClkInitTypeDef    clkconfig;
	 uint32_t              pFLatency;
	 HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
	 uint32_t uwTimclock = HAL_RCC_GetPCLK1Freq();
	 if(clkconfig.APB1CLKDivider != RCC_HCLK_DIV1)  uwTimclock *= 2;
	 return   (uint32_t) ((uwTimclock / freq) - 1U); /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
}

void timer_init(int freq) {
	tim2_prescaler = CaculatePrescale(1000000U);
	  HAL_NVIC_SetPriority(TIM2_IRQn, 0U ,0U);
	  HAL_NVIC_EnableIRQ(TIM2_IRQn);
	//prc = CaculatePrescale(freq);
	 __HAL_RCC_TIM2_CLK_ENABLE();

	TIM2->PSC = tim2_prescaler;
	TIM2->ARR  = UINT32_MAX ;// resets on overflow only
	TIM2->CCR1 = HZ_TICK; // 60 times a second
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE;
	TIM3->CCER |= TIM_CCER_CC1E;
	TIM2->DIER |= TIM_IT_UPDATE | TIM_IT_CC1;
	TIM2->CR1 |= TIM_CR1_CEN;

	// the timer (ticker)
}
