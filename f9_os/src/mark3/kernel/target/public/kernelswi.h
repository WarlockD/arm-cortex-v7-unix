/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012-2016 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
=========================================================================== */
/*!

    \file   kernelswi.h    

    \brief  Kernel Software interrupt declarations

 */


#include "kerneltypes.h"
#include "f9_context.h"
#ifndef __KERNELSWI_H_
#define __KERNELSWI_H_

//---------------------------------------------------------------------------
/*!
    Class providing the software-interrupt required for context-switching in 
    the kernel.
 */

enum class SysCall {
	SaveContext,
	RestoreContext,
	SwitchContext,
	Return, // return from user syscall
	ThreadStart,
	PthreadStart,
	SignalHandler,
	SignalReturn,
	SysCallSize
};

class KernelSWI
{

	// baisc syscall with no return
	template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call() {
		 register uint32_t reg0 __asm__("r0");
		 constexpr static uint8_t swi = SYSCALLNBR;
		  __asm volatile( "swi %0" : : "i"(swi): "lr");
		  return reg0;
	}
	template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0) {
		  register uint32_t reg0 __asm__("r0") = arg0;
			 constexpr static uint8_t swi = SYSCALLNBR;
		  __asm__ __volatile__( "swi %0": : "i"(swi) : "lr", "r0");
		  return reg0;
	}
	template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0,uint32_t arg1) {
		  register uint32_t reg0 __asm__("r0") = arg0;
		  register uint32_t reg1 __asm__("r1") = arg1;
			 constexpr static uint8_t swi = SYSCALLNBR;
		  __asm__ __volatile__( "swi %0": : "i"(swi): "lr", "r0", "r1");
		  return reg0;
	}
	template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0,uint32_t arg1,uint32_t arg2) {
		  register uint32_t reg0 __asm__("r0") = arg0;
		  register uint32_t reg1 __asm__("r1") = arg1;
		  register uint32_t reg2 __asm__("r2") = arg2;
			 constexpr static uint8_t swi = SYSCALLNBR;
		  __asm__ __volatile__( "swi %0": : "i"(swi): "lr", "r0", "r1", "r2");
		  return reg0;
	}
	template<uint8_t SYSCALLNBR> __attribute((always_inline)) static inline uint32_t _sys_call(uint32_t arg0,uint32_t arg1,uint32_t arg2,uint32_t arg3) {
		  register uint32_t reg0 __asm__("r0") = arg0;
		  register uint32_t reg1 __asm__("r1") = arg1;
		  register uint32_t reg2 __asm__("r2") = arg2;
		  register uint32_t reg3 __asm__("r3") = arg3;
		  constexpr static uint8_t swi = SYSCALLNBR;
		  __asm__ __volatile__( "swi %0": : "i"(swi): "lr", "r0", "r1","r2", "r3");
		  return reg0;
	}
	template<typename T>
	__attribute((always_inline)) static inline uint32_t arg_cast(T v) { return (uint32_t)v; }
public:
		template<uint8_t SYSCALLNBR, typename ... Args>
		__attribute((always_inline)) static inline void sys_call(Args ... args) {
			_sys_call<SYSCALLNBR>(arg_cast<Args>(args)...);
		}
		template<uint8_t SYSCALLNBR, typename RET, typename ... Args>
		__attribute((always_inline)) static inline RET sys_call_return(Args ... args) {
			return (RET)(_sys_call<SYSCALLNBR>(arg_cast<Args>(args)...));
		}
		template<SysCall SYSCALLNBR, typename ... Args>
		__attribute((always_inline)) static inline void sys_call(Args ... args) {
			_sys_call<static_cast<uint8_t>(SYSCALLNBR)>(arg_cast<Args>(args)...);
		}
		template<SysCall SYSCALLNBR, typename RET, typename ... Args>
		__attribute((always_inline)) static inline RET sys_call_return(Args ... args) {
			return (RET)(_sys_call<static_cast<uint8_t>(SYSCALLNBR)>(arg_cast<Args>(args)...));
		}
		__attribute((always_inline)) static inline void context_save(f9_context_t* ctx) {
			sys_call<SysCall::SaveContext>(ctx);
		}
		__attribute((always_inline)) static inline void context_restore(f9_context_t* ctx) {
			sys_call<SysCall::RestoreContext>(ctx);
		}
		__attribute((always_inline)) static inline void switch_to(f9_context_t* from,f9_context_t* to) {
			sys_call<SysCall::SwitchContext>(from,to);
		}
    /*!
     *  \brief Config
     *
     *  Configure the software interrupt - must be called before any other 
     *  software interrupt functions are called.
     */
    static void Config(void);

    /*!
     *  \brief Start
     *
     *  Enable ("Start") the software interrupt functionality
     */
    static void Start(void);
    
    /*!
     *  \brief Stop
     *
     *  Disable the software interrupt functionality
     */
    static void Stop(void);
    
    /*!
     *  \brief Clear
     *
     *  Clear the software interrupt
     */
    static void Clear(void);
    
    /*!
     *  \brief Trigger
     *
     *  Call the software interrupt     
     */
    static void Trigger(void);
    
    /*!
     *  \brief DI
     *
     *  Disable the SWI flag itself
     *  
     *  \return previous status of the SWI, prior to the DI call
     */
    static uint8_t DI();
    
    /*!
     *  \brief RI
     *
     *  Restore the state of the SWI to the value specified
     *  
     *  \param bEnable_ true - enable the SWI, false - disable SWI
     */        
    static void RI(bool bEnable_);    
};


#endif // __KERNELSIW_H_
