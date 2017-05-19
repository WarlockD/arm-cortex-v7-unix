/*
 * syscall.cpp
 *
 *  Created on: May 18, 2017
 *      Author: Paul
 */

#include "syscall.hpp"
#include "thread.hpp"
#include "fpage.hpp"
#include "schd.hpp"
#include "softirq.hpp"
#include "ipc.hpp"
#include "irq.hpp"

namespace f9 {
	tcb_t* caller;
	static void sys_thread_control(uint32_t *param1, uint32_t *param2)
	{
		thread_id_t dest = param1[REG_R0];
		thread_id_t space = param1[REG_R1];
		thread_id_t pager = param1[REG_R3];

		if (space != NILTHREAD) {
			memptr_t utcbi = param2[0];
			/* Creation of thread */
			utcb_t *utcb = reinterpret_cast<utcb_t*>(utcbi);	/* R4 */
#ifdef CONFIG_ENABLE_FPAGE
			mempool_t *utcb_pool = mempool_t::getbyid(mempool_t::search(utcbi, UTCB_SIZE));

			if (!utcb_pool || !(utcb_pool->flags % (MP::UR | MP::UW))) {
				/* Incorrect UTCB relocation */
				return;
			}
#endif

			tcb_t *thr = tcb_t::create(dest, utcb);
			thr->space(space, utcb);
			thr->utcb->t_pager = pager;
			param1[REG_R0] = 1;
		} else {
			/* Removal of thread */
			tcb_t *thr = thread_by_globalid(dest);
			thr->free_space();
			delete thr;
			//thread_free_space(thr);
			//thread_destroy(thr);
		}
	}
	class syscall_handler_t : public softirq_t {
		void run() final {
				uint32_t *svc_param1 = reinterpret_cast<uint32_t*>(caller->ctx.sp);
				//uint32_t svc_num = (reinterpret_cast<uint8_t *>(svc_param1[REG_PC]))[-2];
				SYS svc_num = static_cast<SYS>((reinterpret_cast<uint8_t *>(svc_param1[REG_PC]))[-2]);
				uint32_t *svc_param2 = caller->ctx.regs;

				switch(svc_num){
				case SYS::THREAD_CONTROL:
					sys_thread_control(svc_param1, svc_param2);
					caller->state = TSTATE::RUNNABLE;
					break;
				case SYS::IPC:
					f9::sys_ipc(svc_param1);
					break;
				default:
		#ifdef CONFIG_DEBUG
					dbg::print(dbg::DL_SYSCALL,
							   "SVC: %d called [%d, %d, %d, %d]\n", svc_num,
							   svc_param1[REG_R0], svc_param1[REG_R1],
							   svc_param1[REG_R2], svc_param1[REG_R3]);
					caller->state = TSTATE::RUNNABLE;
		#endif
					break;
			}
		}
		public:
			syscall_handler_t() : softirq_t(SOFTIRQ::SYSCALL) {}
	};
	syscall_handler_t syscall_softirq;


	void __svc_handler(void)
	{
		extern tcb_t *kernel;

		/* Kernel requests context switch, satisfy it */
		if (tcb_t::current() == kernel)
			return;

		caller = (tcb_t*)tcb_t::current();
		caller->state = TSTATE::SVC_BLOCKED;
		syscall_softirq.schedule();
		//softirq_schedule(SOFTIRQ::SYSCALL);
	}


} /* namespace f9 */
IRQ_HANDLER(SVC_Handler, f9::__svc_handler);
