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

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <os\irq.h>

#include "diag/Trace.h"
#include "trace_impl.h"
#include "string.h"

#ifndef OS_INTEGER_TRACE_PRINTF_TMP_ARRAY_SIZE
#define OS_INTEGER_TRACE_PRINTF_TMP_ARRAY_SIZE (128)
#endif
static char g_buf[OS_INTEGER_TRACE_PRINTF_TMP_ARRAY_SIZE];

#ifdef TRACE_ENABLE_TIMESTAMP

static char tbuf[16]; // buffer for time its 16 just so its alligned
#define TSTAMP_SIZE 13
static int g_last_char = '\n';

static inline void _update_timestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	snprintf(tbuf, sizeof tbuf, "[%3d.%06ld] ", tv.tv_sec, tv.tv_usec);
}
static inline void _display_timestamp() {
	_trace_write(tbuf,TSTAMP_SIZE);
}
static inline int timestamp_putchar(int c) {
	if(g_last_char == '\n')  _display_timestamp();
	_trace_putchar(c);
	g_last_char = c;
	return c;
}



//https://stackoverflow.com/questions/2408976/struct-timeval-to-printable-format

#if 0
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64], buf[64];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv.tv_usec);
#endif

#endif

// ----------------------------------------------------------------------------
uint32_t trace_critical_begin() { return _trace_critical_begin(); }
void trace_critcal_end(uint32_t flags){ _trace_critical_end(flags); }



void trace_initialize(void) { _trace_initialize(); }

// Implementation dependent
int trace_write(const char* s, size_t nbyte){
	int ret = -1;
#ifdef TRACE_ENABLE_TIMESTAMP
	_update_timestamp();
#endif
	uint32_t flags = irq_critical_begin();
#ifdef TRACE_ENABLE_TIMESTAMP
	 for(int i=0;i < ret; i ++) {
		 uint8_t c = s[i];
		 if(timestamp_putchar(c)==-1) {
			 ret = i;
			 break;
		 }
	 }
	//ret = _timestamp_write(buf,nbyte);
#else
	ret = _trace_write(s,nbyte);
#endif
	irq_critical_end(flags);
	return ret;
}

int trace_vprintf(const char* format, va_list ap) {
#ifdef TRACE_ENABLE_TIMESTAMP
	 _update_timestamp(); // be sure the timestamp is ready
#endif
	 uint32_t flags = irq_critical_begin();
	 int ret = vsnprintf (g_buf, sizeof(g_buf), format, ap);
	 if (ret > 0)    { // Transfer the buffer to the device
#ifdef TRACE_ENABLE_TIMESTAMP
		 for(int i=0;i < ret; i ++) {
			 uint8_t c = g_buf[i];
			 if(timestamp_putchar(c)==-1) {
				 ret = i;
				 break;
			 }
		 }
#else
	ret = _trace_write(buf,ret);
#endif
	 }
	 irq_critical_end(flags);
	  return ret;
}

int trace_printf(const char* format, ...) {
  va_list ap;
  va_start (ap, format);
  int ret = trace_vprintf(format,ap);
  va_end (ap);
  return ret;
}

// special case for time stamps is untill we get the '\n' we don't display the timestamp
int trace_puts(const char *s) {
	int ret = strlen(s);
#ifdef TRACE_ENABLE_TIMESTAMP
	_update_timestamp();
#endif
	uint32_t flags = irq_critical_begin();
#ifdef TRACE_ENABLE_TIMESTAMP
	 for(int i=0;i < ret; i ++) {
		 uint8_t c = s[i];
		 if(timestamp_putchar(c)==-1) {
			 ret = i;
			 break;
		 }
	 }
	// if(g_last_char != '\n') timestamp_putchar('\n');
#else
	ret = _trace_write(s,ret);
//	if(ret > 0 && s[ret-1] != '\n') _trace_putchar('\n');
#endif
	irq_critical_end(flags);
	return ret;
}

int trace_putchar(int c) {
	int ret = -1;
#ifdef TRACE_ENABLE_TIMESTAMP
	// special case as we don't know when it was updated before
	if(g_last_char == '\n')  _update_timestamp();
#endif
	uint32_t flags = irq_critical_begin();
#ifdef TRACE_ENABLE_TIMESTAMP
	ret= timestamp_putchar(c);
#else
	ret= _trace_putchar(c);
#endif
	irq_critical_end(flags);
	return ret;
}

void
trace_dump_args(int argc, char* argv[])
{
  trace_printf("main(argc=%d, argv=[", argc);
  for (int i = 0; i < argc; ++i)
    {
      if (i != 0)
        {
          trace_printf(", ");
        }
      trace_printf("\"%s\"", argv[i]);
    }
  trace_printf("]);\n");
}

// ----------------------------------------------------------------------------

#endif // TRACE
