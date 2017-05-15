/*
 * proc.cpp
 *
 *  Created on: May 11, 2017
 *      Author: Paul
 */

#include "proc.hpp"
#include <os\atomic.h>

#define OK 0
/* The following error codes are generated by the kernel itself. */
#define E_BAD_DEST        -1	/* destination address illegal */
#define E_BAD_SRC         -2	/* source address illegal */
#define E_TRY_AGAIN       -3	/* can't send-- tables full */
#define E_OVERRUN         -4	/* interrupt for task that is not waiting */
#define E_BAD_BUF         -5	/* message buf outside caller's addr space */
#define E_TASK            -6	/* can't send to task */
#define E_NO_MESSAGE      -7	/* RECEIVE failed: no message present */
#define E_NO_PERM         -8	/* ordinary users can't send to tasks */
#define E_BAD_FCN         -9	/* only valid fcns are SEND, RECEIVE, BOTH */
#define E_BAD_ADDR       -10	/* bad address given to utility routine */
#define E_BAD_PROC       -11	/* bad proc number given to utility */
#if 0

// notes for svc handler
void __attribute__ (( naked )) sv_call_handler(void)
{
    asm volatile(
      "movs r0, #4\t\n"
      "mov  r1, lr\t\n"
      "tst  r0, r1\t\n" /* Check EXC_RETURN[2] */
      "beq 1f\t\n"
      "mrs r0, psp\t\n"
      "ldr r1,=sv_call_handler_c\t\n"
      "bx r1\t\n"
      "1:mrs r0,msp\t\n"
      "ldr r1,=sv_call_handler_c\t\n"
      : /* no output */
      : /* no input */
      : "r0" /* clobber */
  );
}
sv_call_handler_c(unsigned int * hardfault_args)
{
    unsigned int stacked_r0;
    unsigned int stacked_r1;
    unsigned int stacked_r2;
    unsigned int stacked_r3;
    unsigned int stacked_r12;
    unsigned int stacked_lr;
    unsigned int stacked_pc;
    unsigned int stacked_psr;
    unsigned int svc_parameter;

    //Exception stack frame
    stacked_r0 = ((unsigned long) hardfault_args[0]);
    stacked_r1 = ((unsigned long) hardfault_args[1]);
    stacked_r2 = ((unsigned long) hardfault_args[2]);
    stacked_r3 = ((unsigned long) hardfault_args[3]);

    stacked_r12 = ((unsigned long) hardfault_args[4]);
    stacked_lr  = ((unsigned long) hardfault_args[5]);
    stacked_pc  = ((unsigned long) hardfault_args[6]);
    stacked_psr = ((unsigned long) hardfault_args[7]);

    svc_parameter = ((char *)stacked_pc)[-2]; /* would be LSB of PC is 1. */

    switch(svc_parameter){
    // each procesure call for the parameter
    }
}
#endif

namespace mimx {

	/* Global variables used in the kernel. */
	static constexpr pid_t IDLE     =       -999;	/* 'cur_proc' = IDLE means nobody is running */
	/* Clocks and timers */
	time_t realtime;	/* real time clock */
	time_t lost_ticks;		/* incremented when clock int can't send mess*/

	/* Processes, signals, and messages. */
	pid_t cur_proc;		/* current process */
	pid_t prev_proc;		/* previous process */
	size_t sig_procs;		/* number of procs with p_pending != 0 */
	message int_mess;	/* interrupt routines build message here */


	/* The kernel and task stacks. */
	struct t_stack {
	  int stk[1024/sizeof(int)];
	} t_stack[NR_TASKS - 1];	/* task stacks; task = -1 never really runs */

	char k_stack[1024];	/* The kernel stack. */

