
#ifndef OS_IRQ_HPP_
#define OS_IRQ_HPP_

#include <stm32f7xx.h>

#include "types.hpp"

namespace dma {
	constexpr static size_t DMA_CONTROLER_COUNT = 2;
	constexpr static size_t DMA_STREAM_COUNT = 8;    // per controler
	constexpr static size_t DMA_CCHANNEL_COUNT = 8;  // per stream
	enum class  state {
		  reset             = 0x00U,  /*!< DMA not yet initialized or disabled */
		  ready             = 0x01U,  /*!< DMA initialized and ready for use   */
		  busy              = 0x02U,  /*!< DMA process is ongoing              */
		  timeout           = 0x03U,  /*!< DMA timeout state                   */
		  error             = 0x04U,  /*!< DMA error state                     */
		  abort             = 0x05U,  /*!< DMA Abort state                     */
	};
	struct callback {
		  virtual void XferCpltCallback()=0;     /*!< DMA transfer complete callback         */
		  virtual void XferHalfCpltCallback()=0; /*!< DMA Half transfer complete callback    */
		  virtual void XferM1CpltCallback()=0;   /*!< DMA transfer complete Memory1 callback */
		  virtual void XferM1HalfCpltCallback()=0;   /*!< DMA transfer Half complete Memory1 callback */
		  virtual void XferErrorCallback()=0;    /*!< DMA transfer error callback            */
		  virtual void XferAbortCallback()=0;    /*!< DMA transfer Abort callback            */
		  virtual ~callback(){}
	};
	enum class error : uint32_t {
		none = 0x00000000U,     /*!< No error                               */
		te = 0x00000001U,     /*!< Transfer error                         */
		fe = 0x00000002U,     /*!< FIFO error                             */
		dme = 0x00000004U,     /*!< Direct Mode error                      */
		timeout = 0x00000020U,     /*!< Timeout error                          */
		param = 0x00000040U,     /*!< Parameter error                        */
		no_xfer = 0x00000080U,     /*!< Abort requested with no Xfer ongoing   */
		not_supported = 0x00000100U,     /*!< Not supported mode 					 */
	};
	enum class direction {
		periph_to_memory = 0x00000000U,       /*!< Peripheral to memory direction */
		memory_to_periph = DMA_SxCR_DIR_0,   /*!< Memory to peripheral direction */
		memory_to_memory = DMA_SxCR_DIR_1,   /*!< Memory to memory direction     */
	};
	enum class pinc : uint32_t {
		enable = ((uint32_t)DMA_SxCR_PINC) , /*!< Peripheral increment mode enable  */
		disable =0
	};
	enum class mic : uint32_t {
		enable = ((uint32_t)DMA_SxCR_MINC) ,  /*!< Memory increment mode enable  */
		disable =0
	};
	enum class pdataalign : uint32_t {
		byte = 0x00000000U ,  /*!< Memory increment mode enable  */
		halfword =DMA_SxCR_PSIZE_0,
		word = DMA_SxCR_PSIZE_1
	};
	enum class mdataalign : uint32_t {
		byte = 0x00000000U ,  /*!< Memory increment mode enable  */
		halfword =DMA_SxCR_MSIZE_0,
		word = DMA_SxCR_MSIZE_1
	};
	enum class mode : uint32_t {
		normal =      0x00000000U,      /*!< Normal mode                  */
		circular =     DMA_SxCR_CIRC,    /*!< Circular mode                */
		pfctrl   =  DMA_SxCR_PFCTRL,  /*!< Peripheral flow control mode */
	};
	enum class priority : uint32_t {
		low =0x00000000U,
		medium =DMA_SxCR_PL_0,
		high =DMA_SxCR_PL_1,
		very_high =DMA_SxCR_PL,
	};
	enum class fifomode : uint32_t {
		disable = 0x00000000U,
		enable = DMA_SxFCR_DMDIS,
	};
	enum class it : uint32_t {
		tc                     =    ((uint32_t)DMA_SxCR_TCIE),
		ht                     =    ((uint32_t)DMA_SxCR_HTIE),
		te                      =   ((uint32_t)DMA_SxCR_TEIE),
		dme                     =   ((uint32_t)DMA_SxCR_DMEIE),
		fe                      =   ((uint32_t)0x00000080U)
	};
	enum class flag : uint32_t {
		FEIF0_4 = 0x00800001U,
		DMEIF0_4 = 0x00800004U,
		TEIF0_4 = 0x00000008U,
		HTIF0_4 = 0x00000010U,
		TCIF0_4 = 0x00000020U,
		FEIF1_5 = 0x00000040U,
		DMEIF1_5 = 0x00000100U,
		TEIF1_5 = 0x00000200U,
		HTIF1_5 = 0x00000400U,
		TCIF1_5 = 0x00000800U,
		FEIF2_6 = 0x00010000U,
		DMEIF2_6 = 0x00040000U,
		TEIF2_6 = 0x00080000U,
		HTIF2_6 = 0x00100000U,
		TCIF2_6 = 0x00200000U,
		FEIF3_7 = 0x00400000U,
		DMEIF3_7 = 0x01000000U,
		TEIF3_7 = 0x02000000U,
		HTIF3_7 = 0x04000000U,
		TCIF3_7 = 0x08000000U,
	};

