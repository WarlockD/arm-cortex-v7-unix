/*
 * clock.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Paul
 */

#include "clock.hpp"
#include <stm32f7xx.h>

namespace mimx {

	uint32_t systick_now() { return SysTick->VAL; }
	void systick_disable() { SysTick->CTRL = 0x00000000; }


	uint32_t systick_flag_count()
	{
		return (SysTick->CTRL & (1 << 16)) >> 16;
	}
	void init_systick(uint32_t tick_reload, uint32_t tick_next_reload)
	{
		/* 250us at 168Mhz */
		SysTick->LOAD = tick_reload - 1;
		SysTick->VAL = 0;
		SysTick->CTRL = 0x00000007;

		if (tick_next_reload)
			SysTick->LOAD  = tick_next_reload - 1;
	}
	constexpr uint32_t CONFIG_KTIMER_MINTICKS = 128;
	using ktimer_queue_t = tailq::head<ktimer,&ktimer::_link>;
	static ktimer_queue_t event_queue; /* pointers to ready list headers */
	static ktimer* KTIMER_MARK = reinterpret_cast<ktimer*>(0x123456789);
	static uint64_t ktimer_now=0;
	static bool ktimer_enabled = false;
	static uint32_t ktimer_delta = 0;
	static long long ktimer_time = 0;
	tickless_verify tickless;
	class softirq_ktimers_t : public softirq {
		void handler() final;
	public:
		softirq_ktimers_t() {}
	};
	softirq_ktimers_t softirq_ktimers;
	static void ktimer_disable() { if(ktimer_enabled) ktimer_enabled = false; }
	static void ktimer_enable(uint32_t delta) {
		if(!ktimer_enabled) {
			ktimer_delta = delta;
			ktimer_time = 0;
			ktimer_enabled = false;
		}
		if(ktimer_tickless_enable)
			tickless.start(ktimer_now, ktimer_delta);
	}

	void ktimer::recalc(uint32_t new_delta){
			printk("KTE: Recalculated event %p D=%d -> %d\n", this, _delta, _delta - new_delta);
			_delta -= new_delta;
	}
	static void __ktimer_handler(void)
	{
		++ktimer_now;

		if (ktimer_enabled && ktimer_delta > 0) {
			++ktimer_time;
			--ktimer_delta;

			if (ktimer_delta == 0) {
				ktimer_enabled = 0;
				ktimer_time = ktimer_delta = 0;

				if(mimx::ktimer_tickless_enable) mimx::tickless.stop(mimx::ktimer_now);
				softirq_ktimers.schedule();
			}
		}
	}
	ktimer::ktimer() :_delta(0) {
		_link.tqe_next = KTIMER_MARK; // mark it
	}
	void ktimer::unschedule(){
		if (_link.tqe_next == KTIMER_MARK) return;  // its already removed
		event_queue.remove(this);
		_link.tqe_next = KTIMER_MARK; // mark it
	}
	void ktimer::schedule(uint32_t ticks){
			long etime = 0, delta = 0;

			if (!ticks || _link.tqe_next != KTIMER_MARK) return;  // its already waiting
			ticks -= ktimer_time;
			_link.tqe_next = nullptr;

			if (event_queue.empty()) {
				/* All other events are already handled, so simply schedule
				 * and enable timer
				 */
				printk("KTE: Scheduled dummy event %p on %d\n",  this, ticks);
				_delta = ticks;
				event_queue.push_front(this);
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
				auto next_event = event_queue.begin();
				if (ticks >= event_queue.front()._delta) {
					decltype(next_event) event;
					do {
						event = next_event++;
						etime += event->_delta;
						delta = ticks - etime;
					} while (next_event!= event_queue.end() && ticks > (etime + next_event->_delta));

					printk("KTE: Scheduled event %p [%p:%p] with "
					           "D=%d and T=%d\n",
					           this, event, next_event, delta, ticks);

					/* Insert into chain and recalculate */
					event_queue.insert(event,this);
				} else {
					/* Event should be scheduled before earlier event */
					printk("KTE: Scheduled early event %p with T=%d\n",
					           this, ticks);
					event_queue.push_front(this);
					delta = ticks;

					/* Reset timer */
					ktimer_enable(ticks);
				}
				if(delta < (long)CONFIG_KTIMER_MINTICKS) delta = 0;
				_delta = delta;
				next_event->recalc(delta);
			}
	}
	void ktimer::execute(){

	}
	void softirq_ktimers_t::handler(){
		if(event_queue.empty()) {
			/* That is bug if we are here */
			printk("KTE: OOPS! handler found no events\n");
			assert(0);
			ktimer_disable();
			return;
		}
		auto event = event_queue.begin();
		while(++event != event_queue.end() && event->_delta ==0);
		decltype(event_queue) tmp;
		tmp.splice(tmp.begin(),event_queue, event_queue.begin(), event);
		for(event = tmp.begin(); event != tmp.end(); ){
			auto c = event++;
			c->unschedule();
			auto delta = c->handler();
			if(delta != 0x0) {
				printk("KTE: Handled and rescheduled event %p @%ld\n",
				           event, ktimer_now);
				c->schedule(delta);
			} else {
				printk("KTE: Handled event %p @%ld\n",
				           event, ktimer_now);
			}
		}
		if (!event_queue.empty()) {
			/* Reset ktimer */
			ktimer_enable(event_queue.front()._delta);
		}

}


};

extern "C" void SysTick_Handler() {
	mimx::__ktimer_handler();
	mimx::request_schedule();
}
