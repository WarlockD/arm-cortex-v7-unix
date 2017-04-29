// BSP support routine
#include "types.h"
#include "defs.h"
#include "param.h"
#include "proc.h"

#include "mpu.h"
#include <os\arch.h>
#include "arm.h"
//#include <stm32f7xx.h>

void cli () { __disable_irq(); }
void sti () { __enable_irq(); }

// return whether interrupt is currently enabled
int int_enabled () { return __get_PRIMASK(); }
// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.
// switch hack

void pushcli (void)
{
    int enabled = int_enabled();
    cli();
    if (cpu->ncli++ == 0) cpu->intena = enabled;
}

void popcli (void)
{
    if (int_enabled())panic("popcli - interruptible");
    if (--cpu->ncli < 0) {
        cprintf("cpu (%d)->ncli: %d\n", cpu, cpu->ncli);
        panic("popcli -- ncli < 0");
    }
    if ((cpu->ncli == 0) && cpu->intena) sti();
}

// Record the current call stack in pcs[] by following the call chain.
// In ARM ABI, the function prologue is as:
//		push	{fp, lr}
//		add		fp, sp, #4
// so, fp points to lr, the return address
void getcallerpcs (void * v, uint32_t pcs[])
{
    uint32_t *fp;
    int i;

    fp = (uint32_t*) v;

    for (i = 0; i < N_CALLSTK; i++) {
        if ((fp == 0) || (fp < (uint32_t*) KERNBASE) || (fp == (uint32_t*) 0xffffffff)) {
            break;
        }

        fp = fp - 1;			// points fp to the saved fp
        pcs[i] = fp[1];     // saved lr
        fp = (uint32_t*) fp[0];	// saved fp
    }

    for (; i < N_CALLSTK; i++) {
        pcs[i] = 0;
    }
}
void manual_callstack(uintptr_t fp) {
	if(fp == 0 || fp < KERNBASE) return;
	uint32_t pcs[N_CALLSTK];
    getcallerpcs((void*)fp, pcs);

    for (int i = N_CALLSTK - 1; i >= 0; i--) {
    	if(pcs[i] == 0) break;
         cprintf("%d: 0x%x\n", i + 1, pcs[i]);
    }
}
void show_callstk (char *s)
{
    uint32_t fp;

    cprintf("%s\n", s);
    __asm volatile("mov %0, fp" : "=r"(fp) : :);
    manual_callstack(fp);
}
void show_trap_callstk (const char *s, struct trapframe* tf)
{
	cprintf("%s\n", s);
    manual_callstack(tf->fp);
}

