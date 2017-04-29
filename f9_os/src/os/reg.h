// this is the fram order
#if !defined(REGS_BEGIN) || !defined(REGS_BEGIN) || !defined(REGS_BEGIN)
#error no macros defined
#endif
// change this file if the frame changes
REGS_BEGIN
	// saved in trap
	REGS_VAL(sp)
	REGS_VAL(basepri)
	REGS_VAL(r4)
	REGS_VAL(r5)
	REGS_VAL(r6)
	REGS_VAL(r7)
	REGS_VAL(r8)
	REGS_VAL(r9)
	REGS_VAL(r10)
	REGS_VAL(fp)
	REGS_VAL(ex_lr)
	/* Automatically saved registers */
	REGS_VAL(r0)
	REGS_VAL(r1)
	REGS_VAL(r2)
	REGS_VAL(r3)
	REGS_VAL(ip)
	REGS_VAL(lr)
	REGS_VAL(pc)
	REGS_VAL(xpsr)
REGS_END

#ifndef TF_HW_OFFSET
#define TF_HW_OFFSET REGS_NAME(r0)
#endif

#ifndef TF_HW_SIZE
#define TF_HW_SIZE ((REGS_NAME(xpsr)-REGS_NAME(r0))*4)
#endif

#ifndef TF_SW_SIZE
#define TF_SW_SIZE ((REGS_NAME(ex_lr)-REGS_NAME(sp))*4)
#endif

#ifndef TF_SIZE
#define TF_SIZE (REGS_NAME(xpsr)*4)
#endif

#undef REGS_BEGIN
#undef REGS_VAL
#undef REGS_END
