/*
 * tracedriver.cpp
 *
 *  Created on: Jun 14, 2017
 *      Author: Paul
 */

#include "tracedriver.hpp"
#include <cassert>
#include <array>

#include <stm32f7xx.h>
#define OS_INTEGER_TRACE_TMP_ARRAY_SIZE  (16)
extern "C" {
volatile int32_t ITM_RxBuffer;                    /*!< External variable to receive characters. */
extern uint32_t SystemCoreClock;
#ifndef ITM_RXBUFFER_EMPTY
#define ITM_RXBUFFER_EMPTY   0x5AA55AA5U /*!< Value identifying \ref ITM_RxBuffer is ready for next character. */
#endif
};
namespace mark3 {
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
#ifndef _AINLINE
#define _AINLINE static inline __attribute__ ((always_inline))
#endif
_AINLINE int call_host (int reason, void* arg);
// Function used in _exit() to return the status code as Angel exception.
_AINLINE int call_host (SEMIHOSTING reason, void* arg) { return call_host(static_cast<uint32_t>(reason),arg); }
_AINLINE void report_exception (int reason) { call_host (SEMIHOSTING::ReportException, (void*) reason); for(;;); }



// SWI numbers and reason codes for RDI (Angel) monitors.
 static constexpr uint8_t AngelSWI_Thumb = 0xAB;
 static constexpr uint32_t AngelSWI_ARM= 0x123456;
#if (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)) || defined(__thumb__)
 static constexpr uint32_t  AngelSWI = AngelSWI_Thumb;
#else
 static constexpr uint32_t  AngelSWI = AngelSWI_ARM;
#endif
 _AINLINE int call_host (int reason, void* arg)
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
int _trace_write_semihosting_debug (const char* buf, size_t nbyte)
{
#if (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)) && !defined(OS_HAS_NO_CORE_DEBUG)
  // Check if the debugger is enabled. CoreDebug is available only on CM3/CM4.
  // [Contributed by SourceForge user diabolo38]
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0)
    {
      // If not, pretend we wrote all bytes
      return  (nbyte);
    }
#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  size_t ret = nbyte;
  // Since the single character debug channel is quite slow, try to
  // optimise and send a null terminated string, if possible.
  if (buf[nbyte] == '\0')
    {
      // send string
      call_host (SEMIHOSTING::SYS_WRITE0, (void*) buf);
    }
  else
    {
      // If not, use a local buffer to speed things up
	  char tmp[OS_INTEGER_TRACE_TMP_ARRAY_SIZE];
      const char* end_buf = buf + nbyte;
      size_t pos = 0;
      while (nbyte>0)
      {
          size_t n = (nbyte > (OS_INTEGER_TRACE_TMP_ARRAY_SIZE-1)) ? (OS_INTEGER_TRACE_TMP_ARRAY_SIZE-1) : nbyte;
    	  std::copy_n(buf,n,tmp);
    	  tmp[n] = '\0';
    	  buf+=n;
    	  nbyte-=n;
          call_host (SEMIHOSTING::SYS_WRITE0, tmp);
        }
    }
  // All bytes written
  return  ret;
}
int
_trace_write_semihosting_stdout (const char* buf, size_t nbyte)
{
#if (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)) && !defined(OS_HAS_NO_CORE_DEBUG)
  // Check if the debugger is enabled. CoreDebug is available only on CM3/CM4.
  // [Contributed by SourceForge user diabolo38]
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0)
    {
      // If not, pretend we wrote all bytes
      return (ssize_t) (nbyte);
    }
#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

  static int handle=0;
  void* block[3];
  int ret;
  constexpr const char* write_filename = ":tt"; // special filename to be used for stdin/out/err
  if (handle == 0)
    {
      // On the first call get the file handle from the host
      block[0] = const_cast<char*>(write_filename); // special filename to be used for stdin/out/err
      block[1] = reinterpret_cast <void*>(4); // mode "w"
      // length of ":tt", except null terminator
      block[2] = reinterpret_cast <void*>(sizeof(write_filename) - 1);

      ret =       call_host (SEMIHOSTING::SYS_OPEN, (void*) block);
      if (ret == -1)
        return -1;

      handle = ret;
    }

  block[0] =  reinterpret_cast <void*>(handle);
  block[1] =  const_cast<char*>(buf);
  block[2] =  reinterpret_cast <void*>(nbyte);
  // send character array to host file/device
  ret =       call_host (SEMIHOSTING::SYS_WRITE, (void*) block);
  // this call returns the number of bytes NOT written (0 if all ok)

  // -1 is not a legal value, but SEGGER seems to return it
  if (ret == -1)
    return -1;

  // The compliant way of returning errors
  if (ret == (int) nbyte)
    return -1;

  // Return the number of bytes written
  return (ssize_t) (nbyte) - (ssize_t) ret;
}
// https://mcuoneclipse.com/2016/10/17/tutorial-using-single-wire-output-swo-with-arm-cortex-m-and-eclipse/
/*!
 * \brief Initialize the SWO trace port for debug message printing
 * \param portBits Port bit mask to be configured
 * \param cpuCoreFreqHz CPU core clock frequency in Hz
 */
