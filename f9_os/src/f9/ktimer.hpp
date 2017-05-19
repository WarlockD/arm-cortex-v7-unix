/*
 * ktimer.hpp
 *
 *  Created on: May 19, 2017
 *      Author: Paul
 */

#ifndef F9_KTIMER_HPP_
#define F9_KTIMER_HPP_

#include "types.hpp"
#include <sys\time.h>

namespace f9 {
	typedef uint32_t (*ktimer_event_handler_t)(void *data);


	struct ktimer_event_t {
		ktimer_event_t *next;
		ktimer_event_handler_t handler;

		clock_t delta;
		void *data;


		static	void operator delete (void * mem);

		bool schedule(clock_t ticks);
		static ktimer_event_t* create(clock_t ticks, ktimer_event_handler_t handler,void *data);
		~ktimer_event_t();
	private:
		void event_recalc(clock_t new_delta);
		ktimer_event_t(ktimer_event_handler_t handler,void *data)
			: next(nullptr), handler(handler), delta(0),data(data) {}
		static void * operator new (size_t size);
	} ;

	void ktimer_event_init(void);

#ifdef CONFIG_KTIMER_TICKLESS
void ktimer_enter_tickless(void);
#endif /* CONFIG_KTIMER_TICKLESS */

} /* namespace f9 */

#endif /* F9_KTIMER_HPP_ */
