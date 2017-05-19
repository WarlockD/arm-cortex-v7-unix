/*
 * softirq.hpp
 *
 *  Created on: May 18, 2017
 *      Author: Paul
 */

#ifndef F9_SOFTIRQ_HPP_
#define F9_SOFTIRQ_HPP_

#include "types.hpp"
#include "thread.hpp"

namespace f9 {
	 enum class SOFTIRQ {
		KTE,		/* Kernel timer event */
		ASYNC,		/* Asynchronius event */
		SYSCALL,
	#ifdef CONFIG_KDB
		KDB,		/* KDB should have least priority */
	#endif
		NR
	} ;
	 // this should be delcared static inside the files they represent
	 class softirq_t {
	 protected:
		 virtual void run() = 0;
		 bool 		  _schedule;
		 static bool _execute_loop();
		 softirq_t(SOFTIRQ type);
		 virtual ~softirq_t() { assert(0); }
	 public:
		 void schedule();
	 	static bool execute();
	 } ;
	 void softirq_schedule(SOFTIRQ type);
} /* namespace f9 */

#endif /* F9_SOFTIRQ_HPP_ */