void SWO_Init(uint32_t portBits, uint32_t cpuCoreFreqHz) {
  uint32_t SWOSpeed = 64000; /* default 64k baud rate */
  uint32_t SWOPrescaler = (cpuCoreFreqHz / SWOSpeed) - 1; /* SWOSpeed in Hz, note that cpuCoreFreqHz is expected to be match the CPU core clock */

  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; /* enable trace in core debug */
  *((volatile unsigned *)(ITM_BASE + 0x400F0)) = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
  *((volatile unsigned *)(ITM_BASE + 0x40010)) = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
  *((volatile unsigned *)(ITM_BASE + 0x00FB0)) = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */
  ITM->TCR = ITM_TCR_TraceBusID_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_ITMENA_Msk; /* ITM Trace Control Register */
  ITM->TPR = ITM_TPR_PRIVMASK_Msk; /* ITM Trace Privilege Register */
  ITM->TER = portBits; /* ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port. */
  *((volatile unsigned *)(ITM_BASE + 0x01000)) = 0x400003FE; /* DWT_CTRL */
  *((volatile unsigned *)(ITM_BASE + 0x40304)) = 0x00000100; /* Formatter and Flush Control Register */
}
void default_Swo(){
	SWO_Init(0x1, SystemCoreClock);

}
/*!
 * \brief Sends a character over the SWO channel
 * \param c Character to be sent
 * \param portNo SWO channel number, value in the range of 0 to 31
 */
void SWO_PrintChar(char c, uint8_t portNo) {
  volatile int timeout;

  /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
  if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
    return; /* not enabled? */
  }
  /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
  if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
    return; /* requested port not enabled? */
  }
  timeout = 5000; /* arbitrary timeout value */
  while (ITM->PORT[0].u32 == 0) {
    /* Wait until STIMx is ready, then send data */
    timeout--;
    if (timeout==0) {
      return; /* not able to send */
    }
  }
  ITM->PORT[0].u16 = 0x08 | (c<<8);
}
/*!
 * \brief Sends a string over SWO to the host
 * \param s String to send
 * \param portNumber Port number, 0-31, use 0 for normal debug strings
 */
void SWO_PrintString(const char *s, uint8_t portNumber) {
  while (*s!='\0') {
    SWO_PrintChar(*s++, portNumber);
  }
}
static int _swo_write(const char* buf, size_t nbyte){
	while(nbyte--)     SWO_PrintChar(*buf++, 0);
}
static inline int32_t _swo_gegc (void)
{
  int32_t ch = -1;                           /* no character available */
//  extern volatile int32_t ITM_RxBuffer;                    /*!< External variable to receive characters. */
  if (ITM_RxBuffer != ITM_RXBUFFER_EMPTY)
  {
    ch = ITM_RxBuffer;
    ITM_RxBuffer = ITM_RXBUFFER_EMPTY;       /* ready for next character */
  }

  return (ch);

}
static int _swo_read(char* buf, size_t nbyte){
	int ch;
	int ret = nbyte;
	while(nbyte--) {
		while((ch = _swo_gegc() == -1));
		*buf++ = ch & 0xFF;
	}
	return ret;
}


	void TraceDriver::Init(TraceMode mode){
		switch(mode) {
		case TraceMode::OpenOCD_Semihosting_Debug:
		   _write_function = _trace_write_semihosting_debug;
		   _read_function = nullptr;
		   break;
		case TraceMode::OpenOCD_Semihosting_Stdout:
			   _write_function = _trace_write_semihosting_stdout;
			   _read_function = nullptr;
			   break;
		case TraceMode::Stm32_SWO:
			 default_Swo();
			_write_function = _swo_write;
			_read_function = _swo_read;
			break;
		}
	}
	void TraceDriver::Init(){ Init(TraceMode::OpenOCD_Semihosting_Debug);}

	uint8_t TraceDriver::Open() {
		return 0; // no change
	}
	uint8_t TraceDriver::Close() {
		return 0; // no change
	}
	uint16_t TraceDriver::Read( uint16_t u16Bytes_, uint8_t *pu8Data_) {
		assert(_read_function);
		return _read_function((char*)pu8Data_,u16Bytes_);
	}
	uint16_t TraceDriver::Write( uint16_t u16Bytes_, uint8_t *pu8Data_) {
		assert(_write_function);
		return _write_function((const char*)pu8Data_,u16Bytes_);
	}
	uint16_t TraceDriver::Control( uint16_t u16Event_,  void *pvDataIn_, uint16_t u16SizeIn_,  void *pvDataOut_, uint16_t u16SizeOut_ ){

	}
} /* namespace mark3 */
