#include <assert.h>
#include <stm32746g_discovery.h>
#include <stm32746g_discovery_qspi.h>
#include <stm32746g_discovery_sdram.h>
#include <pin_macros.h>
#include <string.h>
#include <stdio.h>

#include <os\printk.h>
#include <sys\time.h>

#include "f9\thread.hpp"

#include <os\printk.hpp>
#include "scm\scmRTOS.h"
#include "scm\os_serial.hpp"
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

typedef OS::process<OS::pr0, 512> TProc1;
typedef OS::process<OS::pr1, 512> TProc2;
typedef OS::process<OS::pr2, 512> TProc3;
extern UART_HandleTypeDef huart1;
OS::STM32F7_UART test(&huart1);

template<size_t _BUFFER_SIZE>
struct TLog {
	static constexpr size_t BUFFER_SIZE=_BUFFER_SIZE;
	char _buffer[BUFFER_SIZE+1];
	bool filled = false;
	void vprint(const char* fmt, va_list va) {
		while(filled) {
			OS::sleep(100);
		}
		int len = vsnprintf(_buffer,BUFFER_SIZE,fmt,va);
		if(len > 0 && len < BUFFER_SIZE) {
			if(_buffer[len-1] != '\n') {
				_buffer[len++] = '\n';
				_buffer[len++] = 0;
			}
			filled = true;
			test.write(_buffer,len);
		}
	}
	void print(const char* fmt,...) {
		va_list va;
		va_start(va,fmt);
		vprint(fmt,va);
		va_end(va);
	}
};
TLog<512> log;

TProc1 Proc1;
TProc2 Proc2;
TProc3 Proc3;
//
//      Test objects
//
struct TMamont                   //  data type for sending by message
{                                //
    enum TSource
    {
        PROC_SRC,
        ISR_SRC
    }
    src;
    int data;                    //
};                               //


OS::message<TMamont> MamontMsg;  // OS::message object

namespace OS
{
    template <>
    OS_PROCESS void TProc1::exec()
    {
        for(;;)
        {
            MamontMsg.wait();               // wait for message
            TMamont Mamont = MamontMsg;     // read message content into local TMamont variable

            if (Mamont.src == TMamont::PROC_SRC)
            {
            	log.print("TProc1: Message recived from Process (%d)\n"); // show that message received from other process
            }
            else
            {
            	log.print("TProc1: Message recived from isr (%d)\n"); // show that message received from other process
            }
        }
    }

    template <>
    OS_PROCESS void TProc2::exec()
    {
        for(;;)
        {
            sleep(100);
        }
    }

    template <>
    OS_PROCESS void TProc3::exec()
    {
        for (;;)
        {
            sleep(1);
            TMamont m;                      // create message content
            m.src  = TMamont::PROC_SRC;
            m.data = 5;
            MamontMsg = m;                  // put the content to the OS::message object
            log.print("TProc3: Message sending\n"); // show that message received from other process
            MamontMsg.send();               // send the message
        }
    }
}

void OS::system_timer_user_hook()
{
   // TMamont m;                              // create message content
   // m.src  = TMamont::ISR_SRC;
   // m.data = 10;
  //  MamontMsg = m;                          // put the content to the OS::message object
  //  PB0.On();
    MamontMsg.send_isr();                   // send the message
}

#if scmRTOS_IDLE_HOOK_ENABLE
void OS::idle_process_user_hook()
{
	__WFI();
}
#endif


extern "C" void scmrtos_test_start()
{
	printk("starting os!\n");
	OS::run();
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
