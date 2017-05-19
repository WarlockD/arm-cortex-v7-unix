#ifndef PLATFORM_LINK_H_
#define PLATFORM_LINK_H_

#include "types.hpp"


/*
 * Entries about linker address
 */

extern uint32_t bss_start;
extern uint32_t bss_end;

#ifdef LOADER
extern uint32_t stack_end;
extern uint32_t kernel_flash_start;
extern uint32_t kernel_start;
extern uint32_t kernel_end;
extern uint32_t data_start;
extern uint32_t data_end;
extern uint32_t stack_end;
#else
extern uint32_t kernel_text_start;
extern uint32_t kernel_text_end;
extern uint32_t kernel_data_start;
extern uint32_t kernel_data_end;
extern uint32_t kernel_bss_start;
extern uint32_t kernel_bss_end;
extern uint32_t kernel_ahb_start;
extern uint32_t kernel_ahb_end;

extern uint32_t idle_stack_start;
extern uint32_t idle_stack_end;
extern uint32_t kernel_stack_end;

extern uint32_t user_text_start;
extern uint32_t user_text_end;
extern uint32_t user_data_start;
extern uint32_t user_data_end;
extern uint32_t user_bss_start;
extern uint32_t user_bss_end;
extern uint32_t user_text_flash_start;
extern uint32_t user_text_flash_end;

extern uint32_t root_stack_start;
extern uint32_t root_stack_end;

extern uint32_t kip_start;
extern uint32_t kip_end;

extern uint32_t bitmap_start;
extern uint32_t bitmap_end;
extern uint32_t bitmap_bitband_start;
extern uint32_t bitmap_bitband_end;

extern uint32_t symtab_start;
extern uint32_t symtab_end;

extern uint32_t mem0_start;
extern uint32_t mem1_start;
#endif

#define __BSS 			__attribute__ ((section(".bss")))
#define __ISR_VECTOR		__attribute__ ((section(".isr_vector")))
#ifndef LOADER
#define __KIP 			__attribute__ ((section(".kip")))
#define __KTABLE		__attribute__ ((section(".ktable")))

#ifdef CONFIG_BITMAP_BITBAND
#define __BITMAP		__attribute__ ((section(".bitmap_bitband")))
#else
#define __BITMAP		__attribute__ ((section(".bitmap")))
#endif

#define __USER_TEXT		__attribute__ ((section(".user_text")))
#define __USER_DATA		__attribute__ ((section(".user_data")))
#define __USER_BSS		__attribute__ ((section(".user_bss")))
#define __USER_SC		__attribute__ ((section(".syscall")))
#endif

#define __PACKED		__attribute__ ((packed))

#define __NAKED 		__attribute__ ((naked))

#endif	/* PLATFORM_LINK_H_ */
