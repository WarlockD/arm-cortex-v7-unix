/*
 * ipc.cpp
 *
 *  Created on: May 18, 2017
 *      Author: Paul
 */

#include "ipc.hpp"
#include "thread.hpp"
#include "schd.hpp"
#include "fpage.hpp"
#include "ktimer.hpp"

namespace f9 {
	extern tcb_t *caller;

	/* Imports from thread.c */
	extern tcb_t *thread_map[];
	extern int thread_count;

	static void user_ipc_error(tcb_t *thr, UE error)
	{
		/* Set ipc unsuccessful */
		ipc_msg_tag_t tag(thr->ipc_read(0));
		tag.flags(tag.flags() | 0x8);

		thr->ipc_write(0, tag.raw());
		set_user_error(thr, error);
	}
	static inline void do_ipc_error(tcb_t *from, tcb_t *to, UE from_err,UE to_err,TSTATE from_state,TSTATE to_state)
	{
		user_ipc_error(from, from_err);
		user_ipc_error(to, to_err);
		from->state = from_state;
		to->state = to_state;
	}

	static void do_ipc(tcb_t *from, tcb_t *to)
	{
		ipc_typed_item_t	typed_item;
		int untyped_idx, typed_idx, typed_item_idx;
		uint32_t typed_data;		/* typed item extra word */
		thread_id_t from_recv_tid;

		/* Clear timeout event when ipc is established. */
		from->timeout_event = 0;
		to->timeout_event = 0;

		/* Copy tag of message */
		ipc_msg_tag_t tag( from->ipc_read(0));
		int untyped_last = tag.untyped() + 1;
		int typed_last = untyped_last + tag.typed();

		if (typed_last > IPC_MR_COUNT) {
			do_ipc_error(from, to,
			             UE::IPC_MSG_OVERFLOW | UE::IPC_PHASE_SEND,
			             UE::IPC_MSG_OVERFLOW | UE::IPC_PHASE_RECV,
			             TSTATE::RUNNABLE,
						 TSTATE::RUNNABLE);
			return;
		}
		to->ipc_write(0, tag.raw());

		/* Copy untyped words */
		for (untyped_idx = 1; untyped_idx < untyped_last; ++untyped_idx) {
			to->ipc_write(untyped_idx, from->ipc_read(untyped_idx));
		}

		typed_item_idx = -1;
		/* Copy typed words
		 * FSM: j - number of byte */
		for (typed_idx = untyped_idx; typed_idx < typed_last; ++typed_idx) {
			uint32_t mr_data = from->ipc_read(typed_idx);

			/* Write typed mr data to 'to' thread */
			to->ipc_write(typed_idx, mr_data);

			if (typed_item_idx == -1) {
				/* If typed_item_idx == -1 - read typed item's tag */
				typed_item = ipc_typed_item_t(mr_data);
				++typed_item_idx;
			} else if (typed_item.header() & IPC_TI_MAP_GRANT) {
#ifdef CONFIG_ENABLE_FPAGE
				/* MapItem / GrantItem have 1xxx in header */
				int ret;
				typed_data = mr_data;

				ret = map_area(from->as, to->as,
				               typed_item.raw() & 0xFFFFFFC0,
				               typed_data & 0xFFFFFFC0,
				               (typed_item.header() & IPC_TI_GRANT) ?
				                   GRANT : MAP,
								   from->ispriviliged());
				typed_item_idx = -1;

				if (ret < 0) {
					do_ipc_error(from, to,
					             UE::IPC_ABORTED | UE::IPC_PHASE_SEND,
					             UE::IPC_ABORTED | UE::IPC_PHASE_RECV,
					             TSTATE::RUNNABLE,
								 TSTATE::RUNNABLE);
					return;
				}
#else
				panic("map grant not supported!\r\n");
#endif
			}

			/* TODO: StringItem support */
		}

		if (!to->ctx.sp || !from->ctx.sp) {
			do_ipc_error(from, to,
			             UE::IPC_ABORTED | UE::IPC_PHASE_SEND,
			             UE::IPC_ABORTED | UE::IPC_PHASE_RECV,
						 TSTATE::RUNNABLE,
						 TSTATE::RUNNABLE);
			return;
		}

		to->utcb->sender = from->t_globalid;

		to->state = TSTATE::RUNNABLE;
		to->ipc_from = NILTHREAD;
		((uint32_t *) to->ctx.sp)[REG_R0] = from->t_globalid;

		/* If from has receive phases, lock myself */
		from_recv_tid = ((uint32_t *) from->ctx.sp)[REG_R1];
		if (from_recv_tid == NILTHREAD) {
			from->state = TSTATE::RUNNABLE;
		} else {
			from->state = TSTATE::RECV_BLOCKED;
			from->ipc_from = from_recv_tid;

			dbg::print(dbg::DL_IPC, "IPC: %t receiving\n", from->t_globalid);
		}

		/* Dispatch communicating threads */
		sched_slot_dispatch(SSI::NORMAL_THREAD, from);
		sched_slot_dispatch(SSI::IPC_THREAD, to);

		dbg::print(dbg::DL_IPC, "IPC: %t to %t\n", caller->t_globalid, to->t_globalid);
	}


