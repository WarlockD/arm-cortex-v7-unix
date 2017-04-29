
#include "v7_user.h"
#include <sys\time.h>
#include <sys\times.h>
#include <sys\queue.h>
#include "v7_internal.h"
#include <stm32f7xx.h>
#include <stm32746g_discovery.h>
#include <pin_macros.h>
#include "asm.h"

#define CLOCK_PRIOITY 1 /* high piroirty on clock but honestly it just ticks */
static struct timeval arch_time = { 0,0 };
static const struct timeval arch_time_inc = { 0, 1000000U/60 }; /// 60 hz
static TIM_HandleTypeDef TimHandle;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &TimHandle){
		timeradd(&arch_time, &arch_time_inc, &arch_time);
	}
  //HAL_IncTick();
}
time_t v7_time(time_t* t) {
	time_t sec = arch_time.tv_sec;// this is atomic so no locking needed
	if(t) *t=sec;
	return sec;
}
/**
  * @brief  This function handles TIM interrupt request.
  * @retval None
  */
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TimHandle);
}
static void configure_clock() {
	RCC_ClkInitTypeDef    clkconfig;
	uint32_t              uwTimclock, uwAPB1Prescaler = 0U;
	uint32_t              uwPrescalerValue = 0U;
	uint32_t              pFLatency;
	HAL_NVIC_SetPriority(TIM2_IRQn, CLOCK_PRIOITY ,0U);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);  /* Enable the TIM6 global Interrupt */
	__HAL_RCC_TIM6_CLK_ENABLE(); /* Enable TIM6 clock */
	HAL_RCC_GetClockConfig(&clkconfig, &pFLatency); /* Get clock configuration */
	uwAPB1Prescaler = clkconfig.APB1CLKDivider; /* Get APB1 prescaler */
	uwTimclock = HAL_RCC_GetPCLK1Freq();
	if((uwAPB1Prescaler != RCC_HCLK_DIV1) ) uwTimclock*=2;
	/* Compute the prescaler value to have TIM6 counter clock equal to 1MHz */
	uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);
	TimHandle.Instance = TIM2;  /* Initialize TIM2 */
	/* Initialize TIMx peripheral as follow:
	+ Period = [(TIM6CLK/1000) - 1]. to have a (1/1000) s time base.
	+ Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
	+ ClockDivision = 0
	+ Counter direction = Up
	*/
	//TimHandle.Init.Period = (1000000U / 1000U) - 1U; // 1ms
	TimHandle.Init.Period = 1000000U; // 1sec
	//TimHandle.Init.Period = 1000000U/60; //  60hz
	TimHandle.Init.Prescaler = uwPrescalerValue;
	TimHandle.Init.ClockDivision = 0;
	TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
	TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if(HAL_TIM_Base_Init(&TimHandle) == HAL_OK)
	{
	/* Start the TIM time Base generation in interrupt mode */
		HAL_TIM_Base_Start_IT(&TimHandle);
	}
}
void arch_gettimeofday(struct timeval * tv){
	  /* Disable TIM6 update Interrupt */
	  __HAL_TIM_DISABLE_IT(&TimHandle, TIM_IT_UPDATE);
	  *tv = arch_time;
	  /* Enable TIM6 Update interrupt */
	  __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}
void arch_settimeofday(struct timeval * tv){
	  /* Disable TIM6 update Interrupt */
	  __HAL_TIM_DISABLE_IT(&TimHandle, TIM_IT_UPDATE);
	  arch_time = *tv;
	  /* Enable TIM6 Update interrupt */
	  __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
	//osSystickHandler();
}

void show_regs(struct pt_regs * regs);
void asm_do_IRQ(int irq, struct pt_regs *regs)
{

	if(irq < 16) {
		printk("UNHANDLED FAULT! ISR:%d\n", irq);
		show_regs(regs);
		while(1) ;
	} else {
		printk("UNHANDLED IRQ! ISR:%d\n", irq);
		show_regs(regs);

	}

	//printk("ISR %d \n",irq);
	if(regs->ARM_EXC_lr == 0xfffffffd){
		// check if we have work and raise pendf
	}
	while(1) ;
}



void v7_arch_setup() {
	configure_clock();
}