	proc *proc::proc_ptr=nullptr;	/* &proc[cur_proc] */
	proc *proc::bill_ptr=nullptr;	/* ptr to process to bill for clock ticks */
	proc *proc::rdy_head[NQ];	/* pointers to ready list headers */
	proc *proc::rdy_tail[NQ];	/* pointers to ready list tails */
	bitops::bitmap_t<NR_TASKS+1> busy_map;		/* bit map of busy tasks */
	message *proc::task_mess[NR_TASKS+1];	/* ptrs to messages for busy tasks */
  	//constexpr static inline proc* proc_addr(pid_t n) { return &proc::procs[NR_TASKS + n]; }
	/*===========================================================================*
	 *				mini_send				     *
	 *===========================================================================*/
	int proc::mini_send(pid_t caller, pid_t dest, message * m_ptr)
	//int caller;			/* who is trying to send a message? */
	//int dest;			/* to whom is message being sent? */
	//m_ptr;			/* pointer to message buffer */
	{
	/* Send a message from 'caller' to 'dest'.  If 'dest' is blocked waiting for
	 * this message, copy the message to it and unblock 'dest'.  If 'dest' is not
	 * waiting at all, or is waiting for another source, queue 'caller'.
	 */
	  proc *caller_ptr, *dest_ptr, *next_ptr;
	  /* User processes are only allowed to send to FS and MM.  Check for this. */
	  if (caller >= LOW_USER && (dest != FS_PROC_NR && dest != MM_PROC_NR))
		return(E_BAD_DEST);
	  caller_ptr = proc_addr(caller);	/* pointer to source's proc entry */
	  dest_ptr = proc_addr(dest);	/* pointer to destination's proc entry */
	  if (dest_ptr->p_flags & P_SLOT_FREE) return(E_BAD_DEST);	/* dead dest */

#if 0
	  vir_bytes vb;			/* message buffer pointer as vir_bytes */
	  vir_clicks vlo, vhi;		/* virtual clicks containing message to send */
	  vir_clicks len;		/* length of data segment in clicks */
	  /* Check for messages wrapping around top of memory or outside data seg. */
	  len = caller_ptr->p_map[D].mem_len;
	  vb = (vir_bytes) m_ptr;
	  vlo = vb >> CLICK_SHIFT;	/* vir click for bottom of message */
	  vhi = (vb + MESS_SIZE - 1) >> CLICK_SHIFT;	/* vir click for top of message */
	  if (vhi < vlo || vhi - caller_ptr->p_map[D].mem_vir >= len)return(E_BAD_ADDR);
#endif
	  /* Check to see if 'dest' is blocked waiting for this message. */
	  if ( (dest_ptr->p_flags & RECEIVING) &&
			(dest_ptr->p_getfrom == ANY || dest_ptr->p_getfrom == caller) ) {
		/* Destination is indeed waiting for this message. */
		  //dest_ptr->p_mess = m_ptr->copy(caller);
		  assert(dest_ptr->p_messbuf);
		  *dest_ptr->p_messbuf = m_ptr->copy(caller); //&dest_ptr->p_mess;
		//cp_mess(caller, caller_ptr->p_map[D].mem_phys, m_ptr, dest_ptr->p_map[D].mem_phys, dest_ptr->p_messbuf);
		//dest_ptr->p_flags &= ~RECEIVING;	/* deblock destination */
		  dest_ptr->p_flags = static_cast<PROC_STATUS>(dest_ptr->p_flags & ~RECEIVING);
		if (dest_ptr->p_flags == 0) ready(dest_ptr);
	  } else {
		/* Destination is not waiting.  Block and queue caller. */
		if (caller == HARDWARE) return E_OVERRUN;
		caller_ptr->p_messbuf = m_ptr;
		caller_ptr->p_flags = static_cast<PROC_STATUS>(caller_ptr->p_flags | SENDING);
		//caller_ptr->p_flags |= SENDING;
		unready(caller_ptr);

		/* Process is now blocked.  Put in on the destination's queue. */
		if ( (next_ptr = dest_ptr->p_callerq) == NIL_PROC) {
			dest_ptr->p_callerq = caller_ptr;
		} else {
			while (next_ptr->p_sendlink != NIL_PROC)
				next_ptr = next_ptr->p_sendlink;
			next_ptr->p_sendlink = caller_ptr;
		}
		caller_ptr->p_sendlink = NIL_PROC;
	  }
	  return(OK);
	}
#if 0
	/*===========================================================================*
	 *				inform					     *
	 *===========================================================================*/
	void proc::inform(pid_t proc_nr)
//	int proc_nr;			/* MM_PROC_NR or FS_PROC_NR */
	{
	/* When a signal is detected by the kernel (e.g., DEL), or generated by a task
	 * (e.g. clock task for SIGALRM), cause_sig() is called to set a bit in the
	 * p_pending field of the process to signal.  Then inform() is called to see
	 * if MM is idle and can be told about it.  Whenever MM blocks, a check is
	 * made to see if 'sig_procs' is nonzero; if so, inform() is called.
	 */

	  register struct proc *rp, *mmp;

	  /* If MM is not waiting for new input, forget it. */
	  mmp = proc_addr(proc_nr);
	  if ( ((mmp->p_flags & RECEIVING) == 0) || mmp->p_getfrom != ANY) return;

	  /* MM is waiting for new input.  Find a process with pending signals. */
	  for (rp = proc_addr(0); rp < proc_addr(NR_PROCS); rp++)
		if (rp->p_pending != 0) {
			m.m_type = KSIG;
			m.PROC1 = rp - proc - NR_TASKS;
			m.SIG_MAP = rp->p_pending;
			sig_procs--;
			if (mini_send(HARDWARE, proc_nr, &m) != OK)
				panic("can't inform MM", NO_NUM);
			rp->p_pending = 0;	/* the ball is now in MM's court */
			return;
		}
	}
#endif
	/*===========================================================================*
	 *				mini_rec				     *
	 *===========================================================================*/
	int proc::mini_rec(pid_t caller, pid_t src, message* m_ptr)
	//int caller;			/* process trying to get message */
	//int src;			/* which message source is wanted (or ANY) */
	//message *m_ptr;			/* pointer to message buffer */
	{
	/* A process or task wants to get a message.  If one is already queued,
	 * acquire it and deblock the sender.  If no message from the desired source
	 * is available, block the caller.  No need to check parameters for validity.
	 * Users calls are always sendrec(), and mini_send() has checked already.
	 * Calls from the tasks, MM, and FS are trusted.
	 */

	  register struct proc *caller_ptr, *sender_ptr, *prev_ptr;
	  int sender;

	  caller_ptr = proc_addr(caller);	/* pointer to caller's proc structure */

	  /* Check to see if a message from desired source is already available. */
	  sender_ptr = caller_ptr->p_callerq;
	  while (sender_ptr != NIL_PROC) {
		sender = sender_ptr - procs - NR_TASKS;
		if (src == ANY || src == sender) {
			/* An acceptable message has been found. */
			assert(m_ptr);
			//cp_mess(sender, sender_ptr->p_map[D].mem_phys, sender_ptr->p_messbuf, caller_ptr->p_map[D].mem_phys, m_ptr);
			  *m_ptr = sender_ptr->p_messbuf->copy(caller); //&dest_ptr->p_mess;
			sender_ptr->p_flags = static_cast<PROC_STATUS>(sender_ptr->p_flags & ~SENDING);	/* deblock sender */
			if (sender_ptr->p_flags == 0) ready(sender_ptr);
			if (sender_ptr == caller_ptr->p_callerq)
				caller_ptr->p_callerq = sender_ptr->p_sendlink;
			else
				prev_ptr->p_sendlink = sender_ptr->p_sendlink;
			return(OK);
		}
		prev_ptr = sender_ptr;
		sender_ptr = sender_ptr->p_sendlink;
	  }

	  /* No suitable message is available.  Block the process trying to receive. */
	  caller_ptr->p_getfrom = src;
	  caller_ptr->p_messbuf = m_ptr;
	  caller_ptr->p_flags = static_cast<PROC_STATUS>(sender_ptr->p_flags | RECEIVING);
	  unready(caller_ptr);

	  /* If MM has just blocked and there are kernel signals pending, now is the
	   * time to tell MM about them, since it will be able to accept the message.
	   */
	  if (sig_procs > 0 && caller == MM_PROC_NR && src == ANY) inform(MM_PROC_NR);
	  return(OK);
	}



