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
===========================================================================*/
/*!

    \file   kernelswi.cpp

    \brief  Kernel Software interrupt implementation for ARM Cortex-M4

*/

#include "kerneltypes.h"
#include "kernelswi.h"
#include "thread.h"

#include <stm32f7xx.h>


extern "C" {
	void SVC_Handler( void ) __attribute__ (( naked ));
	//void SVC_Handler( void ) ;
};
#include <cassert>

struct hw_context {
	uint32_t* _regs;
};
enum sw_regs_t {
	REG_SP = 0,
	REG_R4,
	REG_R5,
	REG_R6,
	REG_R7,
	REG_R8,
	REG_R9,
	REG_R10,
	REG_R11,
	REG_RET,
};
static uint32_t get_swi(uint32_t* hw_regs){
	return ((char *)hw_regs[REG_PC])[-2];
}
static void svc_save_context(f9_context_t* ctx, uint32_t* sw_regs, uint32_t* hw_regs) {
	ctx->sp = sw_regs[REG_SP];
	ctx->ret = sw_regs[REG_RET]; // lr
	ctx->ctl = __get_CONTROL();
	std::copy(ctx->regs,ctx->regs + sizeof(ctx->regs)/4,sw_regs+1);
}
static void svc_restore_context(f9_context_t* ctx, uint32_t* sw_regs, uint32_t* hw_regs) {
	std::copy(sw_regs+1,sw_regs+9,ctx->regs);
	sw_regs[REG_RET] = ctx->ret;
	sw_regs[REG_SP] = ctx->sp;
	__set_CONTROL(ctx->ctl);
}
//static f9_context_t syscall_context;
// otherwise we will have to schedual a call back on kernel mode for the syscall
extern Thread *g_pclCurrent;   //!< Pointer to the currently-running thread
extern "C" void printk(const char*, ...);
extern "C" void SVC_Handler(void)
{
	// so we are fucking up r4 here damnit
	register uint32_t* sw_regs __asm__("r0");
	register uint32_t* hw_regs __asm__("r3");
	__asm  volatile("tst lr, #0x4");
	__asm__ __volatile("ite eq");
	__asm__ __volatile("mrseq r3,  msp" ::: "r3");
	__asm__ __volatile("mrsne r3, psp"::: "r3");
	__asm volatile("stmdb sp!, { r3-r11, lr}");// backup eveything
	f9_context_t* ctx;
	uint32_t svc_num = get_swi(hw_regs);
	printk("SVC handler %d %p %p\n", svc_num,hw_regs[REG_R0],hw_regs[REG_R2] );
	if(svc_num <= static_cast<uint32_t>(SysCall::SysCallSize)){
		switch(static_cast<SysCall>(svc_num)){
			case SysCall::SaveContext:
				ctx = reinterpret_cast<f9_context_t*>(hw_regs[REG_R0]);
				svc_save_context(ctx, sw_regs,hw_regs);
				break;
			case SysCall::RestoreContext:
				ctx = reinterpret_cast<f9_context_t*>(hw_regs[REG_R0]);
				svc_restore_context(ctx, sw_regs,hw_regs);
				break;
			case SysCall::SwitchContext:
				ctx = reinterpret_cast<f9_context_t*>(hw_regs[REG_R0]);
				svc_save_context(ctx, sw_regs,hw_regs);
				ctx = reinterpret_cast<f9_context_t*>(hw_regs[REG_R1]);
				svc_restore_context(ctx, sw_regs,hw_regs);
				break; // hacky? have to see how it codes out
			case SysCall::Return: break;// return from user syscall
			case SysCall::ThreadStart:break;
			case SysCall::PthreadStart:break;
			case SysCall::SignalHandler:break;
			case SysCall::SignalReturn: break;
			default:
				assert(0); // oops
				break;
		}
	}
	__asm volatile("ldmia sp!, { r3-r11, lr}");// restore eveything
	__asm volatile ("bx lr");
}

//---------------------------------------------------------------------------
void KernelSWI::Config(void)
{
    uint8_t u8MinPriority = (uint8_t)((1 << __NVIC_PRIO_BITS) - 1);
    NVIC_SetPriority(SVCall_IRQn, u8MinPriority);
    NVIC_SetPriority(PendSV_IRQn, u8MinPriority);
    Clear();
}

//---------------------------------------------------------------------------    
void KernelSWI::Start(void)
{        
    // Nothing to do...
}

//---------------------------------------------------------------------------
void KernelSWI::Stop(void)
{
    // Nothing to do...
}

//---------------------------------------------------------------------------
uint8_t KernelSWI::DI()
{
    // Not implemented
    return 0;
}

//---------------------------------------------------------------------------
void KernelSWI::RI(bool bEnable_)
{
	(void)bEnable_;
    // Not implemented
}

//---------------------------------------------------------------------------
void KernelSWI::Clear(void)
{    
    // There's no convenient CMSIS function call for PendSV set/clear,
    // But we do at least have some structs/macros.
    
    // Note that set/clear each have their own bits in the same register.
    // Setting the "set" or "clear" bit results in the desired operation.
    SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk;
}

//---------------------------------------------------------------------------
void KernelSWI::Trigger(void)
{    
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}
