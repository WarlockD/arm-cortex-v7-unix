/*
 * ktimer.cpp
 *
 *  Created on: May 19, 2017
 *      Author: Paul
 */

#include "ktimer.hpp"
#include "irq.hpp"
#include "schd.hpp"
#if defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
#include "tickless-verify.h"
#endif

namespace f9 {
	DECLARE_KTABLE(ktimer_event_t, ktimer_event_table, CONFIG_MAX_KT_EVENTS);
	/* Next chain of events which will be executed */
	ktimer_event_t *event_queue = nullptr;

	static uint64_t ktimer_now;
	static uint32_t ktimer_enabled = 0;
	static uint32_t ktimer_delta = 0;
	static long long ktimer_time = 0;
	//extern uint32_t SystemCoreClock;
	//HAL_RCC_GetHCLKFreq() == SystemCoreClock
	static void init_systick(uint32_t tick_reload, uint32_t tick_next_reload)
	{
		/* 250us at 168Mhz */
		SysTick->LOAD = tick_reload - 1;
		SysTick->VAL = 0;
		SysTick->CTRL = 0x00000007;
		if (tick_next_reload)
			SysTick->LOAD = tick_next_reload - 1;
	}
	static void systick_disable() { SysTick->CTRL = 0x00000000;}
	static uint32_t systick_now(){ return SysTick->VAL;}
	static uint32_t systick_flag_count() { return (SysTick->CTRL & (1 << 16)) >> 16;}
	static void ktimer_init(void)
	{
		init_systick(CONFIG_KTIMER_HEARTBEAT, 0);
	}

	void ktimer_event_init()
	{
		NVIC_SetPriority(SysTick_IRQn, 1);// one higher than 0
		NVIC_SetPriority(PendSV_IRQn, 0xFF);// highest it can ever go
	//	ktable_init(&ktimer_event_table);
		ktimer_init();
	//	softirq_register(KTE_SOFTIRQ, ktimer_event_handler);
	}
	//INIT_HOOK(ktimer_event_init, INIT_LEVEL::KERNEL);
	static void ktimer_disable(void)
	{
		if (ktimer_enabled) {
			ktimer_enabled = 0;
		}
	}
	static void ktimer_enable(uint32_t delta)
	{
		if (!ktimer_enabled) {
			ktimer_delta = delta;
			ktimer_time = 0;
			ktimer_enabled = 1;

	#if defined(CONFIG_KDB) && \
	    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
			tickless_verify_start(ktimer_now, ktimer_delta);
	#endif	/* CONFIG_KDB */
		}
	}
	ktimer_event_t::~ktimer_event_t(){
		assert(next==nullptr);
	}
	class ktimer_softirq_handler_t : public softirq_t {
		void run() final {
			ktimer_event_t *event = event_queue;
				ktimer_event_t *last_event = NULL;
				ktimer_event_t *next_event = NULL;
				uint32_t h_retvalue  = 0;

				if (!event_queue) {
					/* That is bug if we are here */
					dbg::print(dbg::DL_KTIMER, "KTE: OOPS! handler found no events\n");

					ktimer_disable();
					return;
				}

				/* Search last event in event chain */
				do {
					event = event->next;
				} while (event && event->delta == 0);

				last_event = event;

				/* All rescheduled events will be scheduled after last event */
				event = event_queue;
				event_queue = last_event;

				/* walk chain */
				do {
					h_retvalue = event->handler(event);
					next_event = event->next;

					if (h_retvalue != 0x0) {
						dbg::print(dbg::DL_KTIMER,
						           "KTE: Handled and rescheduled event %p @%ld\n",
						           event, ktimer_now);
						event->schedule(h_retvalue);
					} else {
						dbg::print(dbg::DL_KTIMER,
						           "KTE: Handled event %p @%ld\n",
						           event, ktimer_now);
						event->next = nullptr; // sanity so delete dosn't fail
						delete event;
					}

					event = next_event;	/* Guaranteed to be next
								   regardless of re-scheduling */
				} while (next_event && next_event != last_event);

				if (event_queue) {
					/* Reset ktimer */
					ktimer_enable(event_queue->delta);
				}
		}
	public:
		ktimer_softirq_handler_t() : softirq_t(SOFTIRQ::KTE) {}
	};

	ktimer_softirq_handler_t ktimer_softirq_handler;

	static void __ktimer_handler(void)
	{
		++ktimer_now;

		if (ktimer_enabled && ktimer_delta > 0) {
			++ktimer_time;
			--ktimer_delta;

			if (ktimer_delta == 0) {
				ktimer_enabled = 0;
				ktimer_time = ktimer_delta = 0;

	#if defined(CONFIG_KDB) && \
	    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
				tickless_verify_stop(ktimer_now);
	#endif	/* CONFIG_KDB */
				ktimer_softirq_handler.schedule();
			}
		}
	}

#ifdef CONFIG_KDB
void kdb_show_ktimer(void)
{
	dbg_printf(DL_KDB, "Now is %ld\n", ktimer_now);

	if (ktimer_enabled) {
		dbg_printf(DL_KDB,
		           "Ktimer T=%d D=%d\n", ktimer_time, ktimer_delta);
	}
}

#if defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
void kdb_show_tickless_verify(void)
{
	static int init = 0;
	int32_t avg;
	int times;

	if (init == 0) {
		dbg_printf(DL_KDB, "Init tickless verification...\n");
		tickless_verify_init();
		init++;
	} else {
		avg = tickless_verify_stat(&times);
		dbg_printf(DL_KDB, "Times: %d\nAverage: %d\n", times, avg);
	}
}
#endif
#endif	/* CONFIG_KDB */

