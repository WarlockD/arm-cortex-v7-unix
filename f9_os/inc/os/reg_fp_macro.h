#ifndef START_DEFINE_FPU
#if defined(DEFINE_FPD_REG) || defined(DEFINE_FPS_REG)
#define START_DEFINE_FPU /* */
#else
#error needs to be defined
#endif
#endif


#ifndef END_DEFINE_FPU
#if defined(DEFINE_FPD_REG) || defined(DEFINE_FPS_REG)
#define END_DEFINE_FPU /* */
#else
#error needs to be defined
#endif
#endif

#ifndef START_DEFINE_DFPU
#define START_DEFINE_DFPU /* */
#endif

#ifndef END_DEFINE_DFPU
#define END_DEFINE_DFPU /* */
#endif

#ifndef START_DEFINE_SFPU
#define START_DEFINE_SFPU /* */
#endif

#ifndef END_DEFINE_SFPU
#define END_DEFINE_SFPU /* */
#endif

#ifndef DEFINE_FPD_REG
#error needs to be defined
#endif

#ifndef DEFINE_FPS_REG
#error needs to be defined
#endif



/* If the MCU supports a floating point unit, then it will be necessary
 * to save the state of the FPU status register and data registers on
 * each context switch.  These registers are not saved during interrupt
 * level processing, however. So, as a consequence, floating point
 * operations may NOT be performed in interrupt handlers.
 *
 * The FPU provides an extension register file containing 32 single-
 * precision registers. These can be viewed as:
 *
 * - Sixteen 64-bit doubleword registers, D0-D15
 * - Thirty-two 32-bit single-word registers, S0-S31
 *   S<2n> maps to the least significant half of D<n>
 *   S<2n+1> maps to the most significant half of D<n>.
 */


START_DEFINE_FPU
START_DEFINE_DFPU
DEFINE_FPD_REG(D0,SW_INT_REGS+0)  /* D0 */
DEFINE_FPD_REG(D1,SW_INT_REGS+2)  /* D0 */
DEFINE_FPD_REG(D2,SW_INT_REGS+4)  /* D0 */
DEFINE_FPD_REG(D3,SW_INT_REGS+6)  /* D0 */
DEFINE_FPD_REG(D4,SW_INT_REGS+8)  /* D0 */
DEFINE_FPD_REG(D5,SW_INT_REGS+10)  /* D0 */
DEFINE_FPD_REG(D6,SW_INT_REGS+12)  /* D0 */
DEFINE_FPD_REG(D7,SW_INT_REGS+14)  /* D0 */
DEFINE_FPD_REG(D8,SW_INT_REGS+16)  /* D0 */
DEFINE_FPD_REG(D9,SW_INT_REGS+18)  /* D0 */
DEFINE_FPD_REG(D10,SW_INT_REGS+20)  /* D0 */
DEFINE_FPD_REG(D11,SW_INT_REGS+22)  /* D0 */
DEFINE_FPD_REG(D12,SW_INT_REGS+24)  /* D0 */
DEFINE_FPD_REG(D13,SW_INT_REGS+26)  /* D0 */
DEFINE_FPD_REG(D14,SW_INT_REGS+28)  /* D0 */
DEFINE_FPD_REG(D15,SW_INT_REGS+30)  /* D0 */
END_DEFINE_DFPU
START_DEFINE_SFPU
DEFINE_FPS_REG(S0,SW_INT_REGS+0)  /* S0 */
DEFINE_FPS_REG(S1,SW_INT_REGS+1)  /* S1 */
DEFINE_FPS_REG(S2,SW_INT_REGS+2)  /* S2 */
DEFINE_FPS_REG(S3,SW_INT_REGS+3)  /* S3 */
DEFINE_FPS_REG(S4,SW_INT_REGS+4)  /* S4 */
DEFINE_FPS_REG(S5,SW_INT_REGS+5)  /* S5 */
DEFINE_FPS_REG(S6,SW_INT_REGS+6)  /* S6 */
DEFINE_FPS_REG(S7,SW_INT_REGS+7)  /* S7 */
DEFINE_FPS_REG(S8,SW_INT_REGS+8)  /* S8 */
DEFINE_FPS_REG(S9,SW_INT_REGS+9)  /* S9 */
DEFINE_FPS_REG(S10,SW_INT_REGS+10) /* S10 */
DEFINE_FPS_REG(S11,SW_INT_REGS+11) /* S11 */
DEFINE_FPS_REG(S12,SW_INT_REGS+12) /* S12 */
DEFINE_FPS_REG(S13,SW_INT_REGS+13) /* S13 */
DEFINE_FPS_REG(S14,SW_INT_REGS+14) /* S14 */
DEFINE_FPS_REG(S15,SW_INT_REGS+15) /* S15 */
DEFINE_FPS_REG(S16,SW_INT_REGS+16) /* S16 */
DEFINE_FPS_REG(S17,SW_INT_REGS+17) /* S17 */
DEFINE_FPS_REG(S18,SW_INT_REGS+18) /* S18 */
DEFINE_FPS_REG(S19,SW_INT_REGS+19) /* S19 */
DEFINE_FPS_REG(S20,SW_INT_REGS+20) /* S20 */
DEFINE_FPS_REG(S21,SW_INT_REGS+21) /* S21 */
DEFINE_FPS_REG(S22,SW_INT_REGS+22) /* S22 */
DEFINE_FPS_REG(S23,SW_INT_REGS+23) /* S23 */
DEFINE_FPS_REG(S24,SW_INT_REGS+24) /* S24 */
DEFINE_FPS_REG(S25,SW_INT_REGS+25) /* S25 */
DEFINE_FPS_REG(S26,SW_INT_REGS+26) /* S26 */
DEFINE_FPS_REG(S27,SW_INT_REGS+27) /* S27 */
DEFINE_FPS_REG(S28,SW_INT_REGS+28) /* S28 */
DEFINE_FPS_REG(S29,SW_INT_REGS+29) /* S29 */
DEFINE_FPS_REG(S30,SW_INT_REGS+30) /* S30 */
DEFINE_FPS_REG(S31,SW_INT_REGS+31) /* S31 */
END_DEFINE_SFPU
DEFINE_REG(FPSCR,SW_INT_REGS+32) /* Floating point status and control */
END_DEFINE_FPU


#undef START_DEFINE_FPU
#undef END_DEFINE_FPU
#undef DEFINE_FPD_REG
#undef DEFINE_FPS_REG
#undef END_DEFINE_SFPU
#undef START_DEFINE_SFPU
#undef END_DEFINE_DFPU
#undef START_DEFINE_DFPU
