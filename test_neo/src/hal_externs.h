/*
 * hal_externs.h
 *
 *  Created on: Jun 19, 2017
 *      Author: warlo
 */

#ifndef HAL_EXTERNS_H_
#define HAL_EXTERNS_H_

#include "pin_micros.h"
extern ETH_HandleTypeDef heth;
extern RTC_HandleTypeDef hrtc;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart1;
extern HCD_HandleTypeDef hhcd_USB_OTG_HS;



#endif /* HAL_EXTERNS_H_ */
