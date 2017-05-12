#ifndef PRIV_H
#define PRIV_H

/* Declaration of the system privileges structure. It defines flags, system
 * call masks, an synchronous alarm timer, I/O privileges, pending hardware
 * interrupts and notifications, and so on.
 * System processes each get their own structure with properties, whereas all
 * user processes share one structure. This setup provides a clear separation
 * between common and privileged process fields and is very space efficient.
 *
 * Changes:
 *   Jul 01, 2005	Created.  (Jorrit N. Herder)
 */

#include "types.hpp"

namespace mimx {

	struct priv {
	  proc_nr_t s_proc_nr;		/* number of associated process */
	  sys_id_t s_id;		/* index of this system structure */
	  short s_flags;		/* PREEMTIBLE, BILLABLE, etc. */

	  short s_trap_mask;		/* allowed system call traps */
	  sys_map_t s_ipc_from;		/* allowed callers to receive from */
	  sys_map_t s_ipc_to;		/* allowed destination processes */
	  long s_call_mask;		/* allowed kernel calls */

	  sys_map_t s_notify_pending;  	/* bit map with pending notifications */
	  irq_id_t s_int_pending;	/* pending hardware interrupts */
	  sigset_t s_sig_pending;	/* pending signals */

	  timer_t s_alarm_timer;	/* synchronous alarm timer */
	//  struct far_mem s_farmem[NR_REMOTE_SEGS];  /* remote memory map */
	  reg_t *s_stack_guard;		/* stack guard word for kernel tasks */
	};



};




#endif
