
#ifndef OS_VNODE_HPP_
#define OS_VNODE_HPP_

#include "../types.hpp"
#include "../hash.hpp"
#include "../driver.hpp"
#include "usart.hpp"
#include <scm\scmRTOS.h>
#include <stm32f7xx.h>
#include <stm32746g_discovery.h>

namespace os {
	namespace device {
		namespace stm32f7xx {
		enum class uart_flag : uint32_t {
			teack	=       ((uint32_t)0x00200000U),
			sbkf	=       ((uint32_t)0x00040000U),
			cmf		=      ((uint32_t)0x00020000U),
			busy	=      ((uint32_t)0x00010000U),
			abrf	=      ((uint32_t)0x00008000U),
			abre	=      ((uint32_t)0x00004000U),
			eobf	=      ((uint32_t)0x00001000U),
			rotf	=      ((uint32_t)0x00000800U),
			cts		=      ((uint32_t)0x00000400U),
			ctsif	=      ((uint32_t)0x00000200U),
			lbdf 	=      ((uint32_t)0x00000100U),
			txe		=     ((uint32_t)0x00000080U),
			tc		=      ((uint32_t)0x00000040U),
			rxne	=      ((uint32_t)0x00000020U),
			idle 	=     ((uint32_t)0x00000010U),
			ore		=     ((uint32_t)0x00000008U),
			ne		=     ((uint32_t)0x00000004U),
			fe		=    ((uint32_t)0x00000002U),
			pe		=    ((uint32_t)0x00000001U),
		};
		enum class uart_it: uint32_t {
			/** @defgroup UART_Interrupt_definition   UART Interrupts Definition
			  *        Elements values convention: 0000ZZZZ0XXYYYYYb
			  *           - YYYYY  : Interrupt source position in the XX register (5bits)
			  *           - XX  : Interrupt source register (2bits)
			  *                 - 01: CR1 register
			  *                 - 10: CR2 register
			  *                 - 11: CR3 register
			  *           - ZZZZ  : Flag position in the ISR register(4bits)
			  * @{
			  */
			pe		=((uint32_t)0x0028U),
			txe		=((uint32_t)0x0727U),
			tc		=((uint32_t)0x0626U),
			rxne	=((uint32_t)0x0525U),
			idle	= ((uint32_t)0x0424U),
			lbd		= ((uint32_t)0x0846U),
			cts		= ((uint32_t)0x096AU),
			cm		=((uint32_t)0x112EU),
			/**       Elements values convention: 000000000XXYYYYYb
			  *           - YYYYY  : Interrupt source position in the XX register (5bits)
			  *           - XX  : Interrupt source register (2bits)
			  *                 - 01: CR1 register
			  *                 - 10: CR2 register
			  *                 - 11: CR3 register
			  */
			err= ((uint32_t)0x0060U),

			/**       Elements values convention: 0000ZZZZ00000000b
			  *           - ZZZZ  : Flag position in the ISR register(4bits)
			  */
			ORE	= ((uint32_t)0x0300U),
			ne = ((uint32_t)0x0200U),
			fe = ((uint32_t)0x0100U),
		};
		enum class uart_clearf {
			pe		= USART_ICR_PECF,            /*!< Parity Error Clear Flag */
			fe		= USART_ICR_FECF,           /*!< Framing Error Clear Flag */
			ne		= USART_ICR_NCF,             /*!< Noise detected Clear Flag */
			ore		= USART_ICR_ORECF,           /*!< OverRun Error Clear Flag */
			idle	= USART_ICR_IDLECF,          /*!< IDLE line detected Clear Flag */
			tc  	= USART_ICR_TCCF,            /*!< Transmission Complete Clear Flag */
			lbd  	= USART_ICR_LBDCF,           /*!< LIN Break Detection Clear Flag */
			cts  	= USART_ICR_CTSCF,           /*!< CTS Interrupt Clear Flag */
			rto 	= USART_ICR_RTOCF,           /*!< Receiver Time Out Clear Flag */
			eob 	= USART_ICR_EOBCF,           /*!< End Of Block Clear Flag */
			cm 		= USART_ICR_CMCF,            /*!< Character Match Clear Flag */
		};
		enum class uart_error {
			none =(1<<0), 	/*!< No error            */
			pe = (1<<1), 	/*!< Parity error        */
			ne =(1<<2), 	/*!< Noise error         */
			fe  =(1<<3), 	/*!< frame error         */
			ore = (1<<4),   /*!< Overrun error       */
			dma = (1<<5),   /*!< DMA transfer error  */
		};


#if 0
			template<size_t _PORTNUM>
			struct uart_traits : public std::false_type {
				constexpr static size_t port = _PORTNUM; // bad port number
				constexpr static USART_TypeDef* base = usart_helper::list_base[port-1];
				constexpr static IRQn_Type irqn = usart_helper::list_irqn[port-1];
			};
#endif

