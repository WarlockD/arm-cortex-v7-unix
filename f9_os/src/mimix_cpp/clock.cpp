/*
 * clock.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Paul
 */

#include "clock.hpp"
#include "context.hpp"
#include <stm32f7xx.h>

namespace {
	constexpr uint32_t CONFIG_MAX_KT_EVENTS = 64;
	constexpr bool CONFIG_KTIMER_TICKLESS = true;
	constexpr bool CONFIG_KTIMER_TICKLESS_VERIFY = false;
	constexpr int CONFIG_KTIMER_TICKLESS_COMPENSATION = 0;
	constexpr uint32_t CONFIG_KTIMER_HEARTBEAT = 65536;
	constexpr uint32_t CONFIG_KTIMER_MINTICKS = 128;

	static uint32_t ktiimer_next_handle() {
		static uint32_t g_ktimer_handles = 1;
		if(g_ktimer_handles == 0) ++g_ktimer_handles; // never equals 0
		return g_ktimer_handles++;
	}
	struct ktimer_event_t {
		mimx::ktimer_callback callback;
		void* data;
		const uint32_t handle = ktiimer_next_handle(); // this works?
		uint32_t delta = 0;
		tailq::entry<ktimer_event_t> link;
		ktimer_event_t(mimx::ktimer_callback callback, void* data) : callback(callback), data(data) {}
		static void* operator new(size_t size);
		static void operator delete(void*);
		void recalc(uint32_t new_delta) {
			printk("KTE: Recalculated event %p D=%d -> %d\n",
					   this, delta, delta - new_delta);
			delta -= new_delta;
		}
		bool operator<(const ktimer_event_t& r) const { return delta < r.delta; }
	};
	static bitops::bitmap_table_t<ktimer_event_t,CONFIG_MAX_KT_EVENTS> ktimer_table;
	void * ktimer_event_t::operator new (size_t size){ return ktimer_table.alloc(size); }
	void ktimer_event_t::operator delete (void * mem){  ktimer_table.free(mem); }
	//using ktimer_event_queue_t = tailq::prio_head<ktimer_event_t,&ktimer_event_t::link>;
	using ktimer_event_queue_t = tailq::head<ktimer_event_t,&ktimer_event_t::link>;
	ktimer_event_queue_t event_queue;

};

extern uint32_t SystemCoreClock;
namespace mimx {


	static uint64_t ktimer_now=0;
	static bool ktimer_enabled = 0;
	static uint32_t ktimer_delta = 0;
	static long long ktimer_time = 0;


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
	void ktimer_init(void)
	{
		//init_systick(CONFIG_KTIMER_HEARTBEAT, 0);
		init_systick(SystemCoreClock / 1000U, 0);

	}
	typedef union {
		uint16_t raw;
		struct {
			uint32_t	m : 10;
			uint32_t	e : 5;
			uint32_t	a : 1;
		} period;
		struct {
			uint32_t	m : 10;
			uint32_t	c : 1;
			uint32_t	e : 4;
			uint32_t	a : 1;
		} point;
	} ipc_time_t;
	uint32_t mills_to_ticks(uint32_t timeout) {
		//return (SystemCoreClock / 1000U) * timeout;
		return timeout;
	}


	static void ktimer_disable(void)
	{
		if (ktimer_enabled) ktimer_enabled = false;
	}

