/*
 * trace_impl.h
 *
 *  Created on: Jun 19, 2017
 *      Author: warlo
 */

#ifndef SRC_DIAG_TRACE_IMPL_H_
#define SRC_DIAG_TRACE_IMPL_H_

#include "cmsis_device.h"


#ifdef __cplusplus
extern "C" {
#endif

int _trace_write(const char* buf, size_t nbyte);
int _trace_putchar(int c);
void _trace_initialize();

#ifdef __cplusplus
}
#endif

#endif /* SRC_DIAG_TRACE_IMPL_H_ */