		enum class uart_stopbits : uint32_t {
			one=	0x00000000U,
			two = USART_CR2_STOP_1
		};
		enum class uart_wordlength : uint32_t {
			seven=USART_CR1_M_1,
			eight=0x0000U,
			nine=USART_CR1_M_0
		};
		enum class uart_parity : uint32_t {
			none=	0x00000000U,
			even = USART_CR1_PCE,
			odd = (USART_CR1_PCE | USART_CR1_PS)
		};
		enum class uart_hwcontrol {
			none=	0x00000000U,
			rts = USART_CR3_RTSE,
			cts = USART_CR3_CTSE,
			rts_cts = USART_CR3_RTSE|USART_CR3_CTSE
		};
		enum class uart_mode : uint32_t {
			rx=	USART_CR1_RE,
			tx = USART_CR1_TE,
			rx_tx = (USART_CR1_RE | USART_CR1_TE)
		};
		enum class uart_state : uint32_t {
			disable=	0x00000000U,
			enable = USART_CR1_UE
		};
		enum class uart_oversampling : uint32_t {
			eight=	USART_CR1_OVER8,
			sixteen = 0x00000000U
		};
		enum class uart_one_bit_sample : uint32_t {
			disable=	0x00000000U,
			enable = USART_CR3_ONEBIT
		};
		struct uart_args {
			  uint32_t BaudRate;                  /*!< This member configures the UART communication baud rate.
			                                           The baud rate register is computed using the following formula:
			                                           - If oversampling is 16 or in LIN mode,
			                                              Baud Rate Register = ((PCLKx) / ((huart->Init.BaudRate)))
			                                           - If oversampling is 8,
			                                              Baud Rate Register[15:4] = ((2 * PCLKx) / ((huart->Init.BaudRate)))[15:4]
			                                              Baud Rate Register[3] =  0
			                                              Baud Rate Register[2:0] =  (((2 * PCLKx) / ((huart->Init.BaudRate)))[3:0]) >> 1      */

			  uart_wordlength WordLength;                /*!< Specifies the number of data bits transmitted or received in a frame.
			                                           This parameter can be a value of @ref UARTEx_Word_Length */

			  uart_stopbits StopBits;                  /*!< Specifies the number of stop bits transmitted.
			                                           This parameter can be a value of @ref UART_Stop_Bits */

			  uart_parity Parity;                    /*!< Specifies the parity mode.
			                                           This parameter can be a value of @ref UART_Parity
			                                           @note When parity is enabled, the computed parity is inserted
			                                                 at the MSB position of the transmitted data (9th bit when
			                                                 the word length is set to 9 data bits; 8th bit when the
			                                                 word length is set to 8 data bits). */

			  uart_mode Mode;                      /*!< Specifies whether the Receive or Transmit mode is enabled or disabled.
			                                           This parameter can be a value of @ref UART_Mode */

			  uart_hwcontrol HwFlowCtl;                 /*!< Specifies whether the hardware flow control mode is enabled
			                                           or disabled.
			                                           This parameter can be a value of @ref UART_Hardware_Flow_Control */

			  uart_oversampling OverSampling;              /*!< Specifies whether the Over sampling 8 is enabled or disabled, to achieve higher speed (up to fPCLK/8).
			                                           This parameter can be a value of @ref UART_Over_Sampling */

