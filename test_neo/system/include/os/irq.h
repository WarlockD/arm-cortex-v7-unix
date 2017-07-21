#ifndef INCLUDE_OS_ISR_MEM_H_
#define INCLUDE_OS_ISR_MEM_H_

#include <stdint.h>

typedef void (*ram_handler_t)() ;
#include "cmsis_device.h"
#ifdef __cplusplus
extern "C" {
#endif

__attribute__((always_inline)) static inline uint32_t irq_critical_begin(){
	uint32_t flags = __get_PRIMASK();
	__disable_irq();
	return flags;
}
__attribute__((always_inline)) static inline void irq_critical_end(uint32_t flags){
	__set_PRIMASK(flags);
}

void install_isr(IRQn_Type isr, ram_handler_t handler);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <embxx\util\StaticFunction.h>
#include <array>

namespace os {
	namespace priv {
		// writes the code to do the callback, change this all to use StaticFunction?
		 void g_setup_callback(volatile uint16_t* code, uint32_t pc, uint32_t arg);
	}
	template<typename T>
	class irq_handler_t {
	public:
		void irq_install_handler(IRQn_Type isr){

			ram_handler_t chandler = reinterpret_cast<ram_handler_t>(&_handler_code[0]);
			install_isr(isr,chandler);
		}
		void irq_uninstall_handler(IRQn_Type isr){
			ram_handler_t chandler = reinterpret_cast<ram_handler_t>(&_handler_code[0]);
			//assert(get_isr(isr) == chandler); // debugging
			install_isr(isr,nullptr);
		}
	protected:
		friend T;
		void irq_handler() {  static_cast<T*>(this)->irq_handler(); }
		irq_handler_t() {
			// https://stackoverflow.com/questions/3068144/print-address-of-virtual-member-function
			void (irq_handler_t::*func_ptr)() = &irq_handler_t::irq_handler;
			priv::g_setup_callback(_handler_code, reinterpret_cast<uint32_t>((void*)(this->*func_ptr)), reinterpret_cast<uint32_t>(this));
		}
	private:
		friend class irq_handler_internal_t;

		__attribute__((aligned(4))) volatile uint16_t _handler_code[8];
		/*
		 * The code for the handlers is
		 * ldr r0, [pc #16]
		 * ldr r12, [pc #16]
		 * b r12
		 * .word this
		 * .word member
		 *
		 * so, to allign, each intterupt takes 16 bytes, but the advantage is the overhead is just 3 instructions on
		 * a member call  otherwise the calling gets big
		 * so when we install the handler we generate the code in _handler_code and give that to install isr
		 * this is done on construction as the offset to the function dosn't change
		 */
	};
}
namespace irq {

}










#endif

#endif /* INCLUDE_OS_ISR_MEM_H_ */
