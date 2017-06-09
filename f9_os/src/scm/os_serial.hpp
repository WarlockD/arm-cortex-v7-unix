/*
 * os_serial.hpp
 *
 *  Created on: Jun 9, 2017
 *      Author: Paul
 */
#include "scmRTOS.h"
#include <stm32f7xx.h>

#ifndef SCM_OS_SERIAL_HPP_
#define SCM_OS_SERIAL_HPP_

namespace OS {
	class STM32F7_UART : public TService {
    protected:
        volatile TProcessMap _outgoingProcessMap;
        volatile TProcessMap _incommingProcessMap;
        volatile bool _trasmitting=false;
        volatile bool _reciveing=false;
        usr::ring_buffer<uint8_t, 1024, size_t> _outgoing; // OS::channel object for 8 'TSlon*' items
        usr::ring_buffer<uint8_t, 1024, size_t> _incomming; // OS::channel object for 8 'TSlon*' items
		UART_HandleTypeDef * _handle;
		friend class STM32F7_UART_INTERNAL;
		void incomming_isr();
		void outgoing_isr();
		void outgoing_isr_complete();
		void isr_handler();
		void start_incomming();
		void stop_incomming();
		void start_outgoing();
		void stop_outgoing();
		friend void _USART1_IRQHandler();
    public:
		STM32F7_UART(UART_HandleTypeDef * handle);

		template<typename T>
		void write(const T* data, size_t count) {
		    TCritSect cs;
		    const uint8_t* cdata = reinterpret_cast<const uint8_t*>(data);
		    count *= sizeof(T);
		    while(_outgoing.get_free_size() < count)
		    {
		        // channel has not enough space, suspend current process
		        suspend(_outgoingProcessMap);
		    }
		    _outgoing.write(cdata,count);
		    if(!_trasmitting) start_outgoing();
		}
		void push(uint8_t c) {
		    TCritSect cs;

		    while (!_outgoing.get_free_size())
		    {
		        // channel is full, suspend current process
		        suspend(_outgoingProcessMap);
		    }
		    _outgoing.push(c);
		    if(!_trasmitting) start_outgoing();
		}

	};




}



#endif /* SCM_OS_SERIAL_HPP_ */
