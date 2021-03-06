/*
 * ktimer.hpp
 *
 *  Created on: Apr 30, 2017
 *      Author: warlo
 */

#ifndef OS_KTIMER_HPP_
#define OS_KTIMER_HPP_

#include "list.hpp"

namespace os {
	static constexpr bool CONFIG_KTIMER_TICKLESS=true;
	static constexpr bool CONFIG_KTIMER_TICKLESS_VERIFY=false;
	static constexpr size_t CONFIG_KTIMER_TICKLESS_COMPENSATION=0;
	static constexpr size_t CONFIG_KTIMER_TICKLESS_INT_COMPENSATION=0;
	static constexpr size_t CONFIG_KTIMER_HEARTBEAT=65536;
	static constexpr size_t CONFIG_KTIMER_MINTICKS=128;

void ktimer_handler(void);

/* Returns 0 if successfully handled
 * or number ticks if need to be rescheduled
 */
struct ktimer_event_t;
typedef uint32_t (*ktimer_event_handler_t)(ktimer_event_t& data);


struct ktimer_event_t {
	list::entry<ktimer_event_t> link;
	ktimer_event_handler_t handler;
	uint32_t delta;
	void *data;
} ;



void ktimer_event_init(void);

int ktimer_event_schedule(uint32_t ticks, ktimer_event_t *kte);
ktimer_event_t *ktimer_event_create(uint32_t ticks,
                                    ktimer_event_handler_t handler,
                                    void *data);
void ktimer_event_handler(void);

#ifdef CONFIG_KTIMER_TICKLESS
void ktimer_enter_tickless(void);
#endif /* CONFIG_KTIMER_TICKLESS */
class ktimer {
};

} /* namespace os */

#endif /* OS_KTIMER_HPP_ */
