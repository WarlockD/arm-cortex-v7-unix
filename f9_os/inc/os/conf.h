/*
 * conf.h
 *
 *  Created on: Mar 11, 2017
 *      Author: warlo
 */

#ifndef OS_CONF_H_
#define OS_CONF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef STM32F7
#include <stm32f7xx.h>
#include <stm32f7xx_hal.h>
#endif

#define PIROITY_STEP 0x01
#define PIROITY_HIGHEST PIROITY_STEP
#define PRIOITY_LOWEST 0x0F

static inline void restore_flags(uint32_t flags) { __set_BASEPRI(flags); }
static inline uint32_t save_flags() { return __get_BASEPRI(); }
static inline void disable_irq() { __set_BASEPRI(0);  }
static inline void enable_irq() { __set_BASEPRI(PRIOITY_LOWEST);  }
static inline uint32_t enter_critical() {
	uint32_t flags = save_flags(); disable_irq(); return flags;
}
static inline void exit_critical(uint32_t flags) { restore_flags(flags); }
#endif /* OS_CONF_H_ */
