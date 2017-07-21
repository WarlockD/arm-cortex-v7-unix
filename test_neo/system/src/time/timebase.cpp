
#include "stm32f7xx_hal.h"
#include <diag\Trace.h>
#include <assert.h>

#define MY_CLOCK
#ifdef  MY_CLOCK


#include <time.h>
#include <sys\time.h>
#include <sys\times.h>

#include <errno.h>

#ifdef errno
#undef errno
#endif
extern int errno;
extern "C" uint64_t sys_usec();
#define USEC (1000000U)
#define MSEC (1000U)

#define HAL_INCRMENT (USEC/(MSEC))-1
volatile uint32_t HAL_Ticks = 0;


struct timer_config {
	void* arg;
	time_t (*callback)(void*);
	time_t usec_left;
};

// disabed so I don't have second interrupts each time
//#define OVERFLOW_ON_SECOND

#ifdef OVERFLOW_ON_SECOND
#define OC_MAX_INCRMENT (USEC-1)
#else
#define OC_MAX_INCRMENT UINT32_MAX
#endif

static volatile uint32_t 		TimerOverflow = 0;
static volatile timer_config	g_timer_info[4];
// set up posic timers from comp
template<typename FUNC>
__attribute__((always_inline)) static inline void run_tim_flag(TIM_TypeDef* TIM, const uint32_t flag, FUNC func ) {
	if((TIM->DIER & flag) && (TIM->SR & flag)) {
		// we can get away with this because the intterrupt enable is in the same place as the SR
		func();
		TIM2->SR &= ~(flag); // clear it
	}
}
static inline uint32_t process_timer(int index) {
	auto& info = g_timer_info[index];
	uint32_t cnt = info.usec_left;
	if(info.usec_left == 0)
		cnt = info.callback(info.arg);
	if(cnt == 0) return 0;
	else if(cnt > OC_MAX_INCRMENT){
		info.usec_left = cnt - OC_MAX_INCRMENT;
		return OC_MAX_INCRMENT;
	} else {
		return cnt;
	}
}

__attribute__((weak)) void TimeBaseSecondCallback()  {}
extern "C" void TIM2_IRQHandler() {
	static_assert(TIM_DIER_UIE == TIM_SR_UIF, "Safety check");
	run_tim_flag(TIM2, TIM_SR_UIF,[]() {
		TimerOverflow++;
	#ifdef OVERFLOW_ON_SECOND
		TimeBaseSecondCallback();
	#endif
	});
	static_assert(TIM_DIER_CC1IE == TIM_SR_CC1IF, "Safety check");
	run_tim_flag(TIM2, TIM_SR_CC1IF,[]() {
		uint32_t cnt = process_timer(0);
		if(cnt == 0) {
			TIM2->DIER &= TIM_DIER_CC1IE; // disable the interrupt
		} else {
			TIM2->CCR1 = cnt; // set new count regester
		}
	});
	static_assert(TIM_DIER_CC2IE == TIM_SR_CC2IF, "Safety check");
	run_tim_flag(TIM2, TIM_SR_CC2IF,[]() {
		uint32_t cnt = process_timer(1);
		if(cnt == 0) {
			TIM2->DIER &= TIM_DIER_CC2IE; // disable the interrupt
		} else {
			TIM2->CCR2 = cnt; // set new count regester
		}
	});
	static_assert(TIM_DIER_CC3IE == TIM_SR_CC3IF, "Safety check");
	run_tim_flag(TIM2, TIM_SR_CC3IF,[]() {
		uint32_t cnt = process_timer(2);
		if(cnt == 0) {
			TIM2->DIER &= TIM_DIER_CC3IE; // disable the interrupt
		} else {
			TIM2->CCR3 = cnt; // set new count regester
		}
	});
	static_assert(TIM_DIER_CC4IE == TIM_SR_CC4IF, "Safety check");
	run_tim_flag(TIM2, TIM_SR_CC4IF,[]() {
		uint32_t cnt = process_timer(3);
		if(cnt == 0) {
			TIM2->DIER &= TIM_DIER_CC4IE; // disable the interrupt
		} else {
			TIM2->CCR4 = cnt; // set new count regester
		}
	});
}



