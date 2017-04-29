
//#include <os\printk.h>
#include <assert.h>
#include "asm.h"
#define __KERNEL__
#include "em\io.h"
#include "em\ptrace.h"
#include "em\kallsyms.h"
static const char *processor_modes[] = {
  "USER_26", "FIQ_26" , "IRQ_26" , "SVC_26" , "UK4_26" , "UK5_26" , "UK6_26" , "UK7_26" ,
  "UK8_26" , "UK9_26" , "UK10_26", "UK11_26", "UK12_26", "UK13_26", "UK14_26", "UK15_26",
  "USER_32", "FIQ_32" , "IRQ_32" , "SVC_32" , "UK4_32" , "UK5_32" , "UK6_32" , "ABT_32" ,
  "UK8_32" , "UK9_32" , "UK10_32", "UND_32" , "UK12_32", "UK13_32", "UK14_32", "SYS_32"
};
#define PM_NUM	(sizeof(processor_modes)/sizeof(processor_modes[0]))

static const char *isa_modes[] = {
  "ARM" , "Thumb" , "Jazelle", "ThumbEE"
};
#define ISA_NUM	(sizeof(isa_modes)/sizeof(isa_modes[0]))

static volatile int hlt_counter=0;

struct nvic_regs {
	unsigned long	some_regs1[837];	/* +000 */
	unsigned long	config_control;		/* +d14 */
	unsigned long	some_regs2[3];		/* +d18 */
	unsigned long	system_handler_csr;	/* +d24 */
	unsigned long	local_fault_status;	/* +d28 */
	unsigned long	hard_fault_status;	/* +d2c */
	unsigned long	some_regs3[1];		/* +d30 */
	unsigned long	mfar;			/* +d34 */
	unsigned long	bfar;			/* +d38 */
	unsigned long	some_regs4[21];		/* +d3c */
	unsigned long	mpu_type;               /* +d90 */
	unsigned long	mpu_control;		/* +d94 */
	unsigned long	reg_number;		/* +d98 */
	unsigned long	reg_base;		/* +d9c */
	unsigned long	reg_attr;		/* +da0 */
};

#define NVIC_REGS_BASE  0xE000E000
#define NVIC		((struct nvic_regs *)(NVIC_REGS_BASE))

enum config_control_bits {
	UNALIGN_TRP	= 0x00000008,
	DIV_0_TRP	= 0x00000010,
};

enum system_handler_csr_bits {
	BUSFAULTENA	= 0x00020000,
	USGFAULTENA	= 0x00040000,
};

enum local_fault_status_bits {
	IBUSERR		= 0x00000100,
	PRECISERR	= 0x00000200,
	IMPRECISERR	= 0x00000400,
	UNSTKERR	= 0x00000800,
	STKERR		= 0x00001000,
	BFARVALID	= 0x00008000,
	UNDEFINSTR	= 0x00010000,
	INVSTATE	= 0x00020000,
	INVPC		= 0x00040000,
	NOCP		= 0x00080000,
	UNALIGNED	= 0x01000000,
	DIVBYZERO	= 0x02000000,
};

enum hard_fault_status_bits {
	VECTTBL		= 0x00000002,
	FORCED		= 0x40000000,
};

enum interrupts {
	HARDFAULT	= 3,
	BUSFAULT	= 5,
	USAGEFAULT	= 6,
};

struct traps {
	char	*name;
	int	test_bit;
	int	handler;
};

