
#include "stm32f7_uart.hpp"

//UART_HandleTypeDef huart1;
//DMA_HandleTypeDef hdma_usart1_tx;
#if 0
int uartputc(int c) {
	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE) == 0);
	huart1.Instance->TDR = c & 0xFF;
	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == 0);
	return c & 0xFF;
}
#endif
#if 0
static os::device::stm32f7_uart usart1(1);

extern "C" void USART1_IRQHandler() {
	os::device::stm32f7_uart::isr_handler(usart1);
}

#endif



