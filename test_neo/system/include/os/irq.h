#ifndef INCLUDE_OS_ISR_MEM_H_
#define INCLUDE_OS_ISR_MEM_H_

#include <stdint.h>

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

void install_isr(IRQn_Type isr, void (*handler)(void));

#ifdef __cplusplus
}
#endif


#endif /* INCLUDE_OS_ISR_MEM_H_ */
