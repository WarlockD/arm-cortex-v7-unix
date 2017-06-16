/*
 * trace.h
 *
 *  Created on: Jun 13, 2017
 *      Author: Paul
 */

#ifndef OS_TRACE_H_
#define OS_TRACE_H_


#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C"
{
#endif

  void trace_initialize(bool line_timestamp);

  // Implementation dependent
  int trace_write(const char* buf, size_t nbyte);

  // ----- Portable -----
  int trace_vprintf(const char* format, va_list va);
  int trace_printf(const char* format, ...);

  int trace_puts(const char *s);

  int trace_putchar(int c);

  void trace_dump_args(int argc, char* argv[]);



#if defined(__cplusplus)
}
#endif


#endif /* OS_TRACE_H_ */
