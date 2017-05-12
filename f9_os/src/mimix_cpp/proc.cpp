/*
 * proc.cpp
 *
 *  Created on: May 11, 2017
 *      Author: Paul
 */

#include "proc.hpp"

namespace mimx {
	void tq_list::run_task_queue(void * f){
		tq_list* l = reinterpret_cast<tq_list*>(f);
		l->run();
	}
	 tq_list tq_list::tq_immediate;
	 tq_list tq_list::tq_scheduler;
	 tq_list tq_list::tq_disk;

	 proc proc::procs[NR_TASKS + NR_PROCS];	/* process table */
	 proc * proc::pproc_addr[NR_TASKS + NR_PROCS];
	 proc *proc::rdy_head[NR_SCHED_QUEUES]; /* ptrs to ready list headers */
	 proc *proc::rdy_tail[NR_SCHED_QUEUES]; /* ptrs to ready list tails */
proc::proc() {
	// TODO Auto-generated constructor stub

}

proc::~proc() {
	// TODO Auto-generated destructor stub
}
int proc::mini_send(int dst,message* m_ptr, uint32_t flags){

}
int proc::mini_receive(int src, message *m_ptr, uint32_t flags){

}
int proc::mini_notify(int dst){

}
void proc::enqueue(){

}
void proc::dequeue(){

}
void proc::sched(int *queue, int *front) {

}
void proc::pick_proc(void){

}
} /* namespace mimx */