	uint32_t ipc_timeout(void *data)
	{
		ktimer_event_t *event = (ktimer_event_t *) data;
		tcb_t *thr = (tcb_t *) event->data;

		if (thr->timeout_event == (uint32_t)data) {

			if (thr->state == TSTATE::RECV_BLOCKED)
				user_ipc_error(thr, UE::IPC_TIMEOUT | UE::IPC_PHASE_RECV);

			if (thr->state ==  TSTATE::SEND_BLOCKED)
				user_ipc_error(thr, UE::IPC_TIMEOUT | UE::IPC_PHASE_SEND);

			thr->state = TSTATE::RUNNABLE;
			thr->timeout_event = 0;
		}

		return 0;
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
	static void sys_ipc_timeout(uint32_t timeout)
	{
		ipc_time_t t;
		t.raw = timeout ;

		/* millisec to ticks */
		uint32_t ticks = (t.period.m << t.period.e) /
		                 ((1000000) / (HAL_RCC_GetHCLKFreq()/CONFIG_KTIMER_HEARTBEAT));

		ktimer_event_t *kevent = ktimer_event_t::create(ticks, ipc_timeout, caller);

		caller->timeout_event = (uint32_t) kevent;
	}


	void sys_ipc(uint32_t *param1)
	{
		/* TODO: Checking of recv-mask */
		tcb_t *to_thr = NULL;
		thread_id_t to_tid = param1[REG_R0], from_tid = param1[REG_R1];
		uint32_t timeout = param1[REG_R2];

		if (to_tid == NILTHREAD &&
			from_tid == NILTHREAD) {
			caller->state = TSTATE::INACTIVE;
			if (timeout)
				sys_ipc_timeout(timeout);
			return;
		}

		if (to_tid != NILTHREAD) {
			to_thr = thread_by_globalid(to_tid);

			if (to_tid == TID_TO_GLOBALID(THREAD::LOG)) {
				user_log(caller);
				caller->state = TSTATE::RUNNABLE;
				return;
			} else if (to_tid == TID_TO_GLOBALID(THREAD::IRQ_REQUEST)) {
#if 0
				user_interrupt_config(caller);

				caller->state = TSTATE::RUNNABLE;
#else
				assert(0); // no user interrupts yet, oooh signals this make sence humm
#endif
				return;
			} else if ((to_thr && to_thr->state == TSTATE::RECV_BLOCKED)
			           || to_tid == caller->t_globalid) {
				/* To thread who is waiting for us or sends to myself */
				do_ipc(caller, to_thr);
				return;
			} else if (to_thr && to_thr->state == TSTATE::INACTIVE &&
			           GLOBALID_TO_TID(to_thr->utcb->t_pager) ==
			           GLOBALID_TO_TID(caller->t_globalid)) {
				if (caller->ipc_read( 0) == 0x00000005) {
					/* mr1: thread func, mr2: stack addr,
					 * mr3: stack size
				         * mr4: thread entry, mr5: thread args
				         * thread start protocol */

					memptr_t sp = caller->ipc_read( 2);
					size_t stack_size = caller->ipc_read(3);
					uint32_t regs[4];	/* r0, r1, r2, r3 */

					dbg::print(dbg::DL_IPC,
					           "IPC: %t thread start\n", to_tid);

					to_thr->stack_base = sp - stack_size;
					to_thr->stack_size = stack_size;
#if 0
					regs[REG_R0] = (uint32_t)&kip;
#else
					regs[REG_R0] = 0;
#endif
					regs[REG_R1] = (uint32_t)to_thr->utcb;
					regs[REG_R2] = caller->ipc_read(4);
					regs[REG_R3] = caller->ipc_read( 5);
					to_thr->init_ctx((void *) sp,
					                (void *) caller->ipc_read(1),
					                regs);

					caller->state = TSTATE::RUNNABLE;

					/* Start thread */
					to_thr->state = TSTATE::RUNNABLE;

					return;
				} else {
					do_ipc(caller, to_thr);
					to_thr->state = TSTATE::INACTIVE;

					return;
				}
			} else  {
				/* No waiting, block myself */
				caller->state = TSTATE::SEND_BLOCKED;
				caller->utcb->intended_receiver = to_tid;
				dbg::print(dbg::DL_IPC,
				           "IPC: %t sending\n", caller->t_globalid);

				if (timeout)
					sys_ipc_timeout(timeout);

				return;
			}
		}

		if (from_tid != NILTHREAD) {
			tcb_t *thr = nullptr;

			if (from_tid == ANYTHREAD) {
				/* Find out if there is any sending thread waiting
				 * for caller
				 */
				for (int i = 1; i < thread_count; ++i) {
					thr = thread_map[i];
					if (thr->state == TSTATE::SEND_BLOCKED &&
					    thr->utcb->intended_receiver ==
					    caller->t_globalid) {
						do_ipc(thr, caller);
						return;
					}
				}
			} else if (from_tid != TID_TO_GLOBALID(THREAD::INTERRUPT)) {
				thr = thread_by_globalid(from_tid);

				if (thr->state == TSTATE::SEND_BLOCKED &&
				    thr->utcb->intended_receiver ==
				    caller->t_globalid) {
					do_ipc(thr, caller);
					return;
				}
			}

			/* Only receive phases, simply lock myself */
			caller->state = TSTATE::RECV_BLOCKED;
			caller->ipc_from = from_tid;

			if (from_tid == TID_TO_GLOBALID(THREAD::INTERRUPT)) {
				/* Threaded interrupt is ready */
#if 0
				user_interrupt_handler_update(caller);
#else
				assert(0); // no user interrupts yet, oooh signals this make sence humm
#endif
			}

			if (timeout)
				sys_ipc_timeout(timeout);

			dbg::print(dbg::DL_IPC, "IPC: %t receiving\n", caller->t_globalid);

			return;
		}

		caller->state = TSTATE::SEND_BLOCKED;
	}

	uint32_t ipc_deliver(void *data)
	{
		thread_id_t receiver;
		tcb_t *from_thr = NULL, *to_thr = NULL;

		for (int i = 1; i < thread_count; ++i) {
			tcb_t *thr = thread_map[i];
			switch (thr->state) {
			case TSTATE::RECV_BLOCKED:
				if (thr->ipc_from != NILTHREAD &&
					thr->ipc_from != ANYTHREAD &&
					thr->ipc_from != TID_TO_GLOBALID(THREAD::INTERRUPT)) {
					from_thr = thread_by_globalid(thr->ipc_from);
					/* NOTE: Must check from_thr intend to send*/
					if (from_thr->state == TSTATE::SEND_BLOCKED &&
					    from_thr->utcb->intended_receiver == thr->t_globalid)
						do_ipc(from_thr, thr);
				}
				break;
			case TSTATE::SEND_BLOCKED:
				receiver = thr->utcb->intended_receiver;
				if (receiver != NILTHREAD &&
				    receiver != ANYTHREAD) {
					to_thr = thread_by_globalid(receiver);
					if (to_thr->state == TSTATE::RECV_BLOCKED)
						do_ipc(thr, to_thr);
				}
				break;
			default:
				break;
			}
		}

		return 4096;
	}


} /* namespace f9 */