	/*===========================================================================*
	 *				pick_proc				     *
	 *===========================================================================*/
	void proc::pick_proc()
	{
	/* Decide who to run now. */

	  register int q;		/* which queue to use */

	  if (rdy_head[TASK_Q] != NIL_PROC) q = TASK_Q;
	  else if (rdy_head[SERVER_Q] != NIL_PROC) q = SERVER_Q;
	  else q = USER_Q;

	  /* Set 'cur_proc' and 'proc_ptr'. If system is idle, set 'cur_proc' to a
	   * special value (IDLE), and set 'proc_ptr' to point to an unused proc table
	   * slot, namely, that of task -1 (HARDWARE), so save() will have somewhere to
	   * deposit the registers when a interrupt occurs on an idle machine.
	   * Record previous process so that when clock tick happens, the clock task
	   * can find out who was running just before it began to run.  (While the
	   * clock task is running, 'cur_proc' = CLOCKTASK. In addition, set 'bill_ptr'
	   * to always point to the process to be billed for CPU time.
	   */
	  prev_proc = cur_proc;
	  if (rdy_head[q] != NIL_PROC) {
		/* Someone is runnable. */
		cur_proc = rdy_head[q] - procs - NR_TASKS;
		proc_ptr = rdy_head[q];
		if (cur_proc >= LOW_USER) bill_ptr = proc_ptr;
	  } else {
		/* No one is runnable. */
		cur_proc = IDLE;
		proc_ptr = proc_addr(HARDWARE);
		bill_ptr = proc_ptr;
	  }
	}

