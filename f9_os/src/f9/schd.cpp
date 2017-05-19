#include "schd.hpp"

#include <cassert>

namespace f9 {
	static sched_slot_t slots[NUM_SCHED_SLOTS];
	tcb_t *schedule_select()
	{
		/* For each scheduler slot try to dispatch thread from it */
		for (size_t slot_id = 0; slot_id < NUM_SCHED_SLOTS; ++slot_id) {
			tcb_t *scheduled = slots[slot_id].ss_scheduled;

			if (scheduled && scheduled->isrunnable()) {
				/* Found thread, try to dispatch it */
				return scheduled;
			} else if (slots[slot_id].ss_handler) {
				/* No appropriate thread found (i.e. timeslice
				 * exhausted, no softirqs in kernel),
				 * try to redispatch another thread
				 */
				scheduled = slots[slot_id].ss_handler(&slots[slot_id]);

				if (scheduled) {
					slots[slot_id].ss_scheduled = scheduled;
					return scheduled;
				}
			}
		}

		/* not reached (last slot is IDLE which is always runnable) */
		panic("Reached end of schedule()\n");
		return nullptr;
	}
	int schedule()
	{
		tcb_t *scheduled = schedule_select();
		assert(scheduled);
		scheduled->switch_to();
		return 1;
	}

	/*
	 * Dispatch thread in slot
	 */
	void sched_slot_dispatch(SSI slot_id, tcb_t *thread)
	{
		slots[static_cast<uint32_t>(slot_id)].ss_scheduled = thread;
	}

	void sched_slot_set_handler(SSI slot_id, sched_handler_t handler)
	{
		slots[static_cast<uint32_t>(slot_id)].ss_handler = handler;
	}
};
