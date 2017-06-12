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

    \file   kerneltimer.cpp

    \brief  Kernel Timer Implementation for ARM Cortex-M4
*/

#include "kerneltypes.h"
#include "kerneltimer.h"
#include "threadport.h"
#include "scheduler.h"
#include "quantum.h"

#include <stm32f7xx.h>
extern uint32_t SystemCoreClock;

volatile bool timer_running = false;
extern "C" void SysTick_Handler(void)
{
	if(timer_running) {
#if KERNEL_USE_TIMERS
    TimerScheduler::Process();
#endif
#if KERNEL_USE_QUANTUM
    Quantum::UpdateTimer();
#endif
	}

    // Clear the systick interrupt pending bit.
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::Read(void)
{
    return SysTick->VAL;
}
static inline uint32_t ms_to_ticks(uint32_t v) { return (SystemCoreClock/v) -1U; }
void KernelTimer::Config(void)
{
	timer_running = false;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk; // we turn off the systick completely
	SysTick->VAL   = 0UL;   // zero time
	//SysTick->LOAD  = TIMER_FREQ -1U;
    SysTick->LOAD  = ms_to_ticks(1000);                        /* set reload register */
    uint8_t u8Priority = (uint8_t)((1 << __NVIC_PRIO_BITS) - 2);
    NVIC_SetPriority(SysTick_IRQn, u8Priority);
}

//---------------------------------------------------------------------------
void KernelTimer::Start(void)
{    
	if(!timer_running){
	    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
		SysTick->CTRL |=(SysTick_CTRL_ENABLE_Msk |SysTick_CTRL_TICKINT_Msk);
		timer_running = true;
	}

}

//---------------------------------------------------------------------------
void KernelTimer::Stop(void)
{
	if(timer_running){
		timer_running = false;
		SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk |SysTick_CTRL_TICKINT_Msk);
	}
}


//---------------------------------------------------------------------------
uint32_t KernelTimer::SubtractExpiry(uint32_t u32Interval_)
{
	SysTick->LOAD  -= u32Interval_;
    return SysTick->LOAD;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::TimeToExpiry(void)
{
    return SysTick->VAL; // we count down
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::GetOvertime(void)
{
	//return (*SysTick->CTRL & (1 << 16)) >> 16;
    return SysTick->LOAD - SysTick->VAL;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::SetExpiry(uint32_t u32Interval_)
{    
	SysTick->LOAD = u32Interval_;
    return SysTick->LOAD;
}

//---------------------------------------------------------------------------
void KernelTimer::ClearExpiry(void)
{
    // Clear the systick interrupt pending bit.
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
}

//-------------------------------------------------------------------------
uint8_t KernelTimer::DI(void)
{
	uint8_t prev = (SysTick->CTRL &SysTick_CTRL_TICKINT_Msk) ? 1 : 0 ;
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    return prev;
}

//---------------------------------------------------------------------------
void KernelTimer::EI(void)
{    
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

//---------------------------------------------------------------------------
void KernelTimer::RI(bool bEnable_)
{
	if(bEnable_)
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	else
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

//---------------------------------------------------------------------------
