/*
 * clock.hpp
 *
 *  Created on: May 26, 2017
 *      Author: Paul
 */


#ifndef MIMIX_CPP_CLOCK_HPP_
#define MIMIX_CPP_CLOCK_HPP_


#include "proc.hpp"

namespace mimx {

#if	  defined(CONFIG_KTIMER_TICKLESS) && defined(CONFIG_KTIMER_TICKLESS_VERIFY)
	class tickless_verify {
	public:
		void init(void);
		void start(uint32_t ktimer_now, uint32_t need);
		void stop(uint32_t ktimer_now);
		void count(void);
		void count_int(void);
		int32_t stat(int *times);
	};
#else
	static constexpr bool ktimer_tickless_enable = false;
	class tickless_verify {
	public:
		void init(void){}
		void start(uint32_t ktimer_now, uint32_t need){ (void)ktimer_now; (void)need; }
		void stop(uint32_t ktimer_now){}
		void count(void){}
		void count_int(void){}
		int32_t stat(int *times){ (void)times; return 0;}
	};
#endif

	class sysclock : public softirq {
		uint32_t _ticks;
		void handler()  final;
	public:
		sysclock();
	};

} /* namespace f9 */

#endif /* MIMIX_CPP_CLOCK_HPP_ */
