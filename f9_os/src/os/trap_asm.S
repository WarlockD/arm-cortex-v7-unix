
  .syntax unified
  .cpu cortex-m7
  .fpu softvfp
  .thumb

.extern do_debug_default

.global HardFault_Handler
.type  HardFault_Handler, %function
HardFault_Handler:
    tst lr, #4
    ite eq
    mrseq r1, msp
   	mrsne r1, psp
	mrs r0, ipsr
	b do_debug_default
	blx lr
.size  HardFault_Handler, .-HardFault_Handler