			  uart_one_bit_sample OneBitSampling;            /*!< Specifies whether a single sample or three samples' majority vote is selected.
			                                           Selecting the single sample method increases the receiver tolerance to clock
			                                           deviations. This parameter can be a value of @ref UART_OneBit_Sampling */
			  uart_args() : BaudRate(115200), WordLength(uart_wordlength::eight), StopBits(uart_stopbits::one), Parity(uart_parity::none),
					  Mode(uart_mode::rx_tx), HwFlowCtl(uart_hwcontrol::none), OverSampling(uart_oversampling::sixteen), OneBitSampling(uart_one_bit_sample::disable) {}
			  uart_args(uint32_t BaudRate, uart_parity Parity, uart_stopbits StopBits) : BaudRate(BaudRate), WordLength(uart_wordlength::eight), StopBits(StopBits), Parity(Parity),
						  Mode(uart_mode::rx_tx), HwFlowCtl(uart_hwcontrol::none), OverSampling(uart_oversampling::sixteen), OneBitSampling(uart_one_bit_sample::disable) {}
		};


		template<size_t port>
		struct stm32f7_usart_traits { };
#define BUILD_USART_TRAITS(PORT) \
		template<> \
		struct stm32f7_usart_traits<PORT> { \
			static constexpr uint32_t port = PORT;  \
			static constexpr volatile USART_TypeDef* base = USART##PORT; \
			static constexpr IRQn_Type irqn = USART##PORT##_IRQn; \
			static constexpr uint32_t usart_source_mask = RCC_DCKCFGR2_USART##PORT##SEL; \
			enum class usart_source : uint32_t { \
				pclk2 	= ((uint32_t)0x00000000U), \
				sysclk	=  RCC_DCKCFGR2_USART##PORT##SEL_0, \
				hsi	=  RCC_DCKCFGR2_USART##PORT##SEL_1, \
				lse	=  RCC_DCKCFGR2_USART##PORT##SEL, \
			}; \
			static inline void rcc_config(usart_source s){ MODIFY_REG(RCC->DCKCFGR2, usart_source_mask, static_cast<uint32_t>(s)); } \
			static inline usart_source rcc_source(){ return static_cast<usart_source>(READ_BIT(RCC->DCKCFGR2, usart_source_mask)); }; \
		};

#define BUILD_UART_TRAITS(PORT) \
		template<> \
		struct stm32f7_usart_traits<PORT> { \
			static constexpr uint32_t port = PORT;  \
			static constexpr volatile USART_TypeDef* base = UART##PORT; \
			static constexpr IRQn_Type irqn = UART##PORT##_IRQn; \
			static constexpr uint32_t usart_source_mask = RCC_DCKCFGR2_UART##PORT##SEL; \
			enum class usart_source : uint32_t { \
				pclk2 	= ((uint32_t)0x00000000U), \
				sysclk	=  RCC_DCKCFGR2_UART##PORT##SEL_0, \
				hsi	=  RCC_DCKCFGR2_UART##PORT##SEL_1, \
				lse	=  RCC_DCKCFGR2_UART##PORT##SEL, \
			}; \
			static inline void rcc_config(usart_source s){ MODIFY_REG(RCC->DCKCFGR2, usart_source_mask, static_cast<uint32_t>(s)); } \
			static inline usart_source rcc_source(){ return static_cast<usart_source>(READ_BIT(RCC->DCKCFGR2, usart_source_mask)); }; \
		};


		BUILD_USART_TRAITS(1);
		BUILD_USART_TRAITS(2);
		BUILD_USART_TRAITS(3);
		BUILD_UART_TRAITS(4);
		BUILD_UART_TRAITS(5);
		BUILD_USART_TRAITS(6);
	//	BUILD_USART_TRAITS(7);
		//BUILD_USART_TRAITS(8);



		extern "C"  void USART1_IRQHandler();
		extern "C"  void USART2_IRQHandler();
		extern "C"  void USART3_IRQHandler();
		extern "C"  void USART4_IRQHandler();
		extern "C"  void USART5_IRQHandler();
		extern "C"  void USART6_IRQHandler();

		template<uint32_t PORT>
		class stm32f7_uart : public uart {
			friend  void USART1_IRQHandler();
			friend  void USART2_IRQHandler();
			friend  void USART3_IRQHandler();
			friend  void USART4_IRQHandler();
			friend  void USART5_IRQHandler();
			friend  void USART6_IRQHandler();

