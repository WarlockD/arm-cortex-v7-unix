/**
  ******************************************************************************
  * @file    stm32f7xx_hal_timebase_tim_template.c 
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    30-December-2016
  * @brief   HAL time base based on the hardware TIM Template.
  *    
  *          This file overrides the native HAL time base functions (defined as weak)
  *          the TIM time base:
  *           + Intializes the TIM peripheral generate a Period elapsed Event each 1ms
  *           + HAL_IncTick is called inside HAL_TIM_PeriodElapsedCallback ie each 1ms
  * 
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include <os\printk.h>
#include <assert.h>

#define MY_CLOCK
#ifdef  MY_CLOCK


#include <time.h>
#include <sys\time.h>
#include <sys\times.h>

#define OVERFLOW_ON_SECOND
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef        TimHandle;
static uint32_t 				TimerOverflow = 0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void TIM2_IRQHandler() { HAL_TIM_IRQHandler(&TimHandle); }

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */

__attribute__((weak)) void TimerSecondCallback() {}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	TimerOverflow++;
	TimerSecondCallback();
}


uint64_t sys_usec() {
	uint32_t cnt= TIM2->CNT;
	uint64_t ret = TimerOverflow;
	if(cnt > TIM2->CNT){// rare but the timer reset here turned over
		cnt= TIM2->CNT;ret++;
	}
#ifdef OVERFLOW_ON_SECOND
	ret += TimerOverflow * 1000000U;
#else
	ret += TimerOverflow<<32;
#endif
	return ret;
}

int _gettimeofday(struct timeval *__p, void *__tz){
	if(__p) {
#ifdef OVERFLOW_ON_SECOND
		__p->tv_usec = TIM2->CNT;
		__p->tv_sec = TimerOverflow;
		if(__p->tv_usec > TIM2->CNT){// we turned over during this call
			__p->tv_sec++;
			__p->tv_usec = TIM2->CNT;
		}
#else
		uint64_t stamp = sys_usec();
		__p->tv_sec = stamp / 1000000U;
		__p->tv_usec = stamp % 1000000U;
#endif
	}
	return 1;
}

clock_t clock() {
	uint64_t stamp = sys_usec();
	stamp*= _CLOCKS_PER_SEC_;
	stamp /= 1000000U;
	return stamp;
}
uint32_t HAL_GetTick(void)
{
#ifdef OVERFLOW_ON_SECOND
	uint32_t cnt= TIM2->CNT;
	uint32_t ret = TimerOverflow;
	if(cnt > TIM2->CNT){// rare but the timer reset here turned over
		cnt= TIM2->CNT;ret++;
	}
	return ret * 1000U + (cnt / 1000U);
#else
	return TIM2->CNT / 1000U; // TIM is 1MHZ
#endif

}

/**
  * @brief  This function configures the TIM6 as a time base source. 
  *         The time source is configured to have 1ms time base with a dedicated 
  *         Tick interrupt priority. 
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig(). 
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_InitTick (uint32_t TickPriority)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock, uwAPB1Prescaler = 0U;
  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;
  
  TimerOverflow = 0;
    /*Configure the TIM6 IRQ priority */
  // TickPriority RELALY needs to be super high
 // HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority ,0U);
  HAL_NVIC_SetPriority(TIM2_IRQn, 0U ,0U);
  /* Enable the TIM6 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  
  /* Enable TIM6 clock */
  __HAL_RCC_TIM2_CLK_ENABLE();
  
  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
  
  /* Get APB1 prescaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;
  
  /* Compute TIM6 clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1) 
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2*HAL_RCC_GetPCLK1Freq();
  }
  
  /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);
  
  /* Initialize TIM6 */
  TimHandle.Instance = TIM2;
  
  /* Initialize TIMx peripheral as follow:
  + Period = [(TIM6CLK/1000) - 1]. to have a (1/1000) s time base.
  + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
  + ClockDivision = 0
  + Counter direction = Up
  */
 // TimHandle.Init.Period = (1000000U / 1000U) - 1U;
  TimHandle.Init.Period = (1000000U) - 1U; // resets evey secound
 // TimHandle.Init.Period = UINT32_MAX ;// useconds
  TimHandle.Init.Prescaler = uwPrescalerValue;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if(HAL_TIM_Base_Init(&TimHandle) == HAL_OK)
  {
    /* Start the TIM time Base generation in interrupt mode */
    if(HAL_TIM_Base_Start_IT(&TimHandle)== HAL_OK) return HAL_OK;
  }
  assert(0);
  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM6 update interrupt.
  * @retval None
  */
void HAL_SuspendTick(void) // can never be disabled
{
  /* Disable TIM6 update Interrupt */
 // __HAL_TIM_DISABLE_IT(&TimHandle, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM6 update interrupt.
  * @retval None
  */
void HAL_ResumeTick(void)
{
  /* Enable TIM6 Update interrupt */
 // __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}


#endif

