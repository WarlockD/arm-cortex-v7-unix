
// The ARM UART, a memory mapped device
#include <stm32f7xx_hal.h>
#include <stm32f7xx.h>
#include <diag\Trace.h>

struct trapframe {

#define REGS_BEGIN  union { uintptr_t regs[20]; struct {
#define REGS_VAL(name)	uint32_t name;
#define REGS_END }; };

#include "reg.h"
};
// I am the god of regex
//#define MAKE_ENUM_TEXT(VALUE,MESSAGE) { .value = VALUE, .text = #VALUE, .message = MESSAGE }
#define MAKE_ENUM_TEXT(VALUE,MESSAGE)  enum_info(VALUE, #VALUE, MESSAGE)
struct enum_info {
	int value;
	const char* text;
	const char* message;
	enum_info(int value, const char* text, const char* message) : value(value), text(text), message(message) {}
};
// special case cause eveything is fucked
struct hw_trap {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t ip;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
};
struct sw_trap {
	uint32_t sp;
	uint32_t basepri;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t fp;
	uint32_t ret; // ex lr
};
struct hwsw_trap {
	union {
		struct {
			struct sw_trap sw;
			struct hw_trap hw;
		};
		trapframe ctx;
	};
};
struct sw_trap  hard_fault_trap;
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


void dump_hw_trap(hw_trap * trap) {
	trace_printf("r0: 0x%08x r1: 0x%08x r2: 0x%08x r3: 0x%08x\r\n", trap->r0,trap->r1, trap->r2,trap->r3);
	trace_printf("ip: 0x%08x lr: 0x%08x pc: 0x%08x\r\n", trap->ip,trap->lr, trap->r2,trap->pc);
	trace_printf("xpsr: 0x%08x\r\n", trap->r2,trap->xpsr);
}

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

void dump_trapframe (trapframe *tf)
{
	trace_printf(" r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x  r4: 0x%08x\r\n", tf->r0,tf->r1, tf->r2,tf->r3,tf->r4);
	trace_printf(" r5: 0x%08x  r6: 0x%08x  r7: 0x%08x  r8: 0x%08x  r9: 0x%08x\r\n", tf->r5,tf->r6, tf->r7,tf->r8,tf->r9);
	trace_printf("r10: 0x%08x  fp: 0x%08x  ip: 0x%08x  lr: 0x%08x  pc: 0x%08x\r\n", tf->r10,tf->fp, tf->ip,tf->lr,tf->pc);
	trace_printf(" sp: 0x%08x  ret: 0x%08x  ", tf->sp, tf->ex_lr);
	char buf[5];
	buf[0] = tf->xpsr & PSR_N_BIT ? 'N' : 'n';
	buf[1] = tf->xpsr & PSR_Z_BIT ? 'Z' : 'z';
	buf[2] = tf->xpsr & PSR_C_BIT ? 'C' : 'c';
	buf[3] = tf->xpsr & PSR_V_BIT ? 'V' : 'v';
	buf[4] = '\0';
	trace_printf(" xpsr: %s\r\n", buf);
}
#define STACK_END ((uint32_t*)0x20050000)
void dump_stack_intresting_addresses(struct hw_trap *tf){
	uint32_t* stack = (uint32_t*)(tf+1);
	int pos = 0;
	trace_printf("intresting addresses from: 0x%08x to: 0x%08x\n",(uint32_t)stack,(uint32_t)STACK_END);
	while(stack < STACK_END){
		uint32_t value = *stack ;
		if(value > 0x80000000) {
			trace_printf("lr  0x%08x(%d): 0x%08x\n",((uint32_t)stack), pos, value);
		} else {
			trace_printf("    0x%08x(%d): 0x%08x\n",((uint32_t)stack), pos, value);
		}
		stack++;
		pos--;
	}
	trace_printf("end stack addresses %d\n",pos);
}
void dump_debug_trapframe (struct hw_trap *tf)
{
	uint32_t* stack = (uint32_t*)(tf+1);
	trace_printf(" r0: 0x%08x  r1: 0x%08x  r2: 0x%08x  r3: 0x%08x\r\n", tf->r0,tf->r1, tf->r2,tf->r3);
	trace_printf(" ip: 0x%08x  lr: 0x%08x  pc: 0x%08x  sp: 0x%08x ", tf->ip,tf->lr,tf->pc, (uint32_t)stack);
	char buf[5];
	buf[0] = tf->xpsr & PSR_N_BIT ? 'N' : 'n';
	buf[1] = tf->xpsr & PSR_Z_BIT ? 'Z' : 'z';
	buf[2] = tf->xpsr & PSR_C_BIT ? 'C' : 'c';
	buf[3] = tf->xpsr & PSR_V_BIT ? 'V' : 'v';
	buf[4] = '\0';
	trace_printf(" xpsr: %s\r\n", buf);
	// we need to dump the stack as well
	dump_stack_intresting_addresses(tf);
}
void DebugMon_Handler() {
	while(1); // forever hack
}
int do_systick(trapframe* tf){
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
	return 0;
}
void do_pendsw(trapframe* tf){
	// we are switching the context

}

int do_trap(int irq, trapframe* tf) {
	int ipsr;
	__asm volatile("mrs %0, ipsr" : "=r"(ipsr) : :);
	const struct enum_info* info = get_irq_info(irq);
	trace_printf ("     ispr: %d\n", ipsr);
	if(info) trace_printf ("     irq: %s\n", info->text);
	dump_trapframe(tf);
	while(1);
	return 0;
}

