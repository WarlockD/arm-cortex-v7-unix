/*
 * semihosting.hpp
 *
 *  Created on: Jun 13, 2017
 *      Author: Paul
 */

#ifndef OS_HPP_
#define OS_HPP_

#include <cstdint>
#include <cstddef>

namespace os {
// Semihosting operations.
enum class SEMIHOSTING
{
  // Regular operations
  EnterSVC = 0x17,
  ReportException = 0x18,
  SYS_CLOSE = 0x02,
  SYS_CLOCK = 0x10,
  SYS_ELAPSED = 0x30,
  SYS_ERRNO = 0x13,
  SYS_FLEN = 0x0C,
  SYS_GET_CMDLINE = 0x15,
  SYS_HEAPINFO = 0x16,
  SYS_ISERROR = 0x08,
  SYS_ISTTY = 0x09,
  SYS_OPEN = 0x01,
  SYS_READ = 0x06,
  SYS_READC = 0x07,
  SYS_REMOVE = 0x0E,
  SYS_RENAME = 0x0F,
  SYS_SEEK = 0x0A,
  SYS_SYSTEM = 0x12,
  SYS_TICKFREQ = 0x31,
  SYS_TIME = 0x11,
  SYS_TMPNAM = 0x0D,
  SYS_WRITE = 0x05,
  SYS_WRITEC = 0x03,
  SYS_WRITE0 = 0x04,

  // Codes returned by ReportException
  ADP_Stopped_ApplicationExit = ((2 << 16) + 38),
  ADP_Stopped_RunTimeError = ((2 << 16) + 35),
};
  // SWI numbers and reason codes for RDI (Angel) monitors.
  static constexpr uint8_t AngelSWI_Thumb = 0xAB;
  static constexpr uint32_t AngelSWI_ARM= 0x123456;
#if (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)) || defined(__thumb__)
  static constexpr uint32_t  AngelSWI = AngelSWI_Thumb;
#else
  static constexpr uint32_t  AngelSWI = AngelSWI_ARM;
#endif
  static inline
  __attribute__ ((always_inline))
  int call_host (int reason, void* arg)
  {
    int value;
    asm volatile (

        "mov r0, %[rsn]  \n"
        "mov r1, %[arg]  \n"
    		// For thumb only architectures use the BKPT instruction instead of SWI.
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)  || defined(__ARM_ARCH_6M__)
    	"bkpt %[swi] \n"
#else
    	"swi %[swi] \n"
#endif
        " mov %[val], r0"
        : [val] "=r" (value) /* Outputs */
        : [rsn] "r" (reason), [arg] "r" (arg), [swi] "i" (AngelSWI) /* Inputs */
        : "r0", "r1", "r2", "r3", "ip", "lr", "memory", "cc"
        // Clobbers r0 and r1, and lr if in supervisor mode
    );
    // Accordingly to page 13-77 of ARM DUI 0040D other registers
    // can also be clobbered. Some memory positions may also be
    // changed by a system call, so they should not be kept in
    // registers. Note: we are assuming the manual is right and
    // Angel is respecting the APCS.
    return value;
  }
  static inline  __attribute__ ((always_inline))
  int call_host (SEMIHOSTING reason, void* arg) {
	  return call_host(static_cast<uint32_t>(reason),arg);
  }
  // Function used in _exit() to return the status code as Angel exception.
  static inline void
  __attribute__ ((always_inline,noreturn))
  report_exception (int reason) { call_host (SEMIHOSTING::ReportException, (void*) reason); for(;;); }
};


#endif /* OS_HPP_ */
