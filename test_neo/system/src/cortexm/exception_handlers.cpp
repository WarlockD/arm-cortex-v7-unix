/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include "cortexm/ExceptionHandlers.h"
#include "cmsis_device.h"
#include "arm/semihosting.h"
#include "diag/Trace.h"
#include <string.h>
#include <cassert>
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Default exception handlers. Override the ones here by defining your own
// handler routines in your application code.
// ----------------------------------------------------------------------------

#if defined(TRACE)
// I am the god of regex
//#define MAKE_ENUM_TEXT(VALUE,MESSAGE) { .value = VALUE, .text = #VALUE, .message = MESSAGE }
#define MAKE_ENUM_TEXT(VALUE,MESSAGE)  enum_info(VALUE, #VALUE, MESSAGE)
struct enum_info {
	int value;
	const char* text;
	const char* message;
	enum_info(int value, const char* text, const char* message) : value(value), text(text), message(message) {}
};

const enum_info* get_irq_info(int irq) {
static const enum_info trap_strings[] = {
		  MAKE_ENUM_TEXT(NonMaskableInt_IRQn,"2 Non Maskable Interrupt"),
		  MAKE_ENUM_TEXT(MemoryManagement_IRQn,"4 Cortex-M7 Memory Management Interrupt"),
		  MAKE_ENUM_TEXT(BusFault_IRQn,"5 Cortex-M7 Bus Fault Interrupt"),
		  MAKE_ENUM_TEXT(UsageFault_IRQn,"6 Cortex-M7 Usage Fault Interrupt"),
		  MAKE_ENUM_TEXT(SVCall_IRQn,"11 Cortex-M7 SV Call Interrupt"),
		  MAKE_ENUM_TEXT(DebugMonitor_IRQn,"12 Cortex-M7 Debug Monitor Interrupt"),
		  MAKE_ENUM_TEXT(PendSV_IRQn,"14 Cortex-M7 Pend SV Interrupt"),
		  MAKE_ENUM_TEXT(SysTick_IRQn,"15 Cortex-M7 System Tick Interrupt"),
		  MAKE_ENUM_TEXT(WWDG_IRQn,"Window WatchDog Interrupt"),
		  MAKE_ENUM_TEXT(PVD_IRQn,"PVD through EXTI Line detection Interrupt"),
		  MAKE_ENUM_TEXT(TAMP_STAMP_IRQn,"Tamper and TimeStamp interrupts through the EXTI line"),
		  MAKE_ENUM_TEXT(RTC_WKUP_IRQn,"RTC Wakeup interrupt through the EXTI line"),
		  MAKE_ENUM_TEXT(FLASH_IRQn,"FLASH global Interrupt"),
		  MAKE_ENUM_TEXT(RCC_IRQn,"RCC global Interrupt"),
		  MAKE_ENUM_TEXT(EXTI0_IRQn,"EXTI Line0 Interrupt"),
		  MAKE_ENUM_TEXT(EXTI1_IRQn,"EXTI Line1 Interrupt"),
		  MAKE_ENUM_TEXT(EXTI2_IRQn,"EXTI Line2 Interrupt"),
		  MAKE_ENUM_TEXT(EXTI3_IRQn,"EXTI Line3 Interrupt"),
		  MAKE_ENUM_TEXT(EXTI4_IRQn,"EXTI Line4 Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream0_IRQn,"DMA1 Stream 0 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream1_IRQn,"DMA1 Stream 1 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream2_IRQn,"DMA1 Stream 2 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream3_IRQn,"DMA1 Stream 3 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream4_IRQn,"DMA1 Stream 4 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream5_IRQn,"DMA1 Stream 5 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream6_IRQn,"DMA1 Stream 6 global Interrupt"),
		  MAKE_ENUM_TEXT(ADC_IRQn,"ADC1, ADC2 and ADC3 global Interrupts"),
		  MAKE_ENUM_TEXT(CAN1_TX_IRQn,"CAN1 TX Interrupt"),
		  MAKE_ENUM_TEXT(CAN1_RX0_IRQn,"CAN1 RX0 Interrupt"),
		  MAKE_ENUM_TEXT(CAN1_RX1_IRQn,"CAN1 RX1 Interrupt"),
		  MAKE_ENUM_TEXT(CAN1_SCE_IRQn,"CAN1 SCE Interrupt"),
		  MAKE_ENUM_TEXT(EXTI9_5_IRQn,"External Line[9:5] Interrupts"),
		  MAKE_ENUM_TEXT(TIM1_BRK_TIM9_IRQn,"TIM1 Break interrupt and TIM9 global interrupt"),
		  MAKE_ENUM_TEXT(TIM1_UP_TIM10_IRQn,"TIM1 Update Interrupt and TIM10 global interrupt"),
		  MAKE_ENUM_TEXT(TIM1_TRG_COM_TIM11_IRQn,"TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt"),
		  MAKE_ENUM_TEXT(TIM1_CC_IRQn,"TIM1 Capture Compare Interrupt"),
		  MAKE_ENUM_TEXT(TIM2_IRQn,"TIM2 global Interrupt"),
		  MAKE_ENUM_TEXT(TIM3_IRQn,"TIM3 global Interrupt"),
		  MAKE_ENUM_TEXT(TIM4_IRQn,"TIM4 global Interrupt"),
		  MAKE_ENUM_TEXT(I2C1_EV_IRQn,"I2C1 Event Interrupt"),
		  MAKE_ENUM_TEXT(I2C1_ER_IRQn,"I2C1 Error Interrupt"),
		  MAKE_ENUM_TEXT(I2C2_EV_IRQn,"I2C2 Event Interrupt"),
		  MAKE_ENUM_TEXT(I2C2_ER_IRQn,"I2C2 Error Interrupt"),
		  MAKE_ENUM_TEXT(SPI1_IRQn,"SPI1 global Interrupt"),
		  MAKE_ENUM_TEXT(SPI2_IRQn,"SPI2 global Interrupt"),
		  MAKE_ENUM_TEXT(USART1_IRQn,"USART1 global Interrupt"),
		  MAKE_ENUM_TEXT(USART2_IRQn,"USART2 global Interrupt"),
		  MAKE_ENUM_TEXT(USART3_IRQn,"USART3 global Interrupt"),
		  MAKE_ENUM_TEXT(EXTI15_10_IRQn,"External Line[15:10] Interrupts"),
		  MAKE_ENUM_TEXT(RTC_Alarm_IRQn,"RTC Alarm (A and B) through EXTI Line Interrupt"),
		  MAKE_ENUM_TEXT(OTG_FS_WKUP_IRQn,"USB OTG FS Wakeup through EXTI line interrupt"),
		  MAKE_ENUM_TEXT(TIM8_BRK_TIM12_IRQn,"TIM8 Break Interrupt and TIM12 global interrupt"),
		  MAKE_ENUM_TEXT(TIM8_UP_TIM13_IRQn,"TIM8 Update Interrupt and TIM13 global interrupt"),
		  MAKE_ENUM_TEXT(TIM8_TRG_COM_TIM14_IRQn,"TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt"),
		  MAKE_ENUM_TEXT(TIM8_CC_IRQn,"TIM8 Capture Compare Interrupt"),
		  MAKE_ENUM_TEXT(DMA1_Stream7_IRQn,"DMA1 Stream7 Interrupt"),
		  MAKE_ENUM_TEXT(FMC_IRQn,"FMC global Interrupt"),
		  MAKE_ENUM_TEXT(SDMMC1_IRQn,"SDMMC1 global Interrupt"),
		  MAKE_ENUM_TEXT(TIM5_IRQn,"TIM5 global Interrupt"),
		  MAKE_ENUM_TEXT(SPI3_IRQn,"SPI3 global Interrupt"),
		  MAKE_ENUM_TEXT(UART4_IRQn,"UART4 global Interrupt"),
		  MAKE_ENUM_TEXT(UART5_IRQn,"UART5 global Interrupt"),
		  MAKE_ENUM_TEXT(TIM6_DAC_IRQn,"TIM6 global and DAC1&2 underrun error  interrupts"),
		  MAKE_ENUM_TEXT(TIM7_IRQn,"TIM7 global interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream0_IRQn,"DMA2 Stream 0 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream1_IRQn,"DMA2 Stream 1 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream2_IRQn,"DMA2 Stream 2 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream3_IRQn,"DMA2 Stream 3 global Interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream4_IRQn,"DMA2 Stream 4 global Interrupt"),
		  MAKE_ENUM_TEXT(ETH_IRQn,"Ethernet global Interrupt"),
		  MAKE_ENUM_TEXT(ETH_WKUP_IRQn,"Ethernet Wakeup through EXTI line Interrupt"),
		  MAKE_ENUM_TEXT(CAN2_TX_IRQn,"CAN2 TX Interrupt"),
		  MAKE_ENUM_TEXT(CAN2_RX0_IRQn,"CAN2 RX0 Interrupt"),
		  MAKE_ENUM_TEXT(CAN2_RX1_IRQn,"CAN2 RX1 Interrupt"),
		  MAKE_ENUM_TEXT(CAN2_SCE_IRQn,"CAN2 SCE Interrupt"),
		  MAKE_ENUM_TEXT(OTG_FS_IRQn,"USB OTG FS global Interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream5_IRQn,"DMA2 Stream 5 global interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream6_IRQn,"DMA2 Stream 6 global interrupt"),
		  MAKE_ENUM_TEXT(DMA2_Stream7_IRQn,"DMA2 Stream 7 global interrupt"),
		  MAKE_ENUM_TEXT(USART6_IRQn,"USART6 global interrupt"),
		  MAKE_ENUM_TEXT(I2C3_EV_IRQn,"I2C3 event interrupt"),
		  MAKE_ENUM_TEXT(I2C3_ER_IRQn,"I2C3 error interrupt"),
		  MAKE_ENUM_TEXT(OTG_HS_EP1_OUT_IRQn,"USB OTG HS End Point 1 Out global interrupt"),
		  MAKE_ENUM_TEXT(OTG_HS_EP1_IN_IRQn,"USB OTG HS End Point 1 In global interrupt"),
		  MAKE_ENUM_TEXT(OTG_HS_WKUP_IRQn,"USB OTG HS Wakeup through EXTI interrupt"),
		  MAKE_ENUM_TEXT(OTG_HS_IRQn,"USB OTG HS global interrupt"),
		  MAKE_ENUM_TEXT(DCMI_IRQn,"DCMI global interrupt"),
		  MAKE_ENUM_TEXT(RNG_IRQn,"RNG global interrupt"),
		  MAKE_ENUM_TEXT(FPU_IRQn,"FPU global interrupt"),
		  MAKE_ENUM_TEXT(UART7_IRQn,"UART7 global interrupt"),
		  MAKE_ENUM_TEXT(UART8_IRQn,"UART8 global interrupt"),
		  MAKE_ENUM_TEXT(SPI4_IRQn,"SPI4 global Interrupt"),
		  MAKE_ENUM_TEXT(SPI5_IRQn,"SPI5 global Interrupt"),
		  MAKE_ENUM_TEXT(SPI6_IRQn,"SPI6 global Interrupt"),
		  MAKE_ENUM_TEXT(SAI1_IRQn,"SAI1 global Interrupt"),
		  MAKE_ENUM_TEXT(LTDC_IRQn,"LTDC global Interrupt"),
		  MAKE_ENUM_TEXT(LTDC_ER_IRQn,"LTDC Error global Interrupt"),
		  MAKE_ENUM_TEXT(DMA2D_IRQn,"DMA2D global Interrupt"),
		  MAKE_ENUM_TEXT(SAI2_IRQn,"SAI2 global Interrupt"),
		  MAKE_ENUM_TEXT(QUADSPI_IRQn,"Quad SPI global interrupt"),
		  MAKE_ENUM_TEXT(LPTIM1_IRQn,"LP TIM1 interrupt"),
		  MAKE_ENUM_TEXT(CEC_IRQn,"HDMI-CEC global Interrupt"),
		  MAKE_ENUM_TEXT(I2C4_EV_IRQn,"I2C4 Event Interrupt"),
		  MAKE_ENUM_TEXT(I2C4_ER_IRQn,"I2C4 Error Interrupt"),
		  MAKE_ENUM_TEXT(SPDIF_RX_IRQn,"SPDIF-RX global Interrupt"),
};
	for(auto& i : trap_strings)if(i.value == irq) return &i;
	return nullptr;
}
struct fault_info {
	int mask;
	const char* text;
	const char* unset;
	fault_info(int mask, const char* text) : mask(mask), text(text),unset(nullptr) {}
	fault_info(int mask, const char* text,const char* unset) : mask(mask), text(text),unset(unset) {}
};
#define FAULT_TEXT(BIT,TEXT) fault_info((1<<(BIT)),(TEXT)),
#define FLAG_TEXT(BIT,TEXT,DESC) fault_info((1<<(BIT)),(TEXT),(DESC)),

/*
 * PSR bits
 */
#define USR26_MODE	0x00000000
#define FIQ26_MODE	0x00000001
#define IRQ26_MODE	0x00000002
#define SVC26_MODE	0x00000003
#define USR_MODE	0x00000000
#define SVC_MODE	0x00000000
#define FIQ_MODE	0x00000011
#define IRQ_MODE	0x00000012
#define ABT_MODE	0x00000017
#define UND_MODE	0x0000001b
#define SYSTEM_MODE	0x0000001f
#define MODE32_BIT	0x00000010
#define MODE_MASK	0x0000001f
#define PSR_T_BIT	0x01000000
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT	0x00000100
#define PSR_E_BIT	0x00000200
#define PSR_J_BIT	0x01000000
#define PSR_Q_BIT	0x08000000
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000

static  fault_info mem_fault_info[] = {
	FAULT_TEXT(7,"MMARVALID")
	FAULT_TEXT(5,"MLSPERR")
	FAULT_TEXT(4,"MSTKERR")
	FAULT_TEXT(3,"MUNSTKERR")
	FAULT_TEXT(1,"DACCVIOL")
	FAULT_TEXT(0,"IACCVIOL")
};
static  fault_info bus_fault_info[] = {
	FAULT_TEXT(7,"BFARVALID")
	FAULT_TEXT(5,"LSPERR")
	FAULT_TEXT(4,"STKERR")
	FAULT_TEXT(3,"UNSTKERR")
	FAULT_TEXT(2,"IMPRECISERR")
	FAULT_TEXT(1,"PRECISERR")
	FAULT_TEXT(0,"IBUSERR")
};
static  fault_info use_fault_info[] = {
	FAULT_TEXT(9,"DIVBYZERO")
	FAULT_TEXT(8,"UNALIGNED")
	FAULT_TEXT(3,"NOCP")
	FAULT_TEXT(2,"INVPC")
	FAULT_TEXT(1,"INVSTATE")
	FAULT_TEXT(0,"UNDEFINSTR")
};
static fault_info hard_fault_info[] = {
	FAULT_TEXT(31,"DEBUGEVT")
	FAULT_TEXT(30,"FORCED")
	FAULT_TEXT(1,"VECTTBL")
};
#define for_each_fault(V,VAL, A) for(auto& V, A) if(V.mask & (VAL))
template<size_t N>
static const char* display_faults( uint32_t value, fault_info(&fault_list)[N]){
	if(value == 0) return ""; // no faults
	static char buf[128];
	size_t pos =0;
	auto pushc = [&pos](int c) {
		assert(pos < (sizeof(buf)-1));
		buf[pos++] = c;
	};
	bool comma = false;
	pushc('<');
	for(auto& f : fault_list) {
		if((f.mask & value)!=0) {
			if(comma) pushc(','); else comma = true;
			const char* m = f.text;
			while(*m) pushc(*m++);
		}
	}
	pushc('>');
	pushc(0);
	return buf;
}
void display_faults() {
	uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
	uint32_t bfar = SCB->BFAR; 	// Bus Fault Address
	uint32_t cfsr = SCB->CFSR; 	// Configurable Fault Status Registers
	uint32_t afsr = SCB->AFSR;
	uint32_t dfsr = SCB->DFSR;
	uint32_t hfsr = SCB->HFSR;

	uint32_t mem_fault = (cfsr & SCB_CFSR_MEMFAULTSR_Msk)>> SCB_CFSR_MEMFAULTSR_Pos;
	uint32_t bus_fault = (cfsr & SCB_CFSR_BUSFAULTSR_Msk)>> SCB_CFSR_BUSFAULTSR_Pos;
	uint32_t use_fault = (cfsr & SCB_CFSR_USGFAULTSR_Msk)>> SCB_CFSR_USGFAULTSR_Pos;

  trace_printf ("FSR/FAR:\n");
  if(cfsr) {
	  trace_printf (" CFSR =  %08X\n", cfsr);
	  if(mem_fault) {
		  if(mem_fault&(1<<7)) // valid fault address
			  trace_printf("   MMFSR = %08X%s(%08X)\n",mem_fault,display_faults(mem_fault,mem_fault_info),mmfar);
		  else
			  trace_printf("   MMFSR = %08X%s\n",mem_fault,display_faults(mem_fault,mem_fault_info));
	  }
	  if(bus_fault) {
		  if(bus_fault&(1<<7)) // valid fault address
			  trace_printf("    BFSR = %08X%s(%08X)\n",mem_fault,display_faults(bus_fault,bus_fault_info),bfar);
		  else
			  trace_printf("    BFSR = %08X%s\n",mem_fault,display_faults(bus_fault,bus_fault_info));
	  }
	  if(use_fault) trace_printf("    UFSR = %08X%s\n",mem_fault,display_faults(use_fault,use_fault_info));
  }
  if(hfsr) {
	  trace_printf(" HFSR = %08X%s\n",hfsr,display_faults(hfsr,hard_fault_info));
  }
  if(dfsr) {
	  trace_printf (" DFSR =  %08X\n", dfsr);
  }
  if(afsr) {
	  trace_printf (" AFSR =  %08X\n", afsr);
  }
  trace_putchar('\n');
}






// ----------------------------------------------------------------------------



// The values of BFAR and MMFAR stay unchanged if the BFARVALID or
// MMARVALID is set. However, if a new fault occurs during the
// execution of this fault handler, the value of the BFAR and MMFAR
// could potentially be erased. In order to ensure the fault addresses
// accessed are valid, the following procedure should be used:
// 1. Read BFAR/MMFAR.
// 2. Read CFSR to get BFARVALID or MMARVALID. If the value is 0, the
//    value of BFAR or MMFAR accessed can be invalid and can be discarded.
// 3. Optionally clear BFARVALID or MMARVALID.
// (See Joseph Yiu's book).

void dump_xpsr(uint32_t xpsr) {
	static char buf[7];
	buf[0] = xpsr & (1<<31) ? 'N' : 'n';
	buf[1] = xpsr & (1<<30) ? 'Z' : 'z';
	buf[2] = xpsr & (1<<29) ? 'C' : 'c';
	buf[3] = xpsr & (1<<28) ? 'V' : 'v';
	buf[4] = xpsr & (1<<27) ? 'Q' : 'q'; // satuation flag
	buf[5] = xpsr & (1<<24) ? 'T' : 't'; // thumb
	buf[6] = '\0';
	trace_printf(" XPSR = %08X[%s] IPSR: %d]\n", xpsr, buf, xpsr & 0xFF);
}
void dumpExceptionStack (ExceptionStackFrame* frame, uint32_t lr)
{
  trace_printf ("Stack frame:\n");
  trace_printf (" R0  = %08X\n", frame->r0);
  trace_printf (" R1  = %08X\n", frame->r1);
  trace_printf (" R2  = %08X\n", frame->r2);
  trace_printf (" R3  = %08X\n", frame->r3);
  trace_printf (" R12 = %08X\n", frame->r12);
  trace_printf (" LR  = %08X\n", frame->lr);
  trace_printf (" PC  = %08X\n", frame->pc);
  dump_xpsr(frame->psr);
  trace_printf (" SP  = %08X\n", (uint32_t)(frame-1));
  trace_printf (" LR/EXC_RETURN= %08X\n", lr);
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  display_faults();
#endif
}


// default jump for unused handlers
extern const char* g_vector_names[];

__attribute__((always_inline)) static inline void debug_halt_loop() {
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1);
}

extern "C" void __attribute__ ((section(".after_vectors"),weak,used))
//Default_Handler_C (ExceptionStackFrame* frame __attribute__((unused)), uint32_t lr __attribute__((unused)),uint32_t ipsr __attribute__((unused)))
	Default_Handler_C (ExceptionStackFrame* frame, uint32_t lr)
{
#if defined(TRACE)
	int _ipsr = __get_IPSR();
  trace_printf ("[(%d)%s]\n",_ipsr,g_vector_names[_ipsr]);
  dumpExceptionStack (frame, lr);
#endif // defined(TRACE)
  debug_halt_loop();
}
__attribute__((always_inline)) static inline void debug_fault()  {
	  asm volatile(
	      " tst lr,#4       \n"
	      " ite eq          \n"
	      " mrseq r0,msp    \n"
	      " mrsne r0,psp    \n"
	      " mov r1,lr       \n"
	      " ldr r2,=Default_Handler_C \n"
	      " bx r2"
		  : /* Outputs */
		  : /* Inputs */
		  : /* Clobbers */
	  );
}




// The DEBUG version is not naked, but has a proper stack frame,
// to allow setting breakpoints at Reset_Handler.
extern "C" __attribute__((noreturn,weak)) void  _start (void);
extern "C" void __attribute__ ((section(".after_vectors"),noreturn)) Reset_Handler (void){ _start(); }

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M3 and
// Cortex-M4 Processors, Third Edition, Chap. 12.8, page 402).
// theyse are all weak so they can be overwriten
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) HardFault_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) BusFault_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) UsageFault_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) MemManage_Handler (void) { debug_fault(); }

extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) SVC_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) DebugMon_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) PendSV_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) SysTick_Handler (void) { debug_fault(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) NMI_Handler (void) { debug_fault(); }

#else
extern "C" __attribute__((noreturn,weak)) void  _start (void);
extern "C" void __attribute__ ((section(".after_vectors"),naked)) Reset_Handler (void){ __asm volatile(" ldr     r0,=_start\nbx      r0"}; }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) SVC_Handler (void) { debug_halt_loop(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) DebugMon_Handler (void) { debug_halt_loop(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) PendSV_Handler (void) { debug_halt_loop(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak,naked)) SysTick_Handler (void) { debug_halt_loop(); }
extern "C" void __attribute__ ((section(".after_vectors"),weak)) NMI_Handler (void) { debug_halt_loop(); }
#endif
