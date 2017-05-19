/*
 * softirq.cpp
 *
 *  Created on: May 18, 2017
 *      Author: Paul
 */

#include "softirq.hpp"
#include "thread.hpp"

namespace f9 {
	constexpr static size_t SOFTIRQ_COUNT = static_cast<size_t>(SOFTIRQ::NR);
	static softirq_t* softirq[SOFTIRQ_COUNT];
	static const char *softirq_names[SOFTIRQ_COUNT] __attribute__((used)) = {
		"Kernel timer events",
		"Asynchronous events",
		"System calls",
	#ifdef CONFIG_KDB
		"KDB enters",
	#endif
	};

	void softirq_t::schedule(){ if(!_schedule) { _schedule = true; set_kernel_state(TSTATE::RUNNABLE); } }
	void softirq_schedule(SOFTIRQ type) {
		size_t pos = static_cast<size_t>(SOFTIRQ::NR);
		if(softirq[pos]) softirq[pos]->schedule();
	}
	bool softirq_t::_execute_loop(){
		bool executed = false;
#ifdef CONFIG_DEBUG
		int dbg = 0;
#endif
		for(auto s: softirq){
			if(s && s->_schedule) {
				s->run();
				executed = true;
				s->_schedule = false;
#ifdef CONFIG_DEBUG
				dbg::print(dbg::DL_SOFTIRQ,
				           "SOFTIRQ: executing %s\n", softirq_names[dbg++]);
#endif
			}
		}
		return executed;
	}
	bool softirq_t::execute(){
		bool softirq_schedule = false, executed = false;
		do {
			executed |= _execute_loop();
			/* Must ensure that no interrupt reschedule its softirq */
			ARM::irq_disable();
			softirq_schedule = false;
			for(const auto a: softirq) if(a && a->_schedule) { softirq_schedule = true; break; }
			set_kernel_state((softirq_schedule) ? TSTATE::RUNNABLE :  TSTATE::INACTIVE);
			ARM::irq_enable();
		} while(softirq_schedule);

		return executed;
	}
	softirq_t::softirq_t(SOFTIRQ type) :_schedule(false) {
		size_t pos = static_cast<size_t>(SOFTIRQ::NR);
		assert(pos < SOFTIRQ_COUNT && softirq[pos] == nullptr);
		softirq[pos] = this;
	}

	#ifdef CONFIG_KDB
	void kdb_dump_softirq(void)
	{
		for (int i = 0; i < NR_SOFTIRQ; ++i) {
			dbg_printf(DL_KDB, "%32s %s\n", softirq_names[i],
					   atomic_get(&(softirq[i].schedule)) ?
					   "scheduled" : "not scheduled");
		}
	}
	#endif
} /* namespace f9 */