struct fault_info {
	int mask;
	const char* text;
	fault_info(int mask, const char* text) : mask(mask), text(text) {}
};
#define FAULT_TEXT(BIT,TEXT) fault_info((1<<(BIT)),(TEXT)),
//#define FAULT_TEXT(BIT,TEXT) { .mask = (1 <<(BIT)), .text = TEXT },
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
template<size_t N>
static void display_faults(uint32_t value, fault_info(&fault_list)[N]){
	static constexpr const char* comma_fmt = ",%s";
	static constexpr const char* no_comma_fmt = "%s";
	trace_printf("%x<",value);
	const char* fmt = no_comma_fmt;
	for(auto& f : fault_list) {
		if((f.mask & value)!=0) {
			trace_printf(fmt, f.text);
			fmt = comma_fmt;
		}
	}
	trace_printf(">");
}
#define for_each_fault(V,VAL, A) for(auto& V, A) if(V.mask & (VAL))
void display_faults(){
	uint32_t hfsr = SCB->HFSR;
	uint32_t cfsr = SCB->CFSR;

	uint32_t mem_fault = (cfsr & SCB_CFSR_MEMFAULTSR_Msk)>> SCB_CFSR_MEMFAULTSR_Pos;
	uint32_t bus_fault = (cfsr & SCB_CFSR_BUSFAULTSR_Msk)>> SCB_CFSR_BUSFAULTSR_Pos;
	uint32_t use_fault = (cfsr & SCB_CFSR_USGFAULTSR_Msk)>> SCB_CFSR_USGFAULTSR_Pos;

	trace_printf("     hfsr:");
	display_faults(hfsr,hard_fault_info);
	trace_printf("\r\n");

	trace_printf("     mmfsr:");
	display_faults(mem_fault,mem_fault_info);
	trace_printf("\r\n");

	trace_printf("     bfsr:");
	display_faults(bus_fault,bus_fault_info);
	trace_printf("\r\n");

	trace_printf("     ufsr:");
	display_faults(use_fault,use_fault_info);
	trace_printf("\r\n");

	trace_printf("     bar: %x\r\n<",SCB->BFAR);

}
struct irq_info{
	int exception;
	int irq;
	const char* text;
	irq_info(int exception, int irq, const char* text) : exception(exception), irq(irq), text(text) {}
};

static  irq_info irq_string[] = {
		irq_info(1,-15, "RESET"),
		irq_info(2,-14, "NMI"),
		irq_info(3,-13, "HARD FAULT"),
		irq_info(4,-12, "MPU FAULT"),
		irq_info(5,-11, "BUS FAULT"),
		irq_info(6,-10, "USE FAULT"),
		irq_info(11,-5, "SVC CALL"),
		irq_info(14,-2, "PENDSV"),
		irq_info(15,-1, "SYSTICK"),
};
static const char* find_exception_string(int ipsr) {
	for(auto& i : irq_string) if(ipsr == i.exception) return i.text;
	return "UNKONWN";
}
extern "C" void panic(const char*,...);

extern "C" void do_debug_default(int ipsr, struct hw_trap* tf) {
	trace_printf("\r\n\r\n");
	trace_printf("UNHANDED EXCEPTION (%d)%s\r\n", ipsr,find_exception_string(ipsr));
	dump_debug_trapframe(tf);
	display_faults();
	while(ipsr < 16);

	while(1);
}
extern "C" void do_default(int ipsr, trapframe* tf){
	dump_trapframe(tf);
	display_faults();
	panic("UNHANDED EXCEPTION (%d)%s\r\n", ipsr,find_exception_string(ipsr));

	while(1);
}
#if 0
extern "C"  __attribute__((naked)) void HardFault_Handler() {
	__asm volatile(
		    "tst lr, #4\n"
		    "ite eq\n"
		    "mrseq r1, msp\n"
		   	"mrsne r1, psp\n"
			"mrs r0, ipsr\n"
			"push {lr}\n"
			"bl =do_debug_default\n"
			"pop {lr}\n"
			"blx lr\n"
	);
	///volatile uint32_t* stack __asm("sp");

	//do_debug_default(3, (struct hw_trap*)(stack-1));
}
#endif
#if 0
__attribute__((naked)) void HardFault_Handler() {

}
__attribute__((naked)) void MemManage_Handler() {

}
__attribute__((naked)) void BusFault_Handler() {

}
__attribute__((naked)) void UsageFault_Handler() {

}
#endif
#if 0
void do_usagefault(context_t* tf){
	dump_trapframe(tf);
	display_faults();
	//show_trap_callstk("callstack!", tf);
	panic("usagefault");
	while(1);
}
#include "unwind.h"

void do_hardfault(struct hw_trap * tf){
	struct hwsw_trap trap;
	trap.hw = *tf;
	trap.sw = hard_fault_trap;
	dump_trapframe(&trap.ctx);
	display_faults();
	CliStack  results;
	UnwResult r;
	results.frameCount = 0;
	r = UnwindStartRet((uint32_t)(tf), tf->pc, &cliCallbacks, &results);
    for(int t = 0; t < results.frameCount; t++)
    {
        trace_printf("%c: 0x%08x\n",
               (results.address[t] & 0x1) ? 'T' : 'A', results.address[t]);
    }
    trace_printf("\nResult: %d\n", r);


	//show_trap_callstk("callstack!", tf);
	panic("hardfault");
	while(1);
}
void do_memmangefault(context_t* tf){
	dump_trapframe(tf);
	display_faults();
	//show_trap_callstk("callstack!", tf);
	while(1);
}


void do_busfault(context_t* tf){
	dump_trapframe(tf);
	display_faults();
	//show_trap_callstk("callstack!", tf);
	panic("busfault");
	while(1);
}

void            trap_init(void){

}
#endif
