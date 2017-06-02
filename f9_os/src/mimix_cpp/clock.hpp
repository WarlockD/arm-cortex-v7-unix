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
	typedef uint32_t (*ktimer_callback)(void* arg);

	using ktimer_t = uint32_t; // handle

	void ktimer_init();
	ktimer_t ktimer_event_create(uint32_t ticks, ktimer_callback handler, void *data);
	uint32_t mills_to_ticks(uint32_t mills) ;
	namespace ktimer {
	template<typename CALLBACK, typename DATA>
		inline ktimer_t create_event(uint32_t ticks, CALLBACK callback, DATA data) {
			return ktimer_event_create(ticks,reinterpret_cast<ktimer_callback>(callback),reinterpret_cast<void*>(data));
		}
	};

	void ktimer_cancel(ktimer_t handle);

} /* namespace f9 */

#endif /* MIMIX_CPP_CLOCK_HPP_ */
