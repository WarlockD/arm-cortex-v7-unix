#include <assert.h>
#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>
#include <pin_macros.h>
#include <string.h>
#include <stdio.h>

#include <os\printk.h>
#include <sys\time.h>
#include <os\thread.hpp>




#if 0
#include <scm\scmRTOS.h>
typedef TProfiler<0> TProfilerBase;
OS::TRecursiveMutex uart_mutex;

class profiler_t : public TProfilerBase
{
private:
public:
	INLINE profiler_t()
		: TProfilerBase()
	{
		// enable TIM3 clock
		//RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
		// count up to ARR, no divisor, auto reload
		//TIM3->CR1 = 0;
		// period (full scale)
		//TIM3->ARR = 0xFFFF;
		// set prescaler (60MHz/0xFFFF/915 = 1Hz)
		//TIM3->PSC = 915;
		// generate an update event to reload the prescaler value immediately
		//TIM3->EGR = TIM_EGR_UG;
		// run timer
		//TIM3->CR1 |= TIM_CR1_CEN;
	}
	// TimerSecondCallback()
};
/**
 * Profiler object
 */
profiler_t profiler;


template<>
uint32_t TProfilerBase::time_interval()
{
	static uint32_t  prev_ticks=0;
	uint32_t ticks;
#if 0
	ticks = TIM3->CNT;
#else
	struct timeval tv;
	gettimeofday(&tv,NULL);
	ticks = tv.tv_sec ;
#endif
	uint32_t  diff = ticks - prev_ticks;
	prev_ticks=ticks;
	return diff;
}

void OS::context_switch_user_hook() { profiler.advance_counters(); }

typedef OS::process<OS::pr3, 600> TDebugProcess;

namespace OS
{
	template <>
	OS_PROCESS void TDebugProcess::exec()
	{
		printk("\x1B[2J");// clear screen
		for(;;)
		{
			OS::sleep(1000);
			profiler.advance_counters();
			profiler.process_data();
			uart_mutex.lock();

			printk("\x1B[1;1H");// go to 1,1
			printk("\x1B[1m\x1B[37;42m");// set color attributes (white on green)
			printk("Process\tStack\tCPU%\r\n");
			printk("\x1B[1m\x1B[37;40m"); // set color attributes (white on black)

			for(uint_fast8_t i = 0; i < OS::PROCESS_COUNT; i++)
			{
				uint32_t cpu = profiler.get_result(i);
				printk("%d \t %d \t %t\r\n", OS::TPriority(i),
						OS::get_proc(i)->stack_slack() * sizeof(stack_item_t),
						cpu / 100, cpu % 100);
#if 0
				uart << OS::TPriority(i) << '\t'
					<< OS::get_proc(i)->stack_slack() * sizeof(stack_item_t)  << '\t'
					<< (double)cpu / 100 << "\r\n";
#endif
			}
			uart_mutex.unlock();
		}
	}
}

TDebugProcess DebugProcess;
//---------------------------------------------------------------------------
//
//      Process types
//
typedef OS::process<OS::pr0, 300> TProc0;
typedef OS::process<OS::pr1, 300> TProc1;
typedef OS::process<OS::pr2, 300> TProc2;

//---------------------------------------------------------------------------
//
//      Process objects
//
TProc0 Proc0;
TProc1 Proc1;
TProc2 Proc2;
//---------------------------------------------------------------------------
//
//      IO Pins
//


//---------------------------------------------------------------------------
//
//      Event Flags to test
//
OS::TEventFlag event;
OS::TEventFlag timer_event;
#endif

/**
 * Waste some time (payload emulation).
 */
 void waste_time()
{
	for (volatile int i = 0; i < 0x3FF; i++) ;
}

/**
 * Stack angry function.
 * Eats approximately (12 * count) bytes from caller process stack.
 * Called by different processes some time after start.
 * Stack usage changes can be observed in debug terminal.
 */
int waste_stack(int count)
{
	volatile int arr[2];
	arr[0] = TIM2->CNT;	// any volatile register
	arr[1] = count ? waste_stack(count - 1) : TIM2->CNT;
	return (arr[0] + arr[1]) / 2;
}

