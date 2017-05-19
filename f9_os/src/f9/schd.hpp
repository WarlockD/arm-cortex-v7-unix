#ifndef F9_SCHD_HPP_
#define F9_SCHD_HPP_

#include "thread.hpp"

namespace f9 {

	 enum class SSI {
		SOFTIRQ,			/* Kernel thread */
		INTR_THREAD,
		ROOT_THREAD,
		IPC_THREAD,
		NORMAL_THREAD,
		IDLE,
	} ;
	 static constexpr size_t NUM_SCHED_SLOTS = static_cast<uint32_t>(SSI::IDLE)+1;
	struct sched_slot_t;

	typedef tcb_t *(*sched_handler_t)(sched_slot_t *slot);

	struct sched_slot_t {
		tcb_t *ss_scheduled;
		sched_handler_t ss_handler;

	} ;

	tcb_t* schedule_select(void);
	int schedule(void);
	void sched_slot_dispatch(SSI slot_id, tcb_t *thread);
	void sched_slot_set_handler(SSI slot_id, sched_handler_t handler);



};

#endif
