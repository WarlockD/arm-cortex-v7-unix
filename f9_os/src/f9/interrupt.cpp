
#include "thread.hpp"

extern void (* const g_pfnVectors[])(void);

#define AIRCR_VECTKEY_MASK    ((uint32_t) 0x05FA0000)
extern "C" void Default_Handler();
#define _undefined_handler Default_Handler

/* for the sake of saving space, provide default device IRQ handler with
 * weak alias.
 */
#define DEFAULT_IRQ_VEC(n)						\
	void nvic_handler##n(void)					\
		__attribute__((weak, alias("_undefined_handler")));

namespace f9 {
	static int nvic_is_setup(irqn irq) { return !(g_pfnVectors[irq + 16] == _undefined_handler);}
	void user_irq_enable(irqn irq) { if (nvic_is_setup(irq)) NVIC_EnableIRQ(irq); }
	void user_irq_disable(irqn irq) { if (nvic_is_setup(irq)) { NVIC_ClearPendingIRQ(irq); NVIC_DisableIRQ(irq); } }
	void user_irq_set_pending(irqn irq) { if (nvic_is_setup(irq)) NVIC_SetPendingIRQ(irq); }
	void user_irq_clear_pending(irqn irq) { if (nvic_is_setup(irq)) NVIC_ClearPendingIRQ(irq); }

	void __interrupt_handler(irqn n);



};