int simple_mutex;
void process1() {
	using os::kernel;
	timeval_t time;

	int count=0;
   for(;;)
   {
	   simple_mutex++;
	   kernel::sleep(&simple_mutex,kernel::PUSER);
	   simple_mutex--;
	   // waste some time (simulate payload)
	   waste_time();
	   gettimeofday(&time,nullptr);
	   // waste some stack (increasing with time)
	   uint32_t t = (time.tv_usec % 40000) / 5000;
	   waste_stack(t);
	   printk("\x1B[10;1H TProc0 = %d",count++);
   }
}
void process2() {
	using os::kernel;
	timeval_t time;

	int count=0;
   for(;;)
   {
	   if(simple_mutex)
		   kernel::wakeup(&simple_mutex);

       waste_time();
       waste_time();
	   gettimeofday(&time,nullptr);
	   // waste some stack (increasing with time)
	  // uint32_t t = (time.tv_usec % 40000) / 5000;
	  // waste_stack(t);
	   printk("\x1B[11;1H TProc0 = %d",count++);
   }
}
void process3() {
	using os::kernel;
	timeval_t time;

	int count=0;
   for(;;)
   {
	   simple_mutex++;
	   kernel::sleep(&simple_mutex,kernel::PUSER);
	   simple_mutex--;
	   // waste some time (simulate payload)
	   waste_time();
	   gettimeofday(&time,nullptr);
	   // waste some stack (increasing with time)
	   uint32_t t = (time.tv_usec % 40000) / 5000;
	   waste_stack(t);
	   printk("\x1B[12;1H TProc0 = %d",count++);
   }
}
uint32_t proc1_stack[1024];
uint32_t proc2_stack[1024];
uint32_t proc3_stack[1024];
os::proc proc1(proc1_stack, process1);
os::proc proc2(proc2_stack, process2);
os::proc proc3(proc3_stack, process3);

void test_clock(clock_t& start) {
	clock_t current = clock();
	if((current-start) < 100) return;
	printk("CLOCK %d!\r\n",current);
	start = current;
}
void test_time(timeval_t& start) {
	constexpr static timeval_t sec = {1,0 };
	timeval_t current;
	gettimeofday(&current,NULL);
	if((current-start) < sec) return;
	printk("TIMEOFDAY!\r\n");
	start = current;
}
#include "mimix_cpp/fs.hpp"

extern "C" void scmrtos_test_start()
{
	printk("starting os!\n");


	//os::kernel::start_os();

	clock_t start = clock();
	timeval_t start_tv;
	gettimeofday(&start_tv,NULL);

	while(1) {
		test_clock(start);
		test_time(start_tv);
	}
    // configure IO pins
  //  PE0::Direct(OUTPUT);
   // PE0::Off();
  //  PE1::Direct(OUTPUT);
  //  PE1::Off();

}


#if 0
namespace OS
{
    template <>
    OS_PROCESS void TProc0::exec()
    {
    	int count=0;
        for(;;)
        {
        	// PE0 "ON" time = context switch time (~9.6us at 24MHz)
        	event.wait();
        //    PE0::Off();

            // waste some time (simulate payload)
            waste_time();

            // waste some stack (increasing with time)
            tick_count_t t = (OS::get_tick_count() % 40000) / 5000;
            waste_stack(t);
            uart_mutex.lock();
            printk("\x1B[10;1H TProc0 = %d",count++);
            uart_mutex.unlock();
        }
    }

    template <>
    OS_PROCESS void TProc1::exec()
    {
    	int count=0;
        for(;;)
        {
            sleep(10);
         //   PE0::On();
            event.signal();
            // waste time (2x Proc0)
            waste_time();
            waste_time();
            uart_mutex.lock();
            printk("\x1B[11;1H TProc1 = %d",count++);
            uart_mutex.unlock();
        }
    }

    template <>
    OS_PROCESS void TProc2::exec()
    {
    	int count=0;
        for (;;)
        {
            timer_event.wait();

      //      PE1::On();

            // increase load, one step at every 5 seconds after start,
            // resetting at 8th step.
            tick_count_t t = (OS::get_tick_count() % 40000) / 5000;
            for (uint32_t i = 0; i < t; i++)
                waste_time();

            // PE1 led "ON" time ~ Proc2 load
         //   PE1::Off();
            uart_mutex.lock();
            printk("\x1B[12;1H TProc2 = %d",count++);
            uart_mutex.unlock();
        }

    }
}

void OS::system_timer_user_hook()
{
    static const int reload_value = 10;	// 100 Hz
    static int counter = reload_value;
    if (!--counter)
    {
        counter = reload_value;
        timer_event.signal_isr();
    }
}

#if scmRTOS_IDLE_HOOK_ENABLE
void OS::idle_process_user_hook()
{
	__WFI();
}
#endif
#endif