	/**
	  * @brief  DMA Configuration Structure definition
	  */
	struct init_t
	{
	  uint32_t Channel;              /*!< Specifies the channel used for the specified stream.
	                                      This parameter can be a value of @ref DMAEx_Channel_selection                  */

	  direction Direction;            /*!< Specifies if the data will be transferred from memory to peripheral,
	                                      from memory to memory or from peripheral to memory.
	                                      This parameter can be a value of @ref DMA_Data_transfer_direction              */

	  pinc PeriphInc;            /*!< Specifies whether the Peripheral address register should be incremented or not.
	                                      This parameter can be a value of @ref DMA_Peripheral_incremented_mode          */

	  mic MemInc;               /*!< Specifies whether the memory address register should be incremented or not.
	                                      This parameter can be a value of @ref DMA_Memory_incremented_mode              */

	  pdataalign PeriphDataAlignment;  /*!< Specifies the Peripheral data width.
	                                      This parameter can be a value of @ref DMA_Peripheral_data_size                 */

	  mdataalign MemDataAlignment;     /*!< Specifies the Memory data width.
	                                      This parameter can be a value of @ref DMA_Memory_data_size                     */

	  mode Mode;                 /*!< Specifies the operation mode of the DMAy Streamx.
	                                      This parameter can be a value of @ref DMA_mode
	                                      @note The circular buffer mode cannot be used if the memory-to-memory
	                                            data transfer is configured on the selected Stream                        */

	  priority Priority;             /*!< Specifies the software priority for the DMAy Streamx.
	                                      This parameter can be a value of @ref DMA_Priority_level                       */

	  fifomode FIFOMode;             /*!< Specifies if the FIFO mode or Direct mode will be used for the specified stream.
	                                      This parameter can be a value of @ref DMA_FIFO_direct_mode
	                                      @note The Direct mode (FIFO mode disabled) cannot be used if the
	                                            memory-to-memory data transfer is configured on the selected stream       */

	  uint32_t FIFOThreshold;        /*!< Specifies the FIFO threshold level.
	                                      This parameter can be a value of @ref DMA_FIFO_threshold_level                  */

	  uint32_t MemBurst;             /*!< Specifies the Burst transfer configuration for the memory transfers.
	                                      It specifies the amount of data to be transferred in a single non interruptible
	                                      transaction.
	                                      This parameter can be a value of @ref DMA_Memory_burst
	                                      @note The burst mode is possible only if the address Increment mode is enabled. */

	  uint32_t PeriphBurst;          /*!< Specifies the Burst transfer configuration for the peripheral transfers.
	                                      It specifies the amount of data to be transferred in a single non interruptible
	                                      transaction.
	                                      This parameter can be a value of @ref DMA_Peripheral_burst
	                                      @note The burst mode is possible only if the address Increment mode is enabled. */
	};
	class dma_t {
		init_t _init;
		state _state;
		DMA_Stream_TypeDef* _instance;
		bool _locked;
		volatile uint32_t _error;                                                    /*!< DMA Error code                          */
		 uint32_t                    _stream_base_address;   /*!< DMA Stream Base Address                */
		 uint32_t                    _stream_index;
		// simple system that locks a dma
	public:
		 void reset() { _state = state::reset; }
		 void irq_handler();

	};
};


