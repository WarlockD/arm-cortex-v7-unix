#include <diag\Trace.h>
#include <os\atomic.h>

#include <stdarg.h>
#include <string.h>

#include <stm32f7xx.h>
#include <lib\queue.h>
#include <os\conf.h>
#include <assert.h>
extern UART_HandleTypeDef huart1;
#define khuart huart1
//static UART_HandleTypeDef khuart;
#define SEND_BUFSIZE    2048
#define RECV_BUFSIZE    32
#define USART_PIROITY   3
enum {
	DBG_ASYNC,
	DBG_PANIC
} dbg_state;

static uint8_t dbg_uart_tx_buffer[SEND_BUFSIZE];
static uint8_t dbg_uart_rx_buffer[RECV_BUFSIZE];
static volatile bool uart_trasmitting = false;
static struct {
	USART_TypeDef* usart;
	uint32_t status;
	bool newline_sent;
	queue_t rx;
	queue_t tx;
} dbg_uart;

#define WAIT_FLAG(FLAG,MASK) while(((FLAG) & (MASK))!=(MASK)) {}
static void raw_putc(uint8_t c) {
	WAIT_FLAG(khuart.Instance->ISR, USART_ISR_TXE);
	//while((khuart.Instance->ISR & USART_ISR_TXE)!=USART_ISR_TXE);
	USART1->TDR = c;
}
#define __l4_putchar raw_putc

void _putchar(char c) {
	raw_putc(c) ;
}

