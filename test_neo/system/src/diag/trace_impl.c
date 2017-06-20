/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#if defined(TRACE)

#include "trace_impl.h"
#include <assert.h>

#include "diag/Trace.h"



#if defined(OS_USE_TRACE_ITM) && (!defined(__ARM_ARCH_7M__) || !defined(__ARM_ARCH_7EM__))

#if !defined(OS_INTEGER_TRACE_ITM_STIMULUS_PORT)
#define OS_INTEGER_TRACE_ITM_STIMULUS_PORT     (0)
#endif

void _trace_initialize(void){
	// thsi is the only time we check the itm port so we don't check it on evey char c
	   // Check if ITM or the stimulus port are not enabled
	     assert(((ITM->TCR & ITM_TCR_ITMENA_Msk))
		  && ((ITM->TER & (1UL << OS_INTEGER_TRACE_ITM_STIMULUS_PORT)) ));
//	SWO_Init(0x1, 25000000);
  // For regular ITM / semihosting, no inits required.
}
__attribute__((always_inline)) static inline  int __trace_putchar(int c) {
    // Check if ITM or the stimulus port are not enabled
    if (((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0)
	  || ((ITM->TER & (1UL << OS_INTEGER_TRACE_ITM_STIMULUS_PORT)) == 0))
	  return -1; // return error, as we are disabled
    // Wait until STIMx is ready...
    while (ITM->PORT[OS_INTEGER_TRACE_ITM_STIMULUS_PORT].u32 == 0);
    ITM->PORT[0].u8 = (uint8_t)c;
    return c;
}
// defines for Trace.c
int _trace_putchar(int c) {
	return __trace_putchar(c);
}
int _trace_write(const char* buf, size_t nbyte)
{
  for (size_t i = 0; i < nbyte; i++)
	  if(__trace_putchar(buf[i]) == -1) return (int)i;
  return (int)nbyte; // all characters successfully sent
}

#else

#ifdef OS_USE_TRACE_ITM
#error "ITM unavailable"
#endif

#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)
#if defined(OS_USE_TRACE_SEMIHOSTING_STDOUT) || defined(OS_USE_TRACE_SEMIHOSTING_DEBUG)
#error "Cannot debug semihosting using semihosting trace; use OS_USE_TRACE_ITM"
#endif
#endif

#include "arm/semihosting.h"
#define OS_INTEGER_TRACE_TMP_ARRAY_SIZE  (16)
// Semihosting is the other output channel that can be used for the trace
// messages. It comes in two flavours: STDOUT and DEBUG. The STDOUT channel
// is the equivalent of the stdout in POSIX and in most cases it is forwarded
// to the GDB server stdout stream. The debug channel is a separate
// channel. STDOUT is buffered, so nothing is displayed until a \n;
// DEBUG is not buffered, but can be slow.
//
// Choosing between semihosting stdout and debug depends on the capabilities
// of your GDB server, and also on specific needs. It is recommended to test
// DEBUG first, and if too slow, try STDOUT.
//
// The JLink GDB server fully support semihosting, and both configurations
// are available; to activate it, use "monitor semihosting enable" or check
// the corresponding button in the JLink Debugging plug-in.
// In OpenOCD, support for semihosting can be enabled using
// "monitor arm semihosting enable".
//
// Note: Applications built with semihosting output active normally cannot
// be executed without the debugger connected and active, since they use
// BKPT to communicate with the host. However, with a carefully written
// HardFault_Handler, the semihosting BKPT calls can be processed, making
// possible to run semihosting applications as standalone, without being
// terminated with hardware faults.

#if OS_USE_TRACE_SEMIHOSTING_STDOUT

int _trace_write(const char* buf, size_t nbyte)
{
  static int handle;
  void* block[3];
  int ret;

  if (handle == 0)
    {
      // On the first call get the file handle from the host
      block[0] = ":tt"; // special filename to be used for stdin/out/err
      block[1] = (void*) 4; // mode "w"
      // length of ":tt", except null terminator
      block[2] = (void*) (sizeof(":tt") - 1);

      ret = call_host (SEMIHOSTING_SYS_OPEN, (void*) block);
      if (ret == -1)
        return -1;

      handle = ret;
    }

  block[0] = (void*) handle;
  block[1] = (void*) buf;
  block[2] = (void*) nbyte;
  // send character array to host file/device
  ret = call_host (SEMIHOSTING_SYS_WRITE, (void*) block);
  // this call returns the number of bytes NOT written (0 if all ok)

  // -1 is not a legal value, but SEGGER seems to return it
  if (ret == -1)
    return -1;

  // The compliant way of returning errors
  if (ret == (int) nbyte)
    return -1;

  // Return the number of bytes written
  return (int) (nbyte) - (int) ret;
}

#elseif defined(OS_USE_TRACE_SEMIHOSTING_DEBUG)


int _trace_write(const char* buf, size_t nbyte)
{
  // Since the single character debug channel is quite slow, try to
  // optimise and send a null terminated string, if possible.
  if (buf[nbyte] == '\0')
    {
      // send string
      call_host (SEMIHOSTING_SYS_WRITE0, (void*) buf);
    }
  else
    {
      // If not, use a local buffer to speed things up
      char tmp[OS_INTEGER_TRACE_TMP_ARRAY_SIZE];
      size_t togo = nbyte;
      while (togo > 0)
        {
          unsigned int n = ((togo < sizeof(tmp)) ? togo : sizeof(tmp));
          unsigned int i = 0;
          for (; i < n; ++i, ++buf)
            {
              tmp[i] = *buf;
            }
          tmp[i] = '\0';

          call_host (SEMIHOSTING_SYS_WRITE0, (void*) tmp);

          togo -= n;
        }
    }

  // All bytes written
  return (int) nbyte;
}
int _trace_putc(char c) {
	return _trace_write(&c,1);
}

#endif

#endif

#endif

// ----------------------------------------------------------------------------

