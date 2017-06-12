#include "os_serial.hpp"

#include <array>
#include <cassert>
namespace {
	static inline void hal_debug( HAL_StatusTypeDef status) {
		if(status == HAL_OK) return;
		volatile HAL_StatusTypeDef meh = status;
		assert(meh == HAL_OK);
	}
}
namespace OS {
static OS::STM32F7_UART* usart1_driver = nullptr;
void _USART1_IRQHandler() {
	if(usart1_driver) usart1_driver->isr_handler();
}

}

namespace OS {
	class STM32F7_RAWUART {
		USART_TypeDef* _uart;
		bool _trasmit_completed;
		virtual void trasmit_complete() {}
		enum class Parity {
			None =    ((uint32_t)0x00000000U),
			Even= ((uint32_t)USART_CR1_PCE),
			Odd = ((uint32_t)(USART_CR1_PCE | USART_CR1_PS))
		};
		static constexpr uint32_t ParityMask = ((uint32_t)(USART_CR1_PCE | USART_CR1_PS)) ;
		enum class StopBits {
			One = ((uint32_t)0x00000000U),
			Two =  ((uint32_t)USART_CR2_STOP_1)
		};
		static constexpr uint32_t StopBitsMask = ((uint32_t)USART_CR2_STOP_1) ;
		enum class WordLength {
			Bits7=((uint32_t)USART_CR1_M_1),
			Bits8=  ((uint32_t)0x0000U),
			Bits9=  ((uint32_t)USART_CR1_M_0)
		};
		static constexpr uint32_t WordLengthMask = ((uint32_t)USART_CR1_M_1|USART_CR1_M_0) ;
		static Parity get_parity(USART_TypeDef* uart) { return static_cast<Parity>(uart->CR1 & ParityMask); }
		static StopBits get_stopbits(USART_TypeDef* uart) { return static_cast<StopBits>(uart->CR2 & StopBitsMask); }
		static WordLength get_wordlength(USART_TypeDef* uart) { return static_cast<WordLength>(uart->CR1 & WordLengthMask); }
		void endtrasmit_it() {
			  CLEAR_BIT(_uart->CR1, USART_CR1_TCIE);
			  trasmit_complete();
			  _trasmit_completed=true;
		}

	};
	STM32F7_UART::STM32F7_UART(UART_HandleTypeDef * handle) : _handle(handle) {
		usart1_driver = this;
		//if(_handle->Instance == USART1)
	}
	void STM32F7_UART::incomming_isr(){
		// just set up for 8 bit
		uint8_t data = (uint8_t)(_handle->Instance->RDR & 0xFF);
		if(_incomming.get_free_size() == 0) resume_all_isr(_incommingProcessMap);
		_incomming.push(data);
	}
	void STM32F7_UART::outgoing_isr(){
		if(_outgoing.get_count() == 0) {
		      /* Disable the UART Transmit Data Register Empty Interrupt */
		      CLEAR_BIT(_handle->Instance->CR1, USART_CR1_TXEIE);
		      /* Enable the UART Transmit Complete Interrupt */
		      SET_BIT(_handle->Instance->CR1, USART_CR1_TCIE);
		} else {
			_handle->Instance->TDR = (uint8_t)(_outgoing.pop() & (uint8_t)0xFFU);
		}
	}
	void STM32F7_UART::outgoing_isr_complete(){
		  /* Disable TXEIE and TCIE interrupts */
		  CLEAR_BIT(_handle->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));
		  if(_outgoing.get_count() > 0) {
		      SET_BIT(_handle->Instance->CR1, USART_CR1_TXEIE);
		      // start trasmiting again
		  } else {
			  stop_outgoing();
		  }
		  resume_all_isr(_outgoingProcessMap);
	}
	void STM32F7_UART::start_incomming() {
	    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
		if( !_reciveing ) {
			  /* Enable the Peripheral */
			  __HAL_UART_ENABLE(_handle);

		    SET_BIT(_handle->Instance->CR3, USART_CR3_EIE);

		    /* Enable the UART Parity Error and Data Register not empty Interrupts */
		    SET_BIT(_handle->Instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);

			 _reciveing = true;
		}

	}
	void STM32F7_UART::stop_incomming(){
		  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
		  CLEAR_BIT(_handle->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
		  CLEAR_BIT(_handle->Instance->CR3, USART_CR3_EIE);
		  _reciveing = false;
	}
	void STM32F7_UART::start_outgoing(){
		if( !_trasmitting ) {
			  /* Enable the Peripheral */
			  __HAL_UART_ENABLE(_handle);
		    /* Enable the UART Transmit Data Register Empty Interrupt */
		    SET_BIT(_handle->Instance->CR1, USART_CR1_TXEIE);
		}
	}
	void STM32F7_UART::stop_outgoing(){
		  /* Disable TXEIE and TCIE interrupts */
		  CLEAR_BIT(_handle->Instance->CR1, (USART_CR1_TXEIE | USART_CR1_TCIE));
		  _trasmitting = false;
	}
	void STM32F7_UART::isr_handler() {
		auto huart = _handle;
		  uint32_t isrflags   = READ_REG(huart->Instance->ISR);
		  uint32_t cr1its     = READ_REG(huart->Instance->CR1);
		  uint32_t cr3its     = READ_REG(huart->Instance->CR3);
		  uint32_t errorflags;


		  /* If no error occurs */
		  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
		  if (errorflags == RESET)
		  {
		    /* UART in mode Receiver ---------------------------------------------------*/
		    if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
		    {
		    	incomming_isr();
		      return;
		    }
		  }

		  /* If some errors occur */
		  if(   (errorflags != RESET)
		     && (   ((cr3its & USART_CR3_EIE) != RESET)
		         || ((cr1its & (USART_CR1_RXNEIE | USART_CR1_PEIE)) != RESET)) )
		  {

		    /* UART parity error interrupt occurred -------------------------------------*/
		    if(((isrflags & USART_ISR_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
		    {
		      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_PEF);

		      huart->ErrorCode |= HAL_UART_ERROR_PE;
		    }

		    /* UART frame error interrupt occurred --------------------------------------*/
		    if(((isrflags & USART_ISR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
		    {
		      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_FEF);

		      huart->ErrorCode |= HAL_UART_ERROR_FE;
		    }

		    /* UART noise error interrupt occurred --------------------------------------*/
		    if(((isrflags & USART_ISR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
		    {
		      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_NEF);

		      huart->ErrorCode |= HAL_UART_ERROR_NE;
		    }

		    /* UART Over-Run interrupt occurred -----------------------------------------*/
		    if(((isrflags & USART_ISR_ORE) != RESET) &&
		       (((cr1its & USART_CR1_RXNEIE) != RESET) || ((cr3its & USART_CR3_EIE) != RESET)))
		    {
		      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_OREF);

		      huart->ErrorCode |= HAL_UART_ERROR_ORE;
		    }

		    /* Call UART Error Call back function if need be --------------------------*/
		    if(huart->ErrorCode != HAL_UART_ERROR_NONE)
		    {
		      /* UART in mode Receiver ---------------------------------------------------*/
		      if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
		      {
		    	  incomming_isr();
		      }

		      /* If Overrun error occurs, or if any error occurs in DMA mode reception,
		         consider error as blocking */
		      if (((huart->ErrorCode & HAL_UART_ERROR_ORE) != RESET) ||
		          (HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR)))
		      {
		        /* Blocking error : transfer is aborted
		           Set the UART state ready to be able to start again the process,
		           Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
		    	  stop_incomming();

		        /* Disable the UART DMA Rx request if enabled */
		        if (HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR))
		        {
		          CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);
		          // handle errors
#if 0
		          /* Abort the UART DMA Rx channel */
		          if(huart->hdmarx != NULL)
		          {
		            /* Set the UART DMA Abort callback :
		            will lead to call HAL_UART_ErrorCallback() at end of DMA abort procedure */
		            huart->hdmarx->XferAbortCallback = UART_DMAAbortOnError;

		            /* Abort DMA RX */
		            if(HAL_DMA_Abort_IT(huart->hdmarx) != HAL_OK)
		            {
		              /* Call Directly huart->hdmarx->XferAbortCallback function in case of error */
		              huart->hdmarx->XferAbortCallback(huart->hdmarx);
		            }
		          }
		          else
		          {
		            /* Call user error callback */
		            HAL_UART_ErrorCallback(huart);
		          }
#endif
		        }
		        else
		        {
		          /* Call user error callback */
		          HAL_UART_ErrorCallback(huart);
		        }
		      }
		      else
		      {
		        /* Non Blocking error : transfer could go on.
		           Error is notified to user through user error callback */
		        HAL_UART_ErrorCallback(huart);
		        huart->ErrorCode = HAL_UART_ERROR_NONE;
		      }
		    }
		    return;

		  } /* End if some error occurs */

		  /* UART in mode Transmitter ------------------------------------------------*/
		  if(((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
		  {
			  outgoing_isr();
		    return;
		  }

		  /* UART in mode Transmitter (transmission end) -----------------------------*/
		  if(((isrflags & USART_ISR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
		  {
			  outgoing_isr_complete();
		    return;
		  }
	}
}
#if 0

		static void handle_isr(int port) {

		}
		static void handle_isr(UART_HandleTypeDef *huart, int port) {
			  uint32_t isrflags   = READ_REG(huart->Instance->ISR);
			  uint32_t cr1its     = READ_REG(huart->Instance->CR1);
			  uint32_t cr3its     = READ_REG(huart->Instance->CR3);
			  uint32_t errorflags;


			  /* If no error occurs */
			  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
			  if (errorflags == RESET)
			  {
			    /* UART in mode Receiver ---------------------------------------------------*/
			    if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
			    {
			      UART_Receive_IT(huart);
			      return;
			    }
			  }

			  /* If some errors occur */
			  if(   (errorflags != RESET)
			     && (   ((cr3its & USART_CR3_EIE) != RESET)
			         || ((cr1its & (USART_CR1_RXNEIE | USART_CR1_PEIE)) != RESET)) )
			  {

			    /* UART parity error interrupt occurred -------------------------------------*/
			    if(((isrflags & USART_ISR_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
			    {
			      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_PEF);

			      huart->ErrorCode |= HAL_UART_ERROR_PE;
			    }

			    /* UART frame error interrupt occurred --------------------------------------*/
			    if(((isrflags & USART_ISR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
			    {
			      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_FEF);

			      huart->ErrorCode |= HAL_UART_ERROR_FE;
			    }

			    /* UART noise error interrupt occurred --------------------------------------*/
			    if(((isrflags & USART_ISR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
			    {
			      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_NEF);

			      huart->ErrorCode |= HAL_UART_ERROR_NE;
			    }

			    /* UART Over-Run interrupt occurred -----------------------------------------*/
			    if(((isrflags & USART_ISR_ORE) != RESET) &&
			       (((cr1its & USART_CR1_RXNEIE) != RESET) || ((cr3its & USART_CR3_EIE) != RESET)))
			    {
			      __HAL_UART_CLEAR_IT(huart, UART_CLEAR_OREF);

			      huart->ErrorCode |= HAL_UART_ERROR_ORE;
			    }

			    /* Call UART Error Call back function if need be --------------------------*/
			    if(huart->ErrorCode != HAL_UART_ERROR_NONE)
			    {
			      /* UART in mode Receiver ---------------------------------------------------*/
			      if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
			      {
			        UART_Receive_IT(huart);
			      }

			      /* If Overrun error occurs, or if any error occurs in DMA mode reception,
			         consider error as blocking */
			      if (((huart->ErrorCode & HAL_UART_ERROR_ORE) != RESET) ||
			          (HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR)))
			      {
			        /* Blocking error : transfer is aborted
			           Set the UART state ready to be able to start again the process,
			           Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
			        UART_EndRxTransfer(huart);

			        /* Disable the UART DMA Rx request if enabled */
			        if (HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR))
			        {
			          CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);

			          /* Abort the UART DMA Rx channel */
			          if(huart->hdmarx != NULL)
			          {
			            /* Set the UART DMA Abort callback :
			            will lead to call HAL_UART_ErrorCallback() at end of DMA abort procedure */
			            huart->hdmarx->XferAbortCallback = UART_DMAAbortOnError;

			            /* Abort DMA RX */
			            if(HAL_DMA_Abort_IT(huart->hdmarx) != HAL_OK)
			            {
			              /* Call Directly huart->hdmarx->XferAbortCallback function in case of error */
			              huart->hdmarx->XferAbortCallback(huart->hdmarx);
			            }
			          }
			          else
			          {
			            /* Call user error callback */
			            HAL_UART_ErrorCallback(huart);
			          }
			        }
			        else
			        {
			          /* Call user error callback */
			          HAL_UART_ErrorCallback(huart);
			        }
			      }
			      else
			      {
			        /* Non Blocking error : transfer could go on.
			           Error is notified to user through user error callback */
			        HAL_UART_ErrorCallback(huart);
			        huart->ErrorCode = HAL_UART_ERROR_NONE;
			      }
			    }
			    return;

			  } /* End if some error occurs */

			  /* UART in mode Transmitter ------------------------------------------------*/
			  if(((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
			  {
			    UART_Transmit_IT(huart);
			    return;
			  }

			  /* UART in mode Transmitter (transmission end) -----------------------------*/
			  if(((isrflags & USART_ISR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
			  {
			    UART_EndTransmit_IT(huart);
			    return;
			  }
		}
		static void trasmit_blocking(int port, const uint8_t* data, size_t size) {
			UART_HandleTypeDef* huart = handle_from_port(port);
			assert(huart);
			hal_debug(HAL_UART_Transmit(huart, const_cast<uint8_t*>(data),size,1000));
		}

	};

}
#endif
