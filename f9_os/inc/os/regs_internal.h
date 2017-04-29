#ifndef REG_DEFINE
#error DO NOT INCLUDE THIS FILE WILLY NILLY
#endif
#ifndef REG_SW_START
#define REG_SW_START(FIRST)
#endif
#ifndef REG_SW_END
#define REG_SW_END(LAST)
#endif

REG_SW_START(0)
REG_DEFINE_START(R13,0) /* R13 = SP at time of interrupt */
#ifdef CONFIG_ARMV7M_USEBASEPRI
REG_DEFINE_NEXT(BASEPRI)  /* BASEPRI */
#else
REG_DEFINE_NEXT(PRIMASK)  /* PRIMASK */
#endif
REG_DEFINE_NEXT(R4)/* R4 */
REG_DEFINE_NEXT(R5)/* R5 */
REG_DEFINE_NEXT(R6) /* R6 */
REG_DEFINE_NEXT(R7)  /* R7 */
REG_DEFINE_NEXT(R8)  /* R8 */
REG_DEFINE_NEXT(R9)  /* R9 */
REG_DEFINE_NEXT(R10)  /* R10 */
REG_DEFINE_NEXT(R11) /* R11 */
#ifdef CONFIG_BUILD_PROTECTED
REG_DEFINE_NEXT(REG_EXC_RETURN) /* EXC_RETURN */
REG_SW_END(REG_EXC_RETURN)
#else
REG_SW_END(R11)
#endif

#ifdef CONFIG_ARCH_FPU
#ifdef CONFIG_BUILD_PROTECTED
REG_FPU_SW_START(REG_EXC_RETURN)
#else
REG_FPU_SW_START(R11)
#endif

/* If the MCU supports a floating point unit, then it will be necessary
 * to save the state of the non-volatile registers before calling code
 * that may save and overwrite them.
 */

REG_DEFINE_NEXT(S16) /* S16 */
REG_DEFINE_NEXT(S17)      /* S17 */
REG_DEFINE_NEXT(S18)      /* S18 */
REG_DEFINE_NEXT(S19)       /* S19 */
REG_DEFINE_NEXT(S20)       /* S20 */
REG_DEFINE_NEXT(S21)         /* S21 */
REG_DEFINE_NEXT(S22)   /* S22 */
REG_DEFINE_NEXT(S23)        /* S23 */
REG_DEFINE_NEXT(S24)      /* S24 */
REG_DEFINE_NEXT(S25)   /* S25 */
REG_DEFINE_NEXT(S26)      /* S26 */
REG_DEFINE_NEXT(S27)      /* S27 */
REG_DEFINE_NEXT(S28)     /* S28 */
REG_DEFINE_NEXT(S29)    /* S29 */
REG_DEFINE_NEXT(S30)       /* S30 */
REG_DEFINE_NEXT(S31)      /* S31 */
#endif
REG_SW_REGS_SIZE /* defins the total space saved by sw regesters */

/* The total number of registers saved by software */

#define SW_XCPT_REGS        (SW_INT_REGS + SW_FPU_REGS)
#define SW_XCPT_SIZE        (4 * SW_XCPT_REGS)