			static constexpr uint32_t port_number = PORT;
			using traits = stm32f7_usart_traits<PORT>;
			static constexpr volatile USART_TypeDef* USART = traits::base;
			constexpr static size_t _IT_MASK =   ((uint32_t)0x001FU);
			inline static void usart_clear_it(uart_clearf f) { USART->ICR = static_cast<uint32_t>(f); }
			inline static bool usart_get_flag(uart_flag f) { return (USART->ISR & static_cast<uint32_t>(f))==static_cast<uint32_t>(f); }
			inline static uint32_t usart_set_mask( uart_it it) { return 1U << (static_cast<uint32_t>(it) & _IT_MASK); }
			inline static uint32_t usart_test_mask(uart_it it) { return 1U << ((static_cast<uint32_t>(it)>>0x08)); }
			inline static volatile uint32_t* usart_get_cr( uart_it it) { return ((static_cast<uint32_t>(it) >> 5U) == 1) ? &USART->CR1 : ((static_cast<uint32_t>(it) >> 5U) == 2) ? &USART->CR2  : &USART->CR3;}
			inline static void usart_enable_it( uart_it it) {*usart_get_cr(it)  |= usart_set_mask(it);}
			inline static void usart_disable_it( uart_it it) {*usart_get_cr(it)  &= ~usart_set_mask(it);}
			inline static bool usart_get_it( uart_it it) { return (USART->ISR & usart_test_mask(it))!=0; }
			inline static bool usart_get_it_source(uart_it it) { return (*usart_get_cr(it) & usart_test_mask(it)) !=  0; }

			static inline uint32_t  get_clock_source() {
				using  usart_source = typename traits::usart_source;
				switch(traits::rcc_source()){
				//case usart_source::pclk1: return HAL_RCC_GetPCLK1Freq();
				case usart_source::pclk2: return HAL_RCC_GetPCLK2Freq();
				case usart_source::hsi: return HSI_VALUE; // better constant latter
				case usart_source::sysclk: return HAL_RCC_GetSysClockFreq();
				case usart_source::lse: return LSE_VALUE;
				default:
					return 0;
				}
			}
			static inline uint32_t div_sampling8(uint32_t clk, uint32_t band) {return ((((clk)*2)+((band)/2))/((band))); }
			static inline uint32_t div_sampling16(uint32_t clk, uint32_t band) {return (((((clk))+((band)/2))/((band))));}
			inline bool setband(uint32_t band){
				if(band == _args.BaudRate) return true;
				  uint32_t clock_freq = get_clock_source();
				  if(clock_freq ==0) return false;
				  if(_args.OverSampling == uart_oversampling::eight) {
					  uint16_t usartdiv = static_cast<uint16_t>(div_sampling8(clock_freq, band));
					  uint16_t brrtemp  = usartdiv & 0xFFF0U;
					  brrtemp |= (uint16_t)((usartdiv & (uint16_t)0x000FU) >> 1U);
					  USART->BRR = brrtemp;
				  } else {
					  USART->BRR = static_cast<uint16_t>(div_sampling16(clock_freq, band));
				  }
				  _args.BaudRate = band;
				  return true;
			}