static void raw_puts(const char* str) {
	while(*str) {
		uint8_t c = *str++;
		if(c == '\r') continue;
		if(c == '\n') raw_putc('\r');
		raw_putc(c);
	}
	WAIT_FLAG(khuart.Instance->ISR, USART_ISR_TC|USART_ISR_TXE);
//	while((khuart.Instance->ISR & (USART_ISR_TC&USART_ISR_TXE))!=(USART_ISR_TC&USART_ISR_TXE));
}
static void raw_write(const uint8_t* str,size_t len) {
	while(--len) {
		uint8_t c = *str++;
		if(c == '\r') continue;
		if(c == '\n') raw_putc('\r');
		raw_putc(c);
	}
	while((khuart.Instance->ISR & USART_ISR_TC)!=0);
}
static bool raw_deque() {
	if (queue_is_empty(&(dbg_uart.tx))) return false;
	uint32_t flags = enter_critical();
	uint8_t chr;
	queue_pop(&(dbg_uart.tx), &chr);
	USART1->TDR = chr;
	exit_critical(flags);
	return true;
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart){
	if(huart == &khuart){
		uart_trasmitting = false;
	}

}
void USART1_IRQHandler() //__uart_irq_handler(void)
{
	  uint32_t isrflags   = READ_REG(khuart.Instance->ISR);
	  uint32_t cr1its     = READ_REG(khuart.Instance->CR1);
//	  uint32_t cr3its     = READ_REG(khuart.Instance->CR3);
	  uint32_t errorflags;
	  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
	  if(errorflags == RESET && ((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET)){// no error
		    	volatile uint8_t chr = USART1->RDR;
		    	// get the char and bypass the handler
		    	queue_push(&(dbg_uart.rx), chr);
	  } else {
			HAL_UART_IRQHandler(&khuart);
	  }
}

uint8_t dbg_uart_getchar(void)
{
	uint8_t chr = 0;
	if (queue_pop(&(dbg_uart.rx), &chr) == QUEUE_EMPTY)
		return 0;
	return chr;

}

static HAL_StatusTypeDef raw_usart_write(const uint8_t* data, size_t len){
	HAL_StatusTypeDef status;
	switch(dbg_uart.status){
	case DBG_PANIC:
		status=HAL_UART_Transmit(&khuart, (uint8_t*)data, len,500);
		break;
	case DBG_ASYNC:
		uart_trasmitting = true;
		status= HAL_UART_Transmit_IT(&khuart, (uint8_t*)data, len);
		break;
	default:assert(0); break;
	}

	return status;
}
// assume we are already in a critical
static void _writek(const uint8_t* data, size_t len) {
	while(uart_trasmitting || (raw_usart_write(data,len) == HAL_BUSY)){
		restore_flags(USART_PIROITY);
		__WFI(); // wait for an interrupt
	}
}
void writek(const uint8_t* data, size_t len){
	uint32_t flags = enter_critical();
	_writek(data,len);
	exit_critical(flags);
}
void putsk(const char* str){
	writek((const uint8_t*)str,strlen(str));
}

void __l4_vprintf(const char *fmt, va_list va);
static void  vtrace_printf(const char* fmt, va_list va){
	__l4_vprintf(fmt,va);
	//static char buffer[128];
	//int len = vsnprintf(buffer,127,fmt,va);
	//raw_write(buffer,len);
#if 0
	if(dbg_uart.status == DBG_PANIC){
		HAL_UART_Transmit(&khuart, (uint8_t*)buffer, len,500);
	}else
		_writek((uint8_t*)buffer,len);
#endif
}
void init_console(){
	// should be already configured
	khuart.Instance = USART1;
	khuart.Init.BaudRate = 115200;
	khuart.Init.WordLength = UART_WORDLENGTH_8B;
	khuart.Init.StopBits = UART_STOPBITS_1;
	khuart.Init.Parity = UART_PARITY_NONE;
	khuart.Init.Mode = UART_MODE_TX_RX;
	khuart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	khuart.Init.OverSampling = UART_OVERSAMPLING_16;
	khuart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	khuart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(&huart1);

	HAL_NVIC_SetPriority(USART1_IRQn, USART_PIROITY, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	//usart_init(&console_uart);
	queue_init(&(dbg_uart.tx), dbg_uart_tx_buffer, SEND_BUFSIZE);
	queue_init(&(dbg_uart.rx), dbg_uart_rx_buffer, RECV_BUFSIZE);
	//dbg_state = DBG_ASYNC;
	//dbg_uart.status == DBG_PANIC;
}
void clear_console_screen() {
	raw_puts("\033[2J\033[1;1H");
	//putsk("\033[2J\033[1;1H");
}
void panic(const char* fmt, ...){
	while(uart_trasmitting) __WFI();
	uint32_t flags = enter_critical();
	dbg_uart.status = DBG_PANIC;
	va_list va;
	va_start(va, fmt);
	vtrace_printf(fmt, va);
	va_end(va);
	exit_critical(flags);
}
void trace_printf(const char* fmt, ...)
{
	uint32_t flags = enter_critical();
	va_list va;
	va_start(va, fmt);
	vtrace_printf(fmt, va);
	va_end(va);
	exit_critical(flags);
}
void __l4_puts(char *str)
{
	while (*str) {
		if (*str == '\n')
			__l4_putchar('\r');
		__l4_putchar(*(str++));
	}
}

static void __l4_puts_x(char *str, int width, const char pad)
{
	while (*str) {
		if (*str == '\n')
			__l4_putchar('\r');
		__l4_putchar(*(str++));
		--width;
	}

	while (width > 0) {
		__l4_putchar(pad);
		--width;
	}
}

#define hexchars(x)			\
	(((x) < 10) ?			\
		('0' + (x)) :		\
	 	('a' + ((x) - 10)))

static int __l4_put_hex(const uint32_t val, int width, const char pad)
{
	int i, n = 0;
	int nwidth = 0;

	/* Find width of hexnumber */
	while ((val >> (4 * nwidth)) && ((unsigned) nwidth <  2 * sizeof(val)))
		nwidth++;
	if (nwidth == 0)
		nwidth = 1;

	/* May need to increase number of printed characters */
	if (width == 0 && width < nwidth)
		width = nwidth;

	/* Print number with padding */
	for (i = width - nwidth; i > 0; i--, n++)
		__l4_putchar(pad);
	for (i = 4 * (nwidth - 1); i >= 0; i -= 4, n++)
		__l4_putchar(hexchars((val >> i) & 0xF));

	return n;
}

static void __l4_put_dec(const uint32_t val, char sign, const int width, const char pad)
{
	uint32_t divisor;
	int digits;
	char neg = sign && ((int)val) < 0;

	/* estimate number of spaces and digits */
	for (divisor = 1, digits = 1; val / divisor >= 10; divisor *= 10, digits++)
		/* */ ;
	if(neg) digits++;
	/* print spaces */
	for (; digits < width; digits++)
		__l4_putchar(pad);

	/* print digits */
	if(neg) __l4_putchar('-');
	do {
		__l4_putchar(((val / divisor) % 10) + '0');
	} while (divisor /= 10);
}


void __l4_printf(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	__l4_vprintf(fmt, va);
	va_end(va);
}

void __l4_vprintf(const char *fmt, va_list va)
{
	int mode = 0;	/* 0: usual char; 1: specifiers */
	int width = 0;
	char pad = ' ';
	int size = 16;

	while (*fmt) {
		if (*fmt == '%') {
			mode = 1;
			pad = ' ';
			width = 0;
			size = 32;

			fmt++;
			continue;
		}

		if (!mode) {
			if (*fmt == '\n')
				__l4_putchar('\r');
			__l4_putchar(*fmt);
		} else {
			switch (*fmt) {
			case 'c':
				__l4_putchar(va_arg(va, uint32_t));
				mode = 0;
				break;
			case 's':
				__l4_puts_x(va_arg(va, char *), width, pad);
				mode = 0;
				break;
			case 'l':
			case 'L':
				size = 64;
				break;
			case 'u':
				__l4_put_dec(va_arg(va, unsigned),0,
				             width, pad);
				mode = 0;
				break;
			case 'i':
				__l4_put_dec(va_arg(va, int), 1,
				             width, pad);
				mode = 0;
				break;
			case 'd':
			case 'D':
				__l4_put_dec((size == 32) ?
				             va_arg(va, uint32_t) :
				             va_arg(va, uint64_t),1,
				             width, pad);
				mode = 0;
				break;
			case 'p':
			case 't':
				size = 32;
				width = 8;
				pad = '0';
				// no break
			case 'x':
			case 'X':
				__l4_put_hex((size == 32) ?
				             va_arg(va, uint32_t) :
				             va_arg(va, uint64_t),
				             width, pad);
				mode = 0;
				break;
			case '%':
				__l4_putchar('%');
				mode = 0;
				break;
			case '0':
				if (!width)
					pad = '0';
				break;
			case ' ':
				pad = ' ';
			}

			if (*fmt >= '0' && *fmt <= '9') {
				width = width * 10 + (*fmt - '0');
			}
		}

		fmt++;
	}
}

