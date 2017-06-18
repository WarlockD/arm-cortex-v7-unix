/*
 * cortex_loop.hpp
 *
 *  Created on: Jun 17, 2017
 *      Author: Paul
 */

#ifndef EMBXX_DEVICE_CORTEX_LOOP_HPP_
#define EMBXX_DEVICE_CORTEX_LOOP_HPP_

#include <cstdint>
#include <cstddef>


#include <embxx\util\EventLoop.h>
#include <embxx\container\StaticQueue.h>
#include <stm32f7xx.h>

namespace embxx {
	namespace stm32 {
		class InterruptLock
		{
		public:
			InterruptLock() : flags_(0) {}
			void lock()
			{
				__asm volatile("mrs %0, primask" : "=r" (flags_)); // store flags
				__asm volatile("cpsid i"); // disable interrupts
			}

			void unlock()
			{
				if ((flags_ & IntMask) == 0) {
					// Was previously enabled
					__asm volatile("cpsie i"); // enable interrupts
				}
			}
			void lockInterruptCtx()
			{
				// Nothing to do
			}

			void unlockInterruptCtx()
			{
				// Nothing to do
			}

		private:
			volatile std::uint32_t flags_;
			static const std::uint32_t IntMask = 1U << 7;
		};
	}
	class WaitCond
	{
	public:
	    template <typename TLock>
	    void wait(TLock& lock)
	    {
	        // no need to unlock (re-enable interrupts)
	        static_cast<void>(lock);
	        __asm volatile("wfi");
	    }

	    void notify()
	    {
	        // Nothing to do, pending interrupt will cause wfi
	        // to exit even with interrupts disabled
	    }
	};
}



#endif /* EMBXX_DEVICE_CORTEX_LOOP_HPP_ */
