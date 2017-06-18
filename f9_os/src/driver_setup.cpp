
#include "mark3/drivers/tracedriver.hpp"
#include "kernel.h"
#include <stm32f7xx.h>
#include <os\printk.h>

static mark3::TraceDriver clUART; 	//!< UART device driver object

extern "C" {
int uart_putc(int c) {
	while(	READ_BIT(USART1->ISR, USART_ISR_TXE) == 0);
	USART1->TDR = c & 0xFF;
	while(	READ_BIT(USART1->ISR, USART_ISR_TC) == 0);
	return c & 0xFF;
}
int usart_getc() {
	volatile uint32_t c = 0;
	if(READ_BIT(USART1->ISR,USART_ISR_ORE)) {
		c= 0x100; // overrun error
		USART1->ICR = USART_ICR_ORECF;// clear all errors
	}
	if(READ_BIT(USART1->ISR, USART_ISR_RXNE) == 0)
		c |= 0x8000000;
	else
		c = USART1->RDR;
	return c;
}
int driver_putc(int c) {
	clUART.Write(1,(uint8_t*)&c);
	return c;
}
int driver_writec(const char* s, size_t n) {
	return 	clUART.Write(n,(uint8_t*)s);

}
#include <cstring>

void debug_string(const char* s) {
	mark3::_trace_write_semihosting_debug(s,strlen(s));
}
}
#include <cassert>

void mark3_driver_setup() {
    clUART.SetName("/dev/tty");			//!< Add the serial driver
    clUART.Init(mark3::TraceMode::OpenOCD_Semihosting_Debug);
				//!< MUST be before other kernel ops
    DriverList::Add( &clUART );
    clUART.Open();
    debug_string("mark3_init: done\r\n");
    printk_setup(mark3::_trace_write_semihosting_debug, SERIAL_OPTIONS);
}

extern "C" void mark3_init() {
   // Kernel::Init();
    mark3_driver_setup();


}