			void isr_trasmit_start() override final {  SET_BIT(USART->CR1, USART_CR1_TXEIE); }
			void isr_trasmit_stop() override final {  CLEAR_BIT(USART->CR1, USART_CR1_TXEIE); }
			bool isr_trasmitting() const override final { return READ_BIT(USART->CR1, USART_CR1_TXEIE); }
			void isr_recive_start()override final{
			    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
			    SET_BIT(USART->CR3, USART_CR3_EIE);
			    /* Enable the UART Parity Error and Data Register not empty Interrupts */
			    SET_BIT(USART->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
			}
			void isr_recive_stop()override final{
			    CLEAR_BIT(USART->CR3, USART_CR3_EIE);
			    CLEAR_BIT(USART->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
			}
			bool isr_reciving() const override final{ return READ_BIT(USART->CR1, USART_CR1_RXNEIE); }
			void blocking_write(int c) override final{
				while(!usart_get_flag(uart_flag::txe));
				USART->TDR = c & 0xFF;
				while(!usart_get_flag(uart_flag::tc));;
			}
			int blocking_read() override final{
				while(!usart_get_flag(uart_flag::rxne));
				volatile int d = USART->RDR;
				return 0xFF & d; // fix mask here
			}


			constexpr static IRQn_Type list_irqn[] {
				USART1_IRQn,     //!< USART1 global Interrupt
				USART2_IRQn,     //!< USART2 global Interrupt
				USART3_IRQn,     //!< USART3 global Interrupt
				UART4_IRQn,     //!< UART4 global Interrupt
				UART5_IRQn,     //!< UART5 global Interrupt
				USART6_IRQn,     //!< USART6 global interrupt
			};
			bool setup(const uart_args& args){
				  constexpr uint32_t uart_cr1_fields = ((uint32_t)(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | USART_CR1_RE | USART_CR1_OVER8));
				  uint32_t tmpreg                     = 0x00000000U;
				  /*-------------------------- USART CR1 Configuration -----------------------*/
				  /* Clear M, PCE, PS, TE, RE and OVER8 bits and configure
				   *  the UART Word Length, Parity, Mode and oversampling:
				   *  set the M bits according to huart->Init.WordLength value
				   *  set PCE and PS bits according to huart->Init.Parity value
				   *  set TE and RE bits according to huart->Init.Mode value
				   *  set OVER8 bit according to huart->Init.OverSampling value */
				  tmpreg = static_cast<uint32_t>(args.WordLength) | static_cast<uint32_t>(args.Parity) |
				  	  	   static_cast<uint32_t>(args.Mode) | static_cast<uint32_t>(args.OverSampling) ;
				  MODIFY_REG(USART->CR1, uart_cr1_fields, tmpreg);

				  /*-------------------------- USART CR2 Configuration -----------------------*/
				  /* Configure the UART Stop Bits: Set STOP[13:12] bits according
				   * to huart->Init.StopBits value */
				  MODIFY_REG(USART->CR2, USART_CR2_STOP, static_cast<uint32_t>(args.StopBits));

				  /*-------------------------- USART CR3 Configuration -----------------------*/
				  /* Configure
				   * - UART HardWare Flow Control: set CTSE and RTSE bits according
				   *   to huart->Init.HwFlowCtl value
				   * - one-bit sampling method versus three samples' majority rule according
				   *   to huart->Init.OneBitSampling */
				  tmpreg = static_cast<uint32_t>(args.HwFlowCtl) | static_cast<uint32_t>(args.OneBitSampling) ;
				  MODIFY_REG(USART->CR3, (USART_CR3_RTSE | USART_CR3_CTSE | USART_CR3_ONEBIT), tmpreg);

				  /*-------------------------- USART BRR Configuration -----------------------*/
				  uint32_t clock_freq = get_clock_source();
				  if(clock_freq ==0) return false;
				  if(args.OverSampling == uart_oversampling::eight) {
					  uint16_t usartdiv = static_cast<uint16_t>(div_sampling8(clock_freq, args.BaudRate));
					  uint16_t brrtemp  = usartdiv & 0xFFF0U;
					  brrtemp |= (uint16_t)((usartdiv & (uint16_t)0x000FU) >> 1U);
					  USART->BRR = brrtemp;
				  } else {
					  USART->BRR = static_cast<uint16_t>(div_sampling16(clock_freq, args.BaudRate));
				  }
				  _args = args;
				  return true;
			}


			void enable() { USART->CR1 |= USART_CR1_UE; }
			void disable()  { USART->CR1 &= ~USART_CR1_UE; }

			uart_args _args;
			bool usart_init(void* args) override final {
				uart_args* uargs = static_cast<uart_args*>(args);
				disable();
				stop();
				if(args == nullptr || setup(*uargs)){
				    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
				    HAL_NVIC_EnableIRQ(USART1_IRQn);
					enable();
					start();
					return true;
				}
				return false;
			}
		public:
			void debug_enable(){
				disable();
				stop();
			    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
				HAL_NVIC_EnableIRQ(USART1_IRQn);
				enable();
				start();
			}
			void blocking(){
				disable();
				stop();
				_type = uart_trasfer_type::blocking;
				enable();
				start();
			}
			stm32f7_uart(uart_trasfer_type type = uart_trasfer_type::blocking) : uart(type){}
			// needs to be called by the isr
			void isr_handler(){
				 uint32_t isrflags   = READ_REG(USART->ISR);
				  uint32_t cr1its     = READ_REG(USART->CR1);
				  uint32_t cr3its     = READ_REG(USART->CR3);
				  uint32_t ErrorCode = HAL_UART_ERROR_NONE;
				  uint32_t errorflags;

				  /* If no error occurs */
				  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
				  if (errorflags == RESET)
				  {
					/* UART in mode Receiver ---------------------------------------------------*/
					if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
					{
						recive_data(USART->RDR & (uint8_t)0xFF);
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
						usart_clear_it(stm32f7xx::uart_clearf::pe);
						ErrorCode |= HAL_UART_ERROR_PE;
					}

					/* UART frame error interrupt occurred --------------------------------------*/
					if(((isrflags & USART_ISR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
					{
						usart_clear_it(stm32f7xx::uart_clearf::fe);
						ErrorCode |= HAL_UART_ERROR_FE;
					}

					/* UART noise error interrupt occurred --------------------------------------*/
					if(((isrflags & USART_ISR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
					{
						usart_clear_it(stm32f7xx::uart_clearf::ne);
						ErrorCode |= HAL_UART_ERROR_NE;
					}

					/* UART Over-Run interrupt occurred -----------------------------------------*/
					if(((isrflags & USART_ISR_ORE) != RESET) &&
					   (((cr1its & USART_CR1_RXNEIE) != RESET) || ((cr3its & USART_CR3_EIE) != RESET)))
					{
						usart_clear_it(stm32f7xx::uart_clearf::ore);
						ErrorCode |= HAL_UART_ERROR_ORE;
					}

					/* Call UART Error Call back function if need be --------------------------*/
					if(ErrorCode != HAL_UART_ERROR_NONE)
					{
					  /* UART in mode Receiver ---------------------------------------------------*/
					  if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
					  {
						  recive_data(USART->RDR & (uint8_t)0xFF);
					  }

					  /* If Overrun error occurs, or if any error occurs in DMA mode reception,
						 consider error as blocking */
					  if (((ErrorCode & HAL_UART_ERROR_ORE) != RESET) ||
						  (HAL_IS_BIT_SET(USART->CR3, USART_CR3_DMAR)))
					  {
						/* Blocking error : transfer is aborted
						   Set the UART state ready to be able to start again the process,
						   Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
						  isr_recive_stop();
						/* Disable the UART DMA Rx request if enabled */
						if (HAL_IS_BIT_SET(USART->CR3, USART_CR3_DMAR))
						{   // dma handle
						  CLEAR_BIT(USART->CR3, USART_CR3_DMAR);
						}
						isr_error(ErrorCode);
					  }
					  else
					  {
						/* Non Blocking error : transfer could go on.
						   Error is notified to user through user error callback */
						  isr_error(ErrorCode);
						ErrorCode = HAL_UART_ERROR_NONE;
					  }
					}
					return;

				  } /* End if some error occurs */

				  /* UART in mode Transmitter ------------------------------------------------*/
				  if(((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
				  {
					  int data = isr_trasmiter_next_byte();
					  if(data < 0){
						  /* Disable the UART Transmit Data Register Empty Interrupt */
						  CLEAR_BIT(USART->CR1, USART_CR1_TXEIE);

						  /* Enable the UART Transmit Complete Interrupt */
						  SET_BIT(USART->CR1, USART_CR1_TCIE);
					  } else {
						  USART->TDR = data;
					  }
					return;
				  }

				  /* UART in mode Transmitter (transmission end) -----------------------------*/
				  if(((isrflags & USART_ISR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
				  {
					  CLEAR_BIT(USART->CR1, USART_CR1_TCIE);
					  isr_trasmit_complete();
					  return;
				  }
			}

		};
	};

	};

};





#endif /* OS_VNODE_HPP_ */
