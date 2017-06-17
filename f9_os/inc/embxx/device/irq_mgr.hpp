
#ifndef EMBXX_DEVICE_IRQ_MGR_HPP_
#define EMBXX_DEVICE_IRQ_MGR_HPP_

#include <cstdint>
#include <cstddef>

#include "cortex_loop.hpp"
#include <embxx\util\StaticFunction.h>

namespace embxx {
	namespace stm32 {
		inline
		void enable()
		{
			__asm volatile("cpsie i");
		}

		inline
		void disable()
		{
			__asm volatile("cpsid i");
		}


		template <typename THandler = embxx::util::StaticFunction<void ()> >
		class InterruptMgr
		{
		public:
		    typedef THandler HandlerFunc;
		    using IrqId = ::IRQn_Type;
#if 0
		    enum IrqId {
		        IrqId_SysTick,
				IrqId_Timer6,
				IrqId_Usart1,

		        IrqId_AuxInt,
		        IrqId_Gpio1,
		        IrqId_Gpio2,
		        IrqId_Gpio3,
		        IrqId_Gpio4,
		        IrqId_I2C,
		        IrqId_SPI,

		        IrqId_NumOfIds // Must be last
		    };
#endif

		    constexpr static  size_t IrqId_NumOfIds = 128; // caculated on startup of the interrupt manager
		    InterruptMgr();

		    template <typename TFunc>
		    void registerHandler(IrqId id, TFunc&& handler);

		    void enableInterrupt(IrqId id);

		    void disableInterrupt(IrqId id);

		    void handleInterrupt();

		private:

		    typedef std::uint32_t EntryType;

		    struct IrqInfo {
		        IrqInfo() : PreemptPriority(0), SubPriority(15) {} // lowest posable prioirty
		        uint32_t PreemptPriority;
		        uint32_t SubPriority;
		        HandlerFunc handler_;
		       // HandlerFunc enableFunc_;
		       // HandlerFunc disableFunc_;
#if 0
		        volatile EntryType* pendingPtr_;
		        EntryType pendingMask_;
		        volatile EntryType* enablePtr_;
		        volatile EntryType* disablePtr_;
		        EntryType enDisMask_;
#endif
		    };

		    typedef std::array<IrqInfo, IrqId_NumOfIds> IrqsArray;

		    IrqsArray irqs_;
		};

		// Implementation

		template <typename THandler>
		InterruptMgr<THandler>::InterruptMgr()
		{
		    {
		        auto& info = irqs_[TIM6_DAC_IRQn];
		        info.SubPriority = 1; // set the timer to very high priority, but not higher than prem or systick
		        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, info.PreemptPriority,info.SubPriority);
		        static_cast<void>(info);
		    }
		}

		template <typename THandler>
		template <typename TFunc>
		void InterruptMgr<THandler>::registerHandler(
		    IrqId id,
		    TFunc&& handler)
		{
		    GASSERT(id < IrqId_NumOfIds);
		    irqs_[id].handler_ = std::forward<TFunc>(handler);
		}

		template <typename THandler>
		void InterruptMgr<THandler>::enableInterrupt(IrqId id)
		{
		    GASSERT(id < IrqId_NumOfIds);
		    auto& info = irqs_[id];
	    	HAL_NVIC_SetPriority(id, info.PreemptPriority,info.SubPriority);
	    	HAL_NVIC_EnableIRQ(id); // global enable
		}

		template <typename THandler>
		void InterruptMgr<THandler>::disableInterrupt(IrqId id)
		{
			HAL_NVIC_DisableIRQ(id);
		}

		template <typename THandler>
		void InterruptMgr<THandler>::handleInterrupt()
		{
		 // humm
		}






	}
}





#endif
