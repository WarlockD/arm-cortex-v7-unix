/*
 * tracedriver.hpp
 *
 *  Created on: Jun 14, 2017
 *      Author: Paul
 */

#ifndef MARK3_DRIVERS_TRACEDRIVER_HPP_
#define MARK3_DRIVERS_TRACEDRIVER_HPP_

#include "driver.h"
namespace mark3 {
int _trace_write_semihosting_debug (const char* buf, size_t nbyte);
int _trace_write_semihosting_stdout(const char* buf, size_t nbyte);
	enum class TraceMode {
		OpenOCD_Semihosting_Debug,
		OpenOCD_Semihosting_Stdout,
		Stm32_SWO
	};
	class TraceDriver : public Driver {
		int(*_write_function)(const char*,size_t nbytes) = nullptr;
		int(*_read_function)(char*,size_t nbytes) = nullptr;
	public:
	   void Init();
	   // use semihost debug instead of stdout
	   void Init(TraceMode mode);
	   uint8_t Open() ;
	   uint8_t Close() ;
	   uint16_t Read( uint16_t u16Bytes_, uint8_t *pu8Data_) ;
	   uint16_t Write( uint16_t u16Bytes_, uint8_t *pu8Data_) ;
	   uint16_t Control( uint16_t u16Event_,  void *pvDataIn_, uint16_t u16SizeIn_,  void *pvDataOut_, uint16_t u16SizeOut_ );
	};
} /* namespace mark3 */

#endif /* MARK3_DRIVERS_TRACEDRIVER_HPP_ */
