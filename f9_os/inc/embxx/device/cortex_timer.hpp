/*
 * cortex_timer.hpp
 *
 *  Created on: Jun 17, 2017
 *      Author: Paul
 */

#ifndef EMBXX_DEVICE_CORTEX_TIMER_HPP_
#define EMBXX_DEVICE_CORTEX_TIMER_HPP_

#include <cstdint>
#include <cstddef>

#include <cstdint>
#include <cstddef>


#include "cortex_loop.hpp"
#include <embxx/device/irq_mgr.hpp>
#include <embxx/util/Assert.h>
#include <stm32f7xx_hal.h>

namespace embxx {
}
	namespace stm32 {

	template <typename TInterruptMgr,
	          typename THandler = embxx::util::StaticFunction<void (const embxx::error::ErrorStatus&), 20> >
	class Timer
	{
		   enum class STATE {
			   ENABLED,
			   DISABED,
			   WAITING,
			   SUSPENDED
		   };
		    typedef std::uint32_t EntryType;
		    typedef typename InterruptMgr::IrqId IrqId;
	public:
	    typedef uint32_t WaitTimeType;
	    typedef std::chrono::duration<WaitTimeType, std::milli> WaitTimeUnitDuration;

	    typedef TInterruptMgr InterruptMgr;
	    typedef THandler HandlerFunc;

	    Timer(InterruptMgr& interruptMgr)
	    		: interruptMgr_(interruptMgr),
				  state_(STATE::DISABED),
				  overflow_(0),
				  TIMER_(TIM6),
				  TimerFreq_(0)
		    {

	    	  /* Enable TIM6 clock */
	    	  __HAL_RCC_TIM6_CLK_ENABLE();
	    	  /* Return function status */
		    }

	    template <typename TFunc>
	    void setWaitCompleteCallback(TFunc&& handler){
	    	  handler_ = std::forward<TFunc>(handler);
	    }

	    template <typename TContext>
	    void startWait(WaitTimeType waitMs, TContext context){
	        static_cast<void>(context);
	        startWaitInternal(waitMs);
	    }

	    bool cancelWait(embxx::device::context::EventLoop context){
	        static_cast<void>(context);
	        disableInterrupts();
	        if (state_ == STATE::DISABED) return false;
	        state_ = STATE::DISABED;
	        TIMER_->CR1 &= ~TIM_CR1_CEN;
	        return true;
	    }

	    bool suspendWait(embxx::device::context::EventLoop context){
	        static_cast<void>(context);
	        disableInterrupts();
	    	switch(state_) {
	    	case STATE::WAITING:
	    		state_ = STATE::SUSPENDED;
	    		return true;
	    	case STATE::DISABED:
	    	case STATE::ENABLED:
	    	case STATE::SUSPENDED:
	    		return false;
	    	}
	    }
	    void resumeWait(embxx::device::context::EventLoop context) {
	        static_cast<void>(context);
	    	switch(state_) {
	    	case STATE::WAITING:
	    	case STATE::DISABED:
	    	case STATE::ENABLED:
	    		break;
	    	case STATE::SUSPENDED:
	    		state_ = STATE::WAITING;
	    		break;
	    	}
	        enableInterrupts();
	    }

	    uint32_t getElapsed(embxx::device::context::EventLoop context) const {
	    	uint32_t elapsedTicks = TIMER_->ARR - TIMER_->CNT;
	    	uint32_t prescaler = TIMER_->PSC;
	    	 while (0 < prescaler) {
	    	      elapsedTicks <<= 4;
	    	      --prescaler;
	    	 }
			static const unsigned TicksInMillisec = (SysClockFreq / 1000);
			return elapsedTicks / TicksInMillisec;

	    }
	private:
	    void startWaitInternal(WaitTimeType waitMs){
	        GASSERT(!state_ == STATE::WAITING);
	        state_ = STATE::WAITING;
	        configWait(waitMs);
	        enableInterrupts();
	        TIMER_->CR1 |= TIM_CR1_CEN;
	    }
	    uint32_t getTimerFreq() {
	    	  RCC_ClkInitTypeDef    clkconfig;
	    	  uint32_t              uwAPB1Prescaler = 0U;
	    	  uint32_t              uwPrescalerValue = 0U;
	    	  uint32_t              pFLatency;
	    	  /* Get clock configuration */
	    	  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
	    	  uwAPB1Prescaler = clkconfig.APB1CLKDivider;

	    	  /* Compute TIM6 clock */
	    	  if (uwAPB1Prescaler == RCC_HCLK_DIV1)
	    		  return HAL_RCC_GetPCLK1Freq();
	    	  else
	    		  return 2*HAL_RCC_GetPCLK1Freq(); // only do the above once?
	    }
	    bool configWait(WaitTimeType millisecs){
	    	TimerFreq_ = getTimerFreq();

    	  // using a 16 bit timer
    	  static const std::uint64_t TicksInMillisec = (TimerFreq_ / 1000);
    	  static const uint32_t MaxSupportedPrescaler = std::numeric_limits<uint16_t>::max();
    	  static const uint32_t MaxSupportedLoad = std::numeric_limits<uint16_t>::max();
    	  auto totalTicks = TicksInMillisec * millisecs;
    	  unsigned prescaler = 0;
			while (std::numeric_limits<EntryType>::max() < totalTicks && totalTicks >= MaxSupportedLoad) {
			totalTicks >>= 4; // divide by 16;
			prescaler += 1;
			}
			if (MaxSupportedPrescaler < prescaler || MaxSupportedLoad < totalTicks) {
				GASSERT(0);
				return false;
			}
			//__disable_irq();
			TIMER_->SR  = 0; // clear update flag
			TIMER_->CNT = 0; // reset counter?
			TIMER_->ARR = static_cast<uint16_t>(totalTicks);
			TIMER_->PSC = static_cast<uint16_t>(prescaler);
			//__enable_irq();
    	    return true;

    	  /* Compute the prescaler value to have TIM6 counter clock equal to 1MHz */
    	 // uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

	    }
	    void enableInterrupts(){
	    	TIMER_->DIER |= TIM_DIER_UIE;
	        //interruptMgr_.enableInterrupt(IrqId::IrqId_Timer);
	    }
	    void disableInterrupts(){
	    	TIMER_->DIER &= ~TIM_DIER_UIE;
	    }
	    inline void updateHandler() {
	    	state_ = STATE::DISABED;
	        TIMER_->CR1 &= ~TIM_CR1_CEN;
	    }
	    void interruptHandler(){
	    	// if update flag AND we have interrupt enabled
	    	if(TIMER_->SR & TIM_SR_UIF && TIMER_->DIER & TIM_DIER_UIE){
	    		updateHandler();
	    		TIMER_->SR &= ~TIM_SR_UIF;
	    	}
	    }


	    InterruptMgr& interruptMgr_;
	    HandlerFunc handler_;
	    STATE state_;
		uint32_t overflow_;
		uint32_t TimerFreq_;
		TIM_TypeDef*  TIMER_;




	};
}
#endif /* EMBXX_DEVICE_CORTEX_TIMER_HPP_ */
