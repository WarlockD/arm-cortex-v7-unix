
#include <os\irq.h>
#include <cassert>

#include <diag\Trace.h>

typedef void (* const pHandler)(void);
//__attribute__ ((section(".isr_vector"),used))
extern pHandler __isr_vectors[];

// ----------------------------


extern ram_handler_t __ram_vectors_start[];
extern ram_handler_t __ram_vectors_end[];
static size_t g_isr_count = 0;

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

static ram_handler_t& get_isr(IRQn_Type isr){
	uint32_t pos = (uint32_t)(isr);
	pos+=16;
	assert(pos < g_isr_count);
	if(isr >= 0) {
		NVIC_DisableIRQ(isr);
		NVIC_ClearPendingIRQ(isr);
	}
	return __ram_vectors_start[pos];
}

extern "C" void install_isr(IRQn_Type isr, ram_handler_t handler) {
	uint32_t flags = irq_critical_begin();
	if(SCB->VTOR != (uint32_t)__ram_vectors_start){
		__ISB();
		__DSB();
		volatile SCB_Type* scb = SCB;
		scb->VTOR = (uint32_t)__ram_vectors_start;
		assert(SCB->VTOR == (uint32_t)__ram_vectors_start); // sanity check, has to be alligned 256?
		g_isr_count = ((size_t)(__ram_vectors_end - __ram_vectors_start));
		for(size_t i=0; i < g_isr_count;i++)
			__ram_vectors_start[i] = __isr_vectors[i];
	}
	uint32_t pos = (uint32_t)(isr);
	pos+=16;
	assert(pos < g_isr_count);
	if(isr >= 0) {
		NVIC_DisableIRQ(isr);
		NVIC_ClearPendingIRQ(isr);
	}
	if(handler == NULL)
		__ram_vectors_start[pos] = __isr_vectors[pos];
	else
		__ram_vectors_start[pos] = handler;
	irq_critical_end(flags);
}
#else
extern "C" void install_isr(IRQn_Type isr, ram_handler_t handler) {
	(void)isr; (void)handler;
	assert(0); // humm
}

#endif
// cheap way to get the asembler to do my work for me
static void handle_imp() __attribute__((naked,used));
static void handle_imp() {
	__asm volatile (
		"ldr r0, [pc, #2]\n"
		"ldr pc, [pc, #4]\n"
		"nop \n" // filler so we are aligned, and this is two words
		" .word 0x00000000\n" //_thisis\n"
		" .word 0x00000000\n"
	:::);
}

static void handle_imp2() __attribute__((naked,used));
static void handle_imp2() {
	__asm volatile (
		"ldr r0, _thisis\n"			//  4803: one shorts
		"ldr r1, _thisis_member\n"     // 	f8df f010 : two shorts
//		"bkpt 0\n"
		"bx r1 \n"
		"nop \n" // filler so we are aligned, and this is two words
		// 4801 4902 4760 bf00
		"_thisis: .word 0x12345678\n"
		"_thisis_member: .word 0x87654321\n"
		"_irq_func_imp_size: .word . - _irq_func_imp_begin\n"
	:::);
#if 0
	08003b40 <_ZL11handle_imp2v>:
	 8003b40:	4801      	ldr	r0, [pc, #4]	; (8003b48 <_thisis>)
	 8003b42:	4902      	ldr	r1, [pc, #8]	; (8003b4c <_thisis_member>)
	 8003b44:	4760      	bx	ip
	 8003b46:	bf00      	nop

	08003b48 <_thisis>:
	 8003b48:	12345678 	eorsne	r5, r4, #120, 12	; 0x7800000

	08003b4c <_thisis_member>:
	 8003b4c:	87654321 	strbhi	r4, [r5, -r1, lsr #6]!
#endif
}
struct handler_impl_t {
		uint16_t code[4];
		uint32_t r0_value;
		uint32_t pc_value;
};

static constexpr uint16_t g_callback_code[] = { 0x4801, 0x4902, 0x4708, 0xbe00 };
// breakpoint is in here
static constexpr uint16_t g_debug_callback_code[] = { 0x4801, 0x4902, 0xbe00, 0x4708 };


extern const size_t  _irq_func_imp_size;

namespace os {
	namespace priv {
		 void g_setup_callback(volatile uint16_t* code, uint32_t pc, uint32_t arg) {
		//const uint16_t*  g_handler_code = (const uint16_t*)&handle_imp;
			// side not, c returns the function pointer offset because its thumb code, oooooh
		const uint16_t*  g_handler_code = (const uint16_t*)((uintptr_t)handle_imp2 & ~1);
		trace::printn("function stuff ", g_handler_code, g_callback_code, _irq_func_imp_size);
		trace::printn("imp2           ", std::make_pair(g_handler_code,(size_t)4));
		trace::printn("manual         ", std::make_pair(g_callback_code,(size_t)4));

		for(int i=0;i < 4; i++) {
			// testing
			//assert(g_handler_code[i] == g_callback_code[i]);
			code[i] = g_callback_code[i];
		//	code[i] = g_handler_code[i];
			//code[i] = g_debug_callback_code[i];
		}
		//std::copy(g_handler_code,g_handler_code + 4,&code[0]);


		*((volatile uint32_t*)&code[4]) = arg;
		*((volatile uint32_t*)&code[6]) = pc;
	}
	}
}
