#include "os.h"

typedef enum {
	OS_TASK_STATUS_IDLE = 1,
	OS_TASK_STATUS_ACTIVE,
} os_task_status_t;

typedef struct {
	/* The stack pointer (sp) has to be the first element as it is located
	   at the same address as the structure itself (which makes it possible
	   to locate it safely from assembly implementation of PendSV_Handler).
	   The compiler might add padding between other structure elements. */
	volatile uint32_t sp;
	void (*handler)(void *p_params);
	void *p_params;
	volatile os_task_status_t status;
} os_task_t;

static enum {
	OS_STATE_DEFAULT = 1,
	OS_STATE_INITIALIZED,
	OS_STATE_TASKS_INITIALIZED,
	OS_STATE_STARTED,
} m_state = OS_STATE_DEFAULT;

static struct {
	os_task_t tasks[OS_CONFIG_MAX_TASKS];
	volatile uint32_t current_task;
	uint32_t size;
} m_task_table;

volatile os_task_t *os_curr_task;
volatile os_task_t *os_next_task;

static void task_finished(void)
{
	/* This function is called when some task handler returns. */
	OS_ERROR_HANDLER(OS_ERROR_TASK_FINISHED);

	volatile uint32_t i = 0;
	while (1)
		i++;
}

os_error_t os_init(void)
{
	if (m_state != OS_STATE_DEFAULT)
		return OS_ERROR_WRONG_STATE;

	memset(&m_task_table, 0, sizeof(m_task_table));
	m_state = OS_STATE_INITIALIZED;

	return OS_ERROR_OK;
}

os_error_t os_task_init(void (*handler)(void *p_params), void *p_task_params,
			os_stack_t *p_stack, size_t stack_size)
{
	if (m_state != OS_STATE_INITIALIZED &&
	    m_state != OS_STATE_TASKS_INITIALIZED)
		return OS_ERROR_WRONG_STATE;

	if (m_task_table.size >= OS_CONFIG_MAX_TASKS-1)
		return OS_ERROR_NO_MEM;

	/* Initialize the task structure and set SP to the top of the stack
	   minus 16 words (64 bytes) to leave space for storing 16 registers: */
	os_task_t *p_task = &m_task_table.tasks[m_task_table.size];
	p_task->handler = handler;
	p_task->p_params = p_task_params;
	p_task->sp = (uint32_t)(p_stack+stack_size-16);
	p_task->status = OS_TASK_STATUS_IDLE;

	/* Save init. values of registers which will be restored on exc. return:
	   - XPSR: Default value (0x01000000)
	   - PC: Point to the handler function
	   - LR: Point to a function to be called when the handler returns
	   - R0: Point to the handler function's parameter */
	p_stack[stack_size-1] = 0x01000000;
	p_stack[stack_size-2] = (uint32_t)handler;
	p_stack[stack_size-3] = (uint32_t)&task_finished;
	p_stack[stack_size-8] = (uint32_t)p_task_params;

#ifdef OS_CONFIG_DEBUG
	uint32_t base = (m_task_table.size+1)*1000;
	p_stack[stack_size-4] = base+12;  /* R12 */
	p_stack[stack_size-5] = base+3;   /* R3  */
	p_stack[stack_size-6] = base+2;   /* R2  */
	p_stack[stack_size-7] = base+1;   /* R1  */
	/* p_stack[stack_size-8] is R0 */
	p_stack[stack_size-9] = base+7;   /* R7  */
	p_stack[stack_size-10] = base+6;  /* R6  */
	p_stack[stack_size-11] = base+5;  /* R5  */
	p_stack[stack_size-12] = base+4;  /* R4  */
	p_stack[stack_size-13] = base+11; /* R11 */
	p_stack[stack_size-14] = base+10; /* R10 */
	p_stack[stack_size-15] = base+9;  /* R9  */
	p_stack[stack_size-16] = base+8;  /* R8  */
#endif

	m_state = OS_STATE_TASKS_INITIALIZED;
	m_task_table.size++;

	return OS_ERROR_OK;
}
static bool os_running = false;
os_error_t os_start(uint32_t systick_ticks)
{
	if (m_state != OS_STATE_TASKS_INITIALIZED)
		return OS_ERROR_WRONG_STATE;
	  /**Configure the Systick interrupt time
	  */

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
	NVIC_SetPriority(PendSV_IRQn, 0xff); /* Lowest possible priority */
	NVIC_SetPriority(SysTick_IRQn, 0x00); /* Highest possible priority */

	/* Start the SysTick timer: */
	//uint32_t ret_val = SysTick_Config(systick_ticks);
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	  /**Configure the Systick
	  */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
#if 0
	if (ret_val != 0)
		return OS_ERROR_INVALID_PARAM;
#endif
	/* Start the first task: */
	os_curr_task = &m_task_table.tasks[m_task_table.current_task];


	__set_PSP(os_curr_task->sp+64); /* Set PSP to the top of task's stack */
	__set_CONTROL(0x03); /* Switch to Unprivilleged Thread Mode with PSP */
	__ISB(); /* Execute ISB after changing CONTORL (recommended) */
	m_state = OS_STATE_STARTED;
	os_running = true;
	os_curr_task->handler(os_curr_task->p_params);

	return OS_ERROR_OK;
}
void osSystickHandler22()
//void SysTick_Handler(void)
{
	if(m_state == OS_STATE_STARTED) return;
	os_curr_task = &m_task_table.tasks[m_task_table.current_task];
	os_curr_task->status = OS_TASK_STATUS_IDLE;

	/* Select next task: */
	m_task_table.current_task++;
	if (m_task_table.current_task >= m_task_table.size)
		m_task_table.current_task = 0;

	os_next_task = &m_task_table.tasks[m_task_table.current_task];
	os_next_task->status = OS_TASK_STATUS_ACTIVE;
//	CallKernel(5);
	/* Trigger PendSV which performs the actual context switch: */
	//SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