	/*===========================================================================*
	 *				ready					     *
	 *===========================================================================*/
	void proc::ready(proc *rp)
	{
	/* Add 'rp' to the end of one of the queues of runnable processes. Three
	 * queues are maintained:
	 *   TASK_Q   - (highest priority) for runnable tasks
	 *   SERVER_Q - (middle priority) for MM and FS only
	 *   USER_Q   - (lowest priority) for user processes
	 */

	  register int q;		/* TASK_Q, SERVER_Q, or USER_Q */
	  int r;
	  ARM::simple_irq_lock lock;
	 // lock();			/* disable interrupts */
	  r = (rp - procs) - NR_TASKS;	/* task or proc number */
	  q = (r < 0 ? TASK_Q : r < LOW_USER ? SERVER_Q : USER_Q);

	  /* See if the relevant queue is empty. */
	  if (rdy_head[q] == NIL_PROC)
		rdy_head[q] = rp;	/* add to empty queue */
	  else
		rdy_tail[q]->p_nextready = rp;	/* add to tail of nonempty queue */
	  rdy_tail[q] = rp;		/* new entry has no successor */
	  rp->p_nextready = NIL_PROC;
	  //restore();			/* restore interrupts to previous state */
	}


	/*===========================================================================*
	 *				unready					     *
	 *===========================================================================*/
	void proc::unready(proc* rp)
	{
	/* A process has blocked. */
		ARM::simple_irq_lock lock;
	  register struct proc *xp;
	  int r, q;

	 // lock();			/* disable interrupts */
	  r = rp - procs - NR_TASKS;
	  q = (r < 0 ? TASK_Q : r < LOW_USER ? SERVER_Q : USER_Q);
	  if ( (xp = rdy_head[q]) == NIL_PROC) return;
	  if (xp == rp) {
		/* Remove head of queue */
		rdy_head[q] = xp->p_nextready;
		pick_proc();
	  } else {
		/* Search body of queue.  A process can be made unready even if it is
		 * not running by being sent a signal that kills it.
		 */
		while (xp->p_nextready != rp)
			if ( (xp = xp->p_nextready) == NIL_PROC) return;
		xp->p_nextready = xp->p_nextready->p_nextready;
		while (xp->p_nextready != NIL_PROC) xp = xp->p_nextready;
		rdy_tail[q] = xp;
	  }
	  //restore();			/* restore interrupts to previous state */
	}


	/*===========================================================================*
	 *				sched					     *
	 *===========================================================================*/
	void proc::sched()
	{
	/* The current process has run too long.  If another low priority (user)
	 * process is runnable, put the current process on the end of the user queue,
	 * possibly promoting another user to head of the queue.
	 */

		ARM::simple_irq_lock lock;			/* disable interrupts */
	  if (rdy_head[USER_Q] == NIL_PROC) {
		//restore();		/* restore interrupts to previous state */
		return;
	  }

	  /* One or more user processes queued. */
	  rdy_tail[USER_Q]->p_nextready = rdy_head[USER_Q];
	  rdy_tail[USER_Q] = rdy_head[USER_Q];
	  rdy_head[USER_Q] = rdy_head[USER_Q]->p_nextready;
	  rdy_tail[USER_Q]->p_nextready = NIL_PROC;
	  pick_proc();
	  //restore();			/* restore interrupts to previous state */
	}

}; /* namespace mimx */