	void ktimer_event_t::event_recalc(uint32_t new_delta){
		dbg::print(dbg::DL_KTIMER,
				   "KTE: Recalculated event %p D=%d -> %d\n",
				   this, this->delta, this->delta - new_delta);
			this->delta -= new_delta;
	}
	ktimer_event_t* ktimer_event_t::create(uint32_t ticks, ktimer_event_handler_t handler,void *data){
		ktimer_event_t *kte = nullptr;
		do {
			if(handler == nullptr) break;
			ktimer_event_t *kte = new ktimer_event_t(handler,data);
			if(kte == nullptr) break;
			if(!kte->schedule(ticks)) break;
			return kte;
		} while(0);
		if(kte) delete kte;
		return nullptr;
	}
	bool ktimer_event_t::schedule(uint32_t ticks) {
		long etime = 0, delta = 0;
			ktimer_event_t *event = nullptr, *next_event = nullptr;

			if (!ticks) return false;
			ticks -= ktimer_time;
			this->next = nullptr;

			if (event_queue == nullptr) {
				/* All other events are already handled, so simply schedule
				 * and enable timer
				 */
				dbg::print(dbg::DL_KTIMER,
				           "KTE: Scheduled dummy event %p on %d\n",
				           this, ticks);

				this->delta = ticks;
				event_queue = this;

				ktimer_enable(ticks);
			} else {
				/* etime is total delta for event from now (-ktimer_value())
				 * on each iteration we add delta between events.
				 *
				 * Search for event chain until etime is larger than ticks
				 * e.g ticks = 80
				 *
				 * 0---17------------60----------------60---...
				 *                   ^                 ^
				 *                   |           (etime + next_event->delta) =
				 *                   |           = 120 - 17 = 103
				 *                               etime = 60 - 17 =  43
				 *
				 * kte is between event(60) and event(120),
				 * delta = 80 - 43 = 37
				 * insert and recalculate:
				 *
				 * 0---17------------60-------37-------23---...
				 *
				 * */
				next_event = event_queue;

				if (ticks >= event_queue->delta) {
					do {
						event = next_event;
						etime += event->delta;
						delta = ticks - etime;
						next_event = event->next;
					} while (next_event &&
					         ticks > (etime + next_event->delta));

					dbg::print(dbg::DL_KTIMER,
					           "KTE: Scheduled event %p [%p:%p] with "
					           "D=%d and T=%d\n",
					           this, event, next_event, delta, ticks);

					/* Insert into chain and recalculate */
					event->next = this;
				} else {
					/* Event should be scheduled before earlier event */
					dbg::print(dbg::DL_KTIMER,
					           "KTE: Scheduled early event %p with T=%d\n",
					           this, ticks);

					event_queue = this;
					delta = ticks;

					/* Reset timer */
					ktimer_enable(ticks);
				}

				/* Chaining events */
				if (delta < static_cast<int>(CONFIG_KTIMER_MINTICKS))
					delta = 0;

				this->next = next_event;
				this->delta = delta;
				next_event->event_recalc(delta);
			}

			return 0;
	}


#ifdef CONFIG_KDB
void kdb_dump_events(void)
{
	ktimer_event_t *event = event_queue;

	dbg_puts("\nktimer events: \n");
	dbg_printf(DL_KDB, "%8s %12s\n", "EVENT", "DELTA");

	while (event) {
		dbg_printf(DL_KDB, "%p %12d\n", event, event->delta);

		event = event->next;
	}
}
#endif


#ifdef CONFIG_KTIMER_TICKLESS

#define KTIMER_MAXTICKS (SYSTICK_MAXRELOAD / CONFIG_KTIMER_HEARTBEAT)

static uint32_t volatile ktimer_tickless_compensation = CONFIG_KTIMER_TICKLESS_COMPENSATION;
static uint32_t volatile ktimer_tickless_int_compensation = CONFIG_KTIMER_TICKLESS_INT_COMPENSATION;

void ktimer_enter_tickless()
{
	uint32_t tickless_delta;
	uint32_t reload;

	irq_disable();

	if (ktimer_enabled && ktimer_delta <= KTIMER_MAXTICKS) {
		tickless_delta = ktimer_delta;
	} else {
		tickless_delta = KTIMER_MAXTICKS;
	}

	/* Minus 1 for current value */
	tickless_delta -= 1;

	reload = CONFIG_KTIMER_HEARTBEAT * tickless_delta;

	reload += systick_now() - ktimer_tickless_compensation;

	if (reload > 2) {
		init_systick(reload, CONFIG_KTIMER_HEARTBEAT);

#if defined(CONFIG_KDB) && \
    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
		tickless_verify_count();
#endif
	}

	wait_for_interrupt();

	if (!systick_flag_count()) {
		uint32_t tickless_rest = (systick_now() / CONFIG_KTIMER_HEARTBEAT);

		if (tickless_rest > 0) {
			int reload_overflow;

			tickless_delta = tickless_delta - tickless_rest;

			reload = systick_now() % CONFIG_KTIMER_HEARTBEAT - ktimer_tickless_int_compensation;
			reload_overflow = reload < 0;
			reload += reload_overflow * CONFIG_KTIMER_HEARTBEAT;

			init_systick(reload, CONFIG_KTIMER_HEARTBEAT);

			if (reload_overflow) {
				tickless_delta++;
			}

#if defined(CONFIG_KDB) && \
    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
			tickless_verify_count_int();
#endif
		}
	}

	ktimer_time += tickless_delta;
	ktimer_delta -= tickless_delta;
	ktimer_now += tickless_delta;

	irq_enable();
}
#endif /* CONFIG_KTIMER_TICKLESS */

}; /* namespace f9 */















IRQ_HANDLER(SysTick_Handler, f9::__ktimer_handler);
