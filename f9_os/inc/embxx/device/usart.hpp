/*
 * usart.hpp
 *
 *  Created on: Jun 17, 2017
 *      Author: Paul
 */

#ifndef EMBXX_DRIVER_USART_HPP_
#define EMBXX_DRIVER_USART_HPP_

#include "DeviceOpQueue.h"
#include <stm32f7xx.h>

namespace mark3 {

	struct usart {
		using  DeviceIdType = USART_TypeDef;
		using CharType = uint8_t;
        template <typename TFunc>
         void setCanReadHandler(TFunc&& func){

        }
	};

} /* namespace mark3 */

#endif /* EMBXX_DRIVER_USART_HPP_ */