	static void ktimer_enable(uint32_t delta)
	{
		if (!ktimer_enabled) {
			ktimer_delta = delta;
			ktimer_time = 0;
			ktimer_enabled = true;
	#if defined(CONFIG_KDB) && \
	    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
			tickless_verify_start(ktimer_now, ktimer_delta);
	#endif	/* CONFIG_KDB */
		}
	}
	bool  ktimer_event_schedule(uint32_t ticks, ktimer_event_t *kte){
		long etime = 0, delta = 0;
		assert(ticks < 0xFFFFFF);
		if (!ticks) return false;
		ticks -= ktimer_time;
		irq_simple_lock lock;
		if(event_queue.empty()) {
			kte->delta = ticks;
			event_queue.push_front(kte);

			printk("KTE: Scheduled event %p  with "
					   "T=%d\n",
					   kte,   ticks);
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
				if (ticks >= next_event->delta) {
					decltype(next_event) event;
					do {
						event = next_event;
						etime += event->delta;
						delta = ticks - etime;
						++next_event;
					} while (next_event != event_queue.end() &&
							 ticks > (etime + next_event->delta));

					printk("KTE: Scheduled event %p [%p:%p] with "
							   "D=%d and T=%d\n",
							   kte, event, next_event, delta, ticks);

					/* Insert into chain and recalculate */
					event_queue.insert(event, kte);
				} else {
					/* Event should be scheduled before earlier event */
					printk("KTE: Scheduled early event %p with T=%d\n",
							   kte, ticks);
					event_queue.push_front(kte);
					delta = ticks;

					/* Reset timer */
					ktimer_enable(ticks);
				}
				/* Chaining events */
				if (delta < (int)CONFIG_KTIMER_MINTICKS) delta = 0;

				kte->delta = delta;
				next_event->recalc(delta);
		}

		return true;
	}
	ktimer_t ktimer_event_create(uint32_t ticks, ktimer_callback handler, void *data);
	void ktimer_cancel(ktimer_t handle){
		assert(0);
	}
	ktimer_t ktimer_event_create(uint32_t ticks, ktimer_callback handler, void *data)
	{
		ktimer_event_t *kte = new ktimer_event_t(handler,data);
		assert(kte);
		if (!handler) return 0;

		if (!ktimer_event_schedule(ticks, kte)) {
			assert(0);
			delete kte;
			return 0;
		}
		return kte->handle;
	}


	void ktimer_event_handler()
	{
		assert(0);
		if(event_queue.empty()) {
			ktimer_disable();
			printk("empty event quueue but the ktimer handle was still called!\n");
			return; // no events
		}
		while(!event_queue.empty() && event_queue.front().delta == 0) {
			auto* event = &event_queue.front();
			event_queue.pop_front();
			auto ret = event->callback(event->data);
			if (ret != 0x0)
				ktimer_event_schedule(ret, event); // reschedule
			else {
				printk("event deleted %d!\n", ktimer_now);
				delete event;
			}

		}
		if (!event_queue.empty()) {
			/* Reset ktimer */
			ktimer_enable(event_queue.front().delta);
		}
	}


	bool __ktimer_handler(void)
	{
		++ktimer_now;

		if (ktimer_enabled && ktimer_delta > 0) {

			++ktimer_time;
			--ktimer_delta;

			if (ktimer_delta == 0) {
				ktimer_enabled = false;
				ktimer_time = ktimer_delta = 0;
				return true;
	#if defined(CONFIG_KDB) && \
	    defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
				tickless_verify_stop(ktimer_now);
	#endif	/* CONFIG_KDB */
				// softirq_schedule(KTE_SOFTIRQ);
				//ktimer_event_handler(); // fuckit, we will do it live!
			}
		}
		return false;
	}



#if 0

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
#endif

};
static f9::context timer_context;

void handle_clock() {
	mimx::ktimer_event_handler(); // fuckit, we will do it live!
	timer_context.hard_restore();
}
 extern "C" __attribute__((naked))  void SysTick_Handler() ;
 extern "C" __attribute__((naked))  void SysTick_Handler() {
	timer_context.save();
	__asm__ __volatile__ ("push {lr}");
	if(mimx::__ktimer_handler()) {
		__asm__ __volatile__ ("pop {lr}");
		timer_context.save();
		timer_context.push_call(handle_clock);
		timer_context.restore();
		__asm__ __volatile__ ("bx lr");
	}
	__asm__ __volatile__ ("pop {lr}");
	__asm__ __volatile__ ("bx lr");
	//mimx::request_schedule();
}