static struct traps traps[] = {
	{"Vector Read error",		VECTTBL,	HARDFAULT},
	{"uCode stack push error",	STKERR,		BUSFAULT},
	{"uCode stack pop error",	UNSTKERR,	BUSFAULT},
	{"Escalated to Hard Fault",	FORCED,		HARDFAULT},
	{"Pre-fetch error",		IBUSERR,	BUSFAULT},
	{"Precise data bus error",	PRECISERR,	BUSFAULT},
	{"Imprecise data bus error",	IMPRECISERR,	BUSFAULT},
	{"No Coprocessor",		NOCP,		USAGEFAULT},
	{"Undefined Instruction",	UNDEFINSTR,	USAGEFAULT},
	{"Invalid ISA state",		INVSTATE,	USAGEFAULT},
	{"Return to invalid PC",	INVPC,		USAGEFAULT},
	{"Illegal unaligned access",	UNALIGNED,	USAGEFAULT},
	{"Divide By 0",			DIVBYZERO,	USAGEFAULT},
	{NULL}
};

void __show_regs(struct pt_regs *regs)
{
	unsigned long flags;
	char buf[64];

	//printk("CPU: %d    %s  (%s %.*s)\n",
	//	raw_smp_processor_id(), print_tainted(),
	//	init_utsname()->release,
	//	(int)strcspn(init_utsname()->version, " "),
	//	init_utsname()->version);
	print_symbol("PC is at %s\n", instruction_pointer(regs));
	print_symbol("LR is at %s\n", regs->ARM_lr);
	printk("pc : [<%08lx>]    lr : [<%08lx>]    psr: %08lx\n"
	       "sp : %08lx  ip : %08lx  fp : %08lx\n",
		regs->ARM_pc, regs->ARM_lr, regs->ARM_cpsr,
		regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	{
		int i;
		int *ptr = (int *)regs->ARM_pc;
		printk("Code dump at pc [%08lx]:\n", regs->ARM_pc);
		for (i = 0; i < 4; i ++, ptr++) {
			printk("%08x ", *ptr);
		}
		printk("\n");
	}
	printk("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9,
		regs->ARM_r8);
	printk("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6,
		regs->ARM_r5, regs->ARM_r4);
	printk("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2,
		regs->ARM_r1, regs->ARM_r0);

	flags = regs->ARM_cpsr;
	buf[0] = flags & PSR_N_BIT ? 'N' : 'n';
	buf[1] = flags & PSR_Z_BIT ? 'Z' : 'z';
	buf[2] = flags & PSR_C_BIT ? 'C' : 'c';
	buf[3] = flags & PSR_V_BIT ? 'V' : 'v';
	buf[4] = '\0';

	printk("Flags: %s  IRQs o%s  FIQs o%s  Mode %s  ISA %s  Segment %s\n",
		buf, interrupts_enabled(regs) ? "n" : "ff",
		fast_interrupts_enabled(regs) ? "n" : "ff",
		processor_mode(regs) < PM_NUM ?
			processor_modes[processor_mode(regs)] : "unknown",
		isa_mode(regs) < ISA_NUM ?
			isa_modes[isa_mode(regs)] : "unknown",
		get_fs() == get_ds() ? "kernel" : "user");
#ifdef CONFIG_CPU_CP15
	{
		unsigned int ctrl;

		buf[0] = '\0';
#ifdef CONFIG_CPU_CP15_MMU
		{
			unsigned int transbase, dac;
			asm("mrc p15, 0, %0, c2, c0\n\t"
			    "mrc p15, 0, %1, c3, c0\n"
			    : "=r" (transbase), "=r" (dac));
			snprintf(buf, sizeof(buf), "  Table: %08x  DAC: %08x",
			  	transbase, dac);
		}
#endif
		asm("mrc p15, 0, %0, c1, c0\n" : "=r" (ctrl));

		printk("Control: %08x%s\n", ctrl, buf);
	}
#endif
}

void show_regs(struct pt_regs * regs)
{
	printk("\n");
	//printk("Pid: %d, comm: %20s\n", task_pid_nr(current), current->comm);
	__show_regs(regs);
//	__backtrace();
}

/*
 * The function initializes Bus fault and Usage fault exceptions,
 * forbids unaligned data access and division by 0.
 */
void traps_v7m_init(void){
	writel(readl(&NVIC->system_handler_csr) | USGFAULTENA | BUSFAULTENA,
			&NVIC->system_handler_csr);
	writel(
#ifndef CONFIG_ARM_V7M_NO_UNALIGN_TRP
		UNALIGN_TRP |
#endif
		DIV_0_TRP | readl(&NVIC->config_control),
			&NVIC->config_control);
}

/*
 * The function prints information about the reason of the exception
 * @param name		name of current executable (process or kernel)
 * @param regs		state of registers when the exception occurs
 * @param in		IPSR, the number of the exception
 * @param addr		address caused the interrupt, or current pc
 * @param hstatus	status register for hard fault
 * @param lstatus	status register for local fault
 */
static void traps_v7m_print_message(char *name, struct pt_regs *regs,
		enum interrupts in, unsigned long addr,
		unsigned long hstatus, unsigned long lstatus)
{
	int i;
	printk("\n\n%s: fault at 0x%08lx [pc=0x%08lx, sp=0x%08lx]\n",
			name, addr, instruction_pointer(regs), regs->ARM_sp);
	for (i = 0; traps[i].name != NULL; ++i) {
		if ((traps[i].handler == HARDFAULT ? hstatus : lstatus)
				& traps[i].test_bit) {
			printk("%s\n", traps[i].name);
		}
	}
	printk("\n");
}

/*
 * Common routine for high-level exception handlers.
 * @param regs		state of registers when the exception occurs
 * @param in		IPSR, the number of the exception
 */
void traps_v7m_common(struct pt_regs *regs, enum interrupts in)
{
	unsigned long hstatus;
	unsigned long lstatus;
	unsigned long addr;

	hstatus = readl(&NVIC->hard_fault_status);
	lstatus = readl(&NVIC->local_fault_status);
	traps_v7m_print_message("KERNEL", regs, in, addr,
			hstatus, lstatus);
	show_regs(regs);
	if (lstatus & BFARVALID && (in == BUSFAULT ||
			(in == HARDFAULT && (hstatus & FORCED)))) {
		addr = readl(&NVIC->bfar);
	} else {
		addr = instruction_pointer(regs);
	}

	writel(hstatus, &NVIC->hard_fault_status);
	writel(lstatus, &NVIC->local_fault_status);

	if (user_mode(regs)) {
#if defined(CONFIG_DEBUG_USER)
		if (user_debug & UDBG_SEGV) {
			traps_v7m_print_message(current->comm, regs, in, addr,
					hstatus, lstatus);
		}
#endif
#if 0
		if (lstatus & UNDEFINSTR) {
			send_sig(SIGTRAP, current, 0);
		}
		else {
			send_sig(SIGSEGV, current, 0);
		}
#endif
	} else {
		traps_v7m_print_message("KERNEL", regs, in, addr,
				hstatus, lstatus);
		show_regs(regs);
		kpanic("traps_v7m_common");
	}
}

/*
 * High-level exception handler for exception 3 (Hard fault).
 * @param regs		state of registers when the exception occurs
 */
void  do_hardfault(struct pt_regs *regs)
{
	traps_v7m_common(regs, HARDFAULT);
	while(1);
}

/*
 * High-level exception handler for exception 5 (Bus fault).
 * @param regs		state of registers when the exception occurs
 */
void  do_busfault(struct pt_regs *regs)
{
	traps_v7m_common(regs, BUSFAULT);
	while(1);
}

/*
 * High-level exception handler for exception 6 (Usage fault).
 * @param regs		state of registers when the exception occurs
 */
void  do_usagefault(struct pt_regs *regs)
{
	traps_v7m_common(regs, USAGEFAULT);
	while(1);
}
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
	//osSystickHandler();
}
#if 0
void do_systick(struct pt_regs *regs){

}
#endif
void asm_do_IRQ(volatile int isr, struct pt_regs *regs){
	//if(isr == 15) do_systick(regs);
	//else {
		printk("IRQ %i\n", isr);
		while(1);
	//}

}
