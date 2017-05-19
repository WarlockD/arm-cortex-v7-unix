/*
 * syscall.hpp
 *
 *  Created on: May 18, 2017
 *      Author: Paul
 */

#ifndef F9_SYSCALL_HPP_
#define F9_SYSCALL_HPP_

namespace f9 {
	 enum class SYS {
		KERNEL_INTERFACE,		/* Not used, KIP is mapped */
		EXCHANGE_REGISTERS,
		THREAD_CONTROL,
		SYSTEM_CLOCK,
		THREAD_SWITCH,
		SCHEDULE,
		IPC,
		LIPC,
		UNMAP,
		SPACE_CONTROL,
		PROCESSOR_CONTROL,
		MEMORY_CONTROL,
	} ;

} /* namespace f9 */

#endif /* F9_SYSCALL_HPP_ */