namespace irq {
	static constexpr uint32_t SPL_PRIORITY_MAX =15;
	static constexpr uint32_t SPL_PRIORITY_MIN =1;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_CLOCK =SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_NET =SPL_PRIORITY_MIN+1;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_BIO =SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_IMP =SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_TTY=SPL_PRIORITY_MIN+6;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_SOFT_CLOCK=SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_STAT_CLOCK=SPL_PRIORITY_MIN+5;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_VM =SPL_PRIORITY_MIN+6;		/* 0 is hw clock and mabye systick? */
	static constexpr uint32_t SPL_PRIORITY_HIGH =SPL_PRIORITY_MAX;
	static constexpr uint32_t SPL_PRIORITY_SCHED =SPL_PRIORITY_MAX;
	static inline void spls(uint32_t x) {
		__asm volatile ( "msr basepri, %0" :  : "r"(x) );
	}
	static inline uint32_t splg() {
		uint32_t ret;
		__asm volatile ( "mrs %0, basepri"  :  "=r"(ret) :);
		return ret;
	}
	static inline uint32_t splx(uint32_t x) {
		uint32_t ret;
		__asm volatile (
				"mrs %0, basepri\n"
				"msr basepri, %1\n"
				: "=r"(ret) : "r"(x) );
		return ret;
	}
	static inline uint32_t spl0() { return splx(SPL_PRIORITY_MAX);}
	static inline uint32_t spl1() { return splx(2);}
	static inline uint32_t spl2() { return splx(3);}
	static inline uint32_t spl3() { return splx(5);}
	static inline uint32_t spl4() { return splx(3);}
	static inline uint32_t spl5() { return splx(2);}
	static inline uint32_t spl6() { return splx(0);}
	static inline uint32_t spl7() { return splx(0);}

	static inline uint32_t splsoftclock() { return splx(SPL_PRIORITY_SOFT_CLOCK);}
	static inline uint32_t splnet() 		{ return splx(SPL_PRIORITY_NET);}
	static inline uint32_t splbio() 		{ return splx(SPL_PRIORITY_BIO);}
	static inline uint32_t splimp() 		{ return splx(SPL_PRIORITY_IMP);}
	static inline uint32_t spltty() 		{ return splx(SPL_PRIORITY_TTY);}
	static inline uint32_t splclock() 	{ return splx(SPL_PRIORITY_CLOCK);}
	static inline uint32_t splstatclock() { return splx(SPL_PRIORITY_STAT_CLOCK);}
	static inline uint32_t splvm() 		{ return splx(SPL_PRIORITY_VM);}
	static inline uint32_t splhigh() 		{ return splx(SPL_PRIORITY_HIGH);}
	static inline uint32_t splsched() 	{ return splx(SPL_PRIORITY_SCHED);}

	static inline  void enable_interrupts() { __asm__ __volatile__ ("cpsie i"); }
	static inline  void disable_interrupts() { __asm__ __volatile__ ("cpsid i"); }

	static inline void set_interrupt_state(uint32_t status){ __asm__ __volatile__ ("MSR PRIMASK, %0\n": : "r"(status):"memory");}

	static inline uint32_t get_interrupt_state()
	{
			uint32_t sr;
		__asm__ __volatile__ ("MRS %0, PRIMASK" : "=r"(sr) );
		return sr;
	}
	// kernel is only active in an isr anyway
	static inline bool in_irq()  {
		uint32_t ret;
		__asm volatile ( "mrs %0, ipsr"  :  "=r"(ret) :);
		return ret != 0;
	}

	static inline void idle(){
		auto s = spl7();
		__asm volatile("wfi":::"memory");
		splx(s);
	}
	typedef void (*timer_callback)();
	static constexpr size_t HZ10 = 10U; // 10 times a second
	static constexpr size_t HZ60 = 60U; // 60 times a second
	static constexpr size_t HZ1000 = 1000U; // 60 times a second, 1ms
	void init_system_timer(timer_callback callback, size_t hz=(HZ10));// -1 is d

	void lock_system_timer();
	void unlock_system_timer();
	size_t ticks(); // ticks from timer

	// swithc from too
	typedef uint32_t* (*switch_callback)(uint32_t*);
	void init_context_switch(switch_callback func);
	void raise_context_switch();
	// takes a prebiult context and fixes it so that it calls a function
	// the the context starts back up
	uint32_t* init_stack_frame( uint32_t * stack, void (*exec)(), void (*exit_func)()=nullptr);
	// used to setup the stack frame
	uint32_t* setup_exec(uint32_t* stack, void(*enter_exec)(void*), void* arg,void(*exit_exec)()) ;
	inline uint32_t* setup_exec(uint32_t* stack, void(*enter_exec)(void*), void* arg){ return setup_exec(stack,enter_exec,arg,nullptr); }
	inline uint32_t* setup_exec(uint32_t* stack, void(*enter_exec)(void*), void(*exit_exec)()){ return setup_exec(stack,enter_exec,nullptr,exit_exec); }

	typedef void (*signal_callback)(int);
	uint32_t*  sendsig(uint32_t* stack, signal_callback callback,int signo); // sends a signal using the current process stack
};

#endif
