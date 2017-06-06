/*
 * syscall.h
 *
 *  Created on: Jun 3, 2017
 *      Author: warlo
 */

#ifndef MIMIX_CPP_SYSCALL_HPP_
#define MIMIX_CPP_SYSCALL_HPP_

#include <array>
#include <type_traits>
#include <cstdint>
#include <cstddef>

#include "reg.hpp"

namespace os {

	namespace priv {
		// baisc syscall with no return
		template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call() {
			 register uint32_t reg0 __asm__("r0");
			  __asm__ __volatile__( "swi %1": : "i"(SYSCALLNBR), "lr");
			  return reg0;
		}
		template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0) {
			  register uint32_t reg0 __asm__("r0") = arg0;
			  __asm__ __volatile__( "swi %1": : "i"(SYSCALLNBR) : "lr", "r0");
			  return reg0;
		}
		template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0,uint32_t arg1) {
			  register uint32_t reg0 __asm__("r0") = arg0;
			  register uint32_t reg1 __asm__("r1") = arg1;
			  __asm__ __volatile__( "swi %1": : "i"(SYSCALLNBR): "lr", "r0", "r1");
			  return reg0;
		}
		template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0,uint32_t arg1,uint32_t arg2) {
			  register uint32_t reg0 __asm__("r0") = arg0;
			  register uint32_t reg1 __asm__("r1") = arg1;
			  register uint32_t reg1 __asm__("r2") = arg2;
			  __asm__ __volatile__( "swi %1": : "i"(SYSCALLNBR): "lr", "r0", "r1", "r2");
			  return reg0;
		}
		template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0,uint32_t arg1,uint32_t arg2,uint32_t arg3) {
			  register uint32_t reg0 __asm__("r0") = arg0;
			  register uint32_t reg1 __asm__("r1") = arg1;
			  register uint32_t reg0 __asm__("r2") = arg2;
			  register uint32_t reg1 __asm__("r3") = arg3;
			  __asm__ __volatile__( "swi %1": : "i"(SYSCALLNBR): "lr", "r0", "r1","r2", "r3");
			  return reg0;
		}
		template<typename T>
		__attribute((always_inline)) static inline uint32_t arg_cast(T v) { return (uint32_t)v; }
	};
	template<uint8_t SYSCALLNBR, typename ... Args>
	__attribute((always_inline)) static inline void sys_call(Args ... args) {
		priv::_sys_call(arg_cast<Args>(args)...);
	}
	template<uint8_t SYSCALLNBR, typename RET, typename ... Args>
	__attribute((always_inline)) static inline RET sys_call_return(Args ... args) {
		return (RET)(priv::_sys_call(arg_cast<Args>(args)...));
	}
	void syscall_init(uint8_t nbr, uintptr_t call);
	template<typename T>
	__attribute((always_inline)) static inline
	void syscall_init(uint8_t nbr, T call) { syscall_init(nbr,(uintptr_t)call); }
} /* namespace f9 */

#endif /* MIMIX_CPP_SYSCALL_HPP_ */