// this is pure atomic
extern "C" uint64_t sys_usec() {
	uint32_t counter= TIM2->CNT;
	uint64_t overflow = TimerOverflow;
	if(counter > TIM2->CNT){// rare but the timer reset here turned over
		overflow= TIM2->CNT;
		++overflow;
	}
#ifdef OVERFLOW_ON_SECOND
	overflow *= USEC;
	overflow += counter;
#else
	overflow *= UINT32_MAX;
	overflow += counter;
#endif
	return overflow;
}
extern "C"  int clock_gettime(clockid_t clk_id, struct timespec *tp){
	(void)clk_id; // clock id dosn't matter right now
	if(tp){
		uint32_t counter= TIM2->CNT;
		uint32_t overflow = TimerOverflow;
		if(counter > TIM2->CNT){// rare but the timer reset here turned over
			overflow= TIM2->CNT;
			++overflow;
		}
		tp->tv_sec = overflow;
		tp->tv_nsec = counter * 1000U; // to nsec
		return 0;
	}
	errno = EFAULT;
	return -1;

}
// pure atomic
extern "C" void microtime(struct timeval* tv) {
	assert(tv);
#ifdef OVERFLOW_ON_SECOND
	uint32_t counter= TIM2->CNT;
	uint32_t overflow = TimerOverflow;
	if(counter > TIM2->CNT){// rare but the timer reset here turned over
		overflow= TIM2->CNT;
		++overflow;
	}
	tv->tv_sec = overflow;
	tv->tv_usec = counter;
#else
	uint32_t counter= TIM2->CNT;
	uint64_t overflow = TimerOverflow;
	overflow *= UINT32_MAX;
	if(counter > TIM2->CNT) {// rare but the timer reset here turned over
		overflow= TIM2->CNT;
		overflow+=UINT32_MAX;
	}
	overflow+= counter;
	tv->tv_sec = counter / USEC;
	tv->tv_usec = counter  % USEC;
#endif
}


extern "C" clock_t _times (struct tms *buf)
{
	uint64_t stamp = sys_usec();
	if(buf) {
		uint64_t sec = stamp/ USEC;
		buf->tms_utime  = sec;
		buf->tms_stime   = sec;
		buf->tms_cutime   = sec;
		buf->tms_cstime   = sec;
	}
	stamp*= _CLOCKS_PER_SEC_;
	stamp /= USEC;
 // errno = EINVAL;
  return stamp;
}

extern "C" clock_t clock() {
	uint64_t stamp = sys_usec();
	stamp*= _CLOCKS_PER_SEC_;
	stamp /= USEC;
	return stamp;
}

extern "C" int _gettimeofday(struct timeval *__p, void *__tz){
	if(__p) {
		microtime(__p);
		return -1;
	}
	if(__tz) {
		(void)__tz;
	}
	return 0;
}

#define MAX_MSEC (0xFFFFFFFF/MSEC)

extern "C" void HAL_Delay(__IO uint32_t Delay)
{
	if(Delay < OC_MAX_INCRMENT){
		Delay *= 1000U;
		 uint32_t tickstart = TIM2->CNT;
		 while((TIM2->CNT - tickstart) < Delay) {}
	} else {
		assert(0); // wow, thats a looong time
	}
#if 0
  uint32_t tickstart = 0;
  tickstart = HAL_GetTick();
  while((HAL_GetTick() - tickstart) < Delay)
  {
  }
#endif
}

extern "C" uint32_t HAL_GetTick(void)
{
	uint64_t stamp = sys_usec();
	return stamp / 1000U;

}
#include <os/irq.h>
void isr_mem_test() {
	TIM2_IRQHandler() ;
}

// configures TIM2 as a clock source
extern "C" HAL_StatusTypeDef HAL_InitTick (uint32_t TickPriority)
{
  TIM2->CR1 = 0;	// disable evetyhing
  TIM2->CR2 = 0;	// disable evetyhing
  TIM2->DIER = 0;	// and disable interrupts
  TIM2->SR = 0;		// and make sure the flags are cleared
  TimerOverflow = 0;

  // TickPriority RELALY needs to be super high
 // HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority ,0U);
  install_isr(TIM2_IRQn,isr_mem_test);

  HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority ,0U);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  __HAL_RCC_TIM2_CLK_ENABLE();

  /* Compute TIM2 clock */
  uint32_t uwTimclock = HAL_RCC_GetPCLK1Freq();
  if ((uint32_t)(RCC->CFGR & RCC_CFGR_PPRE1) != 0) // ABA prescaler
    uwTimclock *= 2;

  /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
  TIM2->PSC  = (uint32_t) ((uwTimclock / USEC) - 1U);
  TIM2->CNT = 0; // count reset
  TIM2->ARR = OC_MAX_INCRMENT ;// auto reload
#if 1
  // not sure we need below but just in case?
  TIM2->EGR = TIM_EGR_UG; // generate to update the presaler, only for tim1 and tim8?
  TIM2->CR1 |= TIM_CR1_CEN; // enable timer
  while((TIM2->SR & TIM_SR_UIF)==0);
  TIM2->CR1 &= ~TIM_CR1_CEN; // disable timer
  TIM2->SR = 0; // clear all flags
#endif
  // now we start it up!
  TIM2->CR1 |= TIM_CR1_CEN; // enable timer
  TIM2->DIER |= TIM_DIER_UIE; // enable interrput
  /* Return function status */
  return HAL_OK;
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
