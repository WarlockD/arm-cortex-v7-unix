/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include <stdint.h>
#include <stm32f7xx.h>
#include <stm32746g_discovery.h>

#include <string.h>
#include <stdio.h>
#include <os\printk.h>
#include <diag\Trace.h>



extern UART_HandleTypeDef huart1;
void MX_USART1_UART_Init(void);



void usart_sync_write(const uint8_t* data, size_t length) {
	while(HAL_BUSY!=HAL_UART_Transmit(&huart1, (uint8_t*)data, length,500));
}
void usart_async_write(const uint8_t* data, size_t length) {
	while(HAL_BUSY!=HAL_UART_Transmit_IT(&huart1, (uint8_t*)data, length));
}
void usart_sync_puts(const char* str){
	usart_sync_write((const uint8_t*)str,strlen(str));
}
void usart_async_puts(const char* str){
	usart_async_write((const uint8_t*)str,strlen(str));
}
void mark3_init();
int main(void)
{
	// evething has be rebuilt so that __initialize_hardware starts it off
	//  DBGMCU->CR = 0x00000020;
	//  ITM->TCR |= ITM_TCR_ITMENA_Msk;
	//  ITM->TER |= 1; /* ITM Port #0 enabled */
	  trace_printf("Clock &d, Compiled at %s\r\n", SystemCoreClock, __TIME__);
	  mark3_init();

	 // semi_str("semihostin\r\n");

	  while(1) {
		//  trace_printf("Trace is working! wee!\n");
		  printk("SERIAL output %i\r\n",7);
		  HAL_Delay(1000);
	  }
#if 0
	  LCD_Config();

	  LCD_LOG_Init();

	  LCD_ErrLog("I hope this works %s\n", "shit");
#endif
	  scmrtos_test_start();
	  while(1);

	for(;;);
}


#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
#if 0
void __assert_func (const char* filename, int lineno, const char* func, const char* val) {
	printk("%d:%s: %s - %s\r\n",lineno, filename,func,val);
	while(1);
}

void assert_failed(uint8_t* file, uint32_t line)
{
	printk("assert_failed: file %s on line %d\r\n", file, line);
	while(1);
}
void up_assert(const uint8_t *filename, int linenum)  {
	printk("assert_failed: file %s on line %d\r\n", filename, linenum);
	while(1);
}
#endif
#endif
