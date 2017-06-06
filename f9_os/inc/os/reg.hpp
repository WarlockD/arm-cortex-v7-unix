/*
 * reg.hpp
 *
 *  Created on: Jun 3, 2017
 *      Author: warlo
 */

#ifndef OS_REG_HPP_
#define OS_REG_HPP_

#include <array>
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <cassert>

#define CONFIG_BUILD_PROTECTED
#define CONFIG_ARMV7M_USEBASEPRI

//#define CONFIG_ARCH_FPU


namespace chip {

	static constexpr uint32_t EXC_RETURN_BASE = 0xffffffe1; // these bits are aalways set
	static constexpr uint32_t EXC_RETURN_PROCESS_STACK =(1 << 2);
	static constexpr uint32_t EXC_RETURN_THREAD_MODE   =(1 << 3);
	static constexpr uint32_t EXC_RETURN_STD_CONTEXT   =(1 << 4); // no fp
	static constexpr uint32_t EXC_RETURN_HANDLER       =0xfffffff1; // state on main stack Execution uses MSP after return.

	class ex_return {
	public:
		constexpr bool state_on_psp() const { return (_ret & EXC_RETURN_PROCESS_STACK) != 0; }
		constexpr bool thread_mode() const { return (_ret & EXC_RETURN_THREAD_MODE) != 0; }
		constexpr bool std_context() const { return (_ret & EXC_RETURN_STD_CONTEXT) != 0; }
		constexpr void set_state_on_psp() { _ret |= EXC_RETURN_PROCESS_STACK; }
		constexpr void set_state_on_msp() { _ret &= ~EXC_RETURN_PROCESS_STACK; }
		constexpr void set_thread_mode() { _ret |= EXC_RETURN_THREAD_MODE; }
		constexpr void set_handler_mode() { _ret &= ~EXC_RETURN_THREAD_MODE; }
		constexpr void set_std_context() { _ret |= EXC_RETURN_STD_CONTEXT; }
		constexpr void set_fp_context() { _ret &= ~EXC_RETURN_STD_CONTEXT; }
		constexpr operator uint32_t()  const { return _ret; }
		constexpr ex_return(uint32_t ret) : _ret(ret) {}
		constexpr ex_return(bool thread_mode, bool state_on_psp, bool std_context=true) :
				_ret(EXC_RETURN_HANDLER |
						(thread_mode ?  EXC_RETURN_THREAD_MODE : 0) |
						(state_on_psp ?  EXC_RETURN_PROCESS_STACK : 0)  |
						(std_context ?  EXC_RETURN_STD_CONTEXT : 0)) {}
		constexpr ex_return() : _ret(EXC_RETURN_HANDLER)  {}
	public:
		uint32_t _ret;
	};

	static constexpr size_t REG_EXTRA = 1000; // used for fpu

	static inline uint8_t  getreg8(uintptr_t addr) { return *reinterpret_cast<volatile uint8_t*>(addr); }
	static inline uint16_t getreg16(intptr_t addr)
	{
	  uint16_t retval;
	 __asm__ __volatile__("\tldrh %0, [%1]\n\t" : "=r"(retval) : "r"(addr));
	  return retval;
	}
	static inline uint32_t getreg32(uintptr_t addr) { return *reinterpret_cast<volatile uint8_t*>(addr); }


	static inline void putreg8(uintptr_t addr, uint8_t val) {
		 __asm__ __volatile__("\tstrb %0, [%1]\n\t": : "r"(val), "r"(addr));
	}
	static inline void putreg16(uintptr_t addr, uint16_t val) {
	 __asm__ __volatile__("\tstrh %0, [%1]\n\t": : "r"(val), "r"(addr));
	}
	static inline void putreg32(uintptr_t addr, uint32_t val) {
	 __asm__ __volatile__("\tstr %0, [%1]\n\t": : "r"(val), "r"(addr));
	}
	template<typename T, typename std::enable_if<sizeof(T) == 4>::type>
	static inline void putreg(uintptr_t addr, T value) {  putreg32(addr,(uint32_t)value); }
	template<typename T, typename std::enable_if<sizeof(T) == 2>::type>
	static inline void putreg(uintptr_t addr, T value) {  putreg16(addr,(uint16_t)value); }
	template<typename T, typename std::enable_if<sizeof(T) == 1>::type>
	static inline void putreg(uintptr_t addr, T value) {  putreg8(addr,(uint8_t)value); }

	static inline uint32_t xchg32(uintptr_t addr, uint32_t val) {
		  uint32_t retval;
		 __asm__ __volatile__("\tldr %0, [%1]\n\t" : "=r"(retval) : "r"(addr));
		 __asm__ __volatile__("\tstr %0, [%1]\n\t": : "r"(val), "r"(addr));
		  return retval;
	}
	static inline uint16_t xchg16(uintptr_t addr, uint16_t val) {
		  uint32_t retval;
		 __asm__ __volatile__("\tldh %0, [%1]\n\t" : "=r"(retval) : "r"(addr));
		 __asm__ __volatile__("\tsth %0, [%1]\n\t": : "r"(val), "r"(addr));
		  return retval;
	}
	static inline uint8_t xchg8(uintptr_t addr, uint8_t val) {
		  uint32_t retval;
		 __asm__ __volatile__("\tld %0, [%1]\n\t" : "=r"(retval) : "r"(addr));
		 __asm__ __volatile__("\tst %0, [%1]\n\t": : "r"(val), "r"(addr));
		  return retval;
	}
	template<typename T, typename std::enable_if<sizeof(T) == 4>::type>
	static inline T xchg(uintptr_t addr, T value) {  return (T)xchg32(addr,(uint32_t)value); }
	template<typename T, typename std::enable_if<sizeof(T) == 2>::type>
	static inline T xchg(uintptr_t addr, T value) {  return (T)xchg16(addr,(uint16_t)value); }
	template<typename T, typename std::enable_if<sizeof(T) == 1>::type>
	static inline T xchg(uintptr_t addr, T value) {  return (T)xchg8(addr,(uint8_t)value); }

	enum class REG {
		SP       =  0, /* R13 = SP at time of interrupt */
		BASEPRI , // = 1  /* BASEPRI */
		R4 , // = 2  /* R4 */
		R5 , // = 3  /* R5 */
		R6 , // = 4  /* R6 */
		R7 , // = 5  /* R7 */
		R8 , // = 6  /* R8 */
		R9 , // = 7  /* R9 */
		R10 , // = 8  /* R10 */
		R11 , // = 9  /* R11 */
		EXC_RETURN , // = 10 /* EXC_RETURN */
		R0 , // = SW_XCPT_REGS+0 /* R0 */
		R1 , // = SW_XCPT_REGS+1 /* R1 */
		R2 , // = SW_XCPT_REGS+2 /* R2 */
		R3 , // = SW_XCPT_REGS+3 /* R3 */
		R12 , // = SW_XCPT_REGS+4 /* R12 */
		//R14 , // = SW_XCPT_REGS+5 /* R14 = LR */
		LR,
		//R15 , // = SW_XCPT_REGS+6 /* R15 = PC */
		PC,
		XPSR , // = SW_XCPT_REGS+7 /* xPSR */
		MAX_STATE_SIZE,   // end of regs saved state
		IPSR,
		EPSR,
		APSR,
		CONTROL,
		PRIMASK
#ifdef CONFIG_ARCH_FPU
		D0 , // = SW_INT_REGS+0  /* D0 */
		D1 , // = SW_INT_REGS+2  /* D1 */
		D2 , // = SW_INT_REGS+0  /* D0 */
		D3 , // = SW_INT_REGS+2  /* D1 */
		D4 , // = SW_INT_REGS+0  /* D0 */
		D5 , // = SW_INT_REGS+2  /* D1 */
		D6 , // = SW_INT_REGS+0  /* D0 */
		D7 , // = SW_INT_REGS+2  /* D1 */
		D8 , // = SW_INT_REGS+0  /* D0 */
		D9 , // = SW_INT_REGS+2  /* D1 */
		D10 , // = SW_INT_REGS+0  /* D0 */
		D11 , // = SW_INT_REGS+2  /* D1 */
		D12 , // = SW_INT_REGS+0  /* D0 */
		D13 , // = SW_INT_REGS+2  /* D1 */
		D14 , // = SW_INT_REGS+0  /* D0 */
		D15 , // = SW_INT_REGS+2  /* D1 */
#endif
	};

	 // template<typename> struct __is_basic_type : public std::false_type { };
	  template<typename T,size_t SIZE=sizeof(T)>
	  struct is_basic_type {
		  using decay = typename std::decay<T>::type;
		  static constexpr bool value = !std::is_pointer<decay>::value && std::is_arithmetic<decay>::value && SIZE == sizeof(T);
		  using type = typename std::conditional<value,std::true_type,std::false_type>::type;
	  };
	  template<typename T>
	  struct is_basic_type_ptr {
		  using decay = typename std::decay<T>::type;
		 // using type = typename std::remove_pointer<T>::type;
		  static constexpr bool value = std::is_pointer<decay>::value && std::is_arithmetic<typename std::remove_pointer<T>::type>::value;
		  using type = typename std::conditional<value,std::true_type,std::false_type>::type;
	  };
	  namespace priv {
	  	  template <typename T, typename U> struct _reg_t;
	  	  template <typename T, T N> struct _reg_t<T, std::integral_constant<T, N>> {};
	  	  template <typename T> struct _reg_t<T, std::integral_constant<T, 0>> {};
	  	  template <typename T, T N> struct X : _reg_t<T, std::integral_constant<T, N>> {};

	  }
	  // f is int for filler
	  template<typename T, typename E = void, typename F=int> struct reg_t {};

	template<typename T>
	struct reg_t<T, typename std::enable_if<is_basic_type<T,4>::value>::type> {
		  using type = T;
		  using pointer = typename std::add_pointer<type>::type;
		  using refrence = typename std::add_lvalue_reference<T>::type;
		  using vpointer = volatile pointer;
		  using vrefrence = volatile refrence;
		  type operator=(type value) { *(volatile type*)_ptr = value; return value; }
		  void operator|=(type value) { *(volatile type*)_ptr |= value; }
		  void operator&=(type value) { *(volatile type*)_ptr &= value; }
		  operator type() { return *(volatile type*)_ptr; }
		  //reg_t(volatile pointer ptr) : _ptr(ptr) {}
		  reg_t(uintptr_t ptr) : _ptr(ptr) {}
	private:
		  uintptr_t _ptr;
	};
	template<uintptr_t N, typename T=uint32_t>
	struct fixed_reg : std::integral_constant<uintptr_t,N> {
		static constexpr uintptr_t addr = N;
		using type = T;
	    type operator=(type value) { *(volatile type*)addr = value; return value; }
	    void operator|=(type value) { *(volatile type*)addr |= value; }
	    void operator&=(type value) { *(volatile type*)addr &= value; }
	    operator type() { return *(volatile type*)addr; }
	};

	template<typename T>
	struct reg_t<T,  typename std::enable_if<is_basic_type_ptr<T>::value>::type>  {
	  using type = typename std::remove_pointer<T>::type;
	  using pointer = typename std::add_pointer<type>::type;
	  using refrence = typename std::add_lvalue_reference<T>::type;
	  using vpointer = volatile pointer;
	  using vrefrence = volatile refrence;
	  vrefrence at(size_t i) { return ((vpointer)_ptr)[i]; }
	  const vrefrence at(size_t i) const { return ((vpointer)_ptr)[i]; }
	  vrefrence operator[](size_t i) { return at(i); }
	  const vrefrence operator[](size_t i) const { return at(i); }
	  reg_t(uintptr_t ptr) : _ptr(ptr) {}
	private:
	  	  uintptr_t _ptr;
	};

	template<REG R> struct sreg_t {
		static constexpr bool valid = false;
	};
	template<> struct sreg_t<REG::CONTROL> {
		static constexpr bool valid = true;
		static constexpr REG sreg = REG::CONTROL;
		uint32_t __attribute((always_inline)) static inline get(){
			uint32_t value;
			__asm volatile("mrs %0, control" : "=r"(value));
			return value;
		}
		__attribute((always_inline)) static inline void set(uint32_t value) {
			__asm volatile("msr control, %0" : : "r"(value));
		}
	};
	template<> struct sreg_t<REG::BASEPRI> {
		static constexpr bool valid = true;
		static constexpr REG sreg = REG::BASEPRI;
		uint32_t __attribute((always_inline)) static inline get(){
			uint32_t value;
			__asm volatile("mrs %0, basepri" : "=r"(value));
			return value;
		}
		__attribute((always_inline)) static inline void set(uint32_t value) {
			__asm volatile("msr basepri, %0" : : "r"(value));
		}
	};
	template<> struct sreg_t<REG::IPSR> {
		static constexpr bool valid = true;
		static constexpr REG sreg = REG::IPSR;
		uint32_t __attribute((always_inline)) static inline get(){
			uint32_t value;
			__asm volatile("mrs %0, ipsr" : "=r"(value));
			return value;
		}
		__attribute((always_inline)) static inline void set(uint32_t value) {
			__asm volatile("msr ipsr, %0" : : "r"(value));
		}
	};
	template<> struct sreg_t<REG::PRIMASK> {
		static constexpr bool valid = true;
		static constexpr REG sreg = REG::PRIMASK;
		uint32_t __attribute((always_inline)) static inline get(){
			uint32_t value;
			__asm volatile("mrs %0, primask" : "=r"(value));
			return value;
		}
		__attribute((always_inline)) static inline void set(uint32_t value) {
			__asm volatile("msr primask, %0" : : "r"(value));
		}
	};
#if 0
	PSR	RW	Privileged	0x01000000	Program Status Register
	ASPR	RW	Either	Unknown	Application Program Status Register
	IPSR	RO	Privileged	0x00000000	Interrupt Program Status Register
	EPSR	RO	Privileged	0x01000000	Execution Program Status Register
	PRIMASK	RW	Privileged	0x00000000	Priority Mask Register
	FAULTMASK	RW	Privileged	0x00000000	Fault Mask Register
	BASEPRI	RW	Privileged	0x00000000	Base Priority Mask Register
	CONTROL	RW	Privileged	0x00000000	CONTROL register
#endif

	template<REG R>
	struct reg_t<std::integral_constant<REG,R>, typename std::enable_if<sreg_t<R>::valid>::type> {
		using sreg = sreg_t<R>;

	    inline uint32_t operator=(uint32_t value) { sreg::set(value); return value; }
		inline void operator|=(uint32_t value) {  sreg::set(value | sreg::get()); }
		inline void operator&=(uint32_t value) {  sreg::set(value & sreg::get()); }
	    operator uint32_t() { return sreg::get(); }
	};

	template<typename T, uintptr_t A>
	struct reg_t<std::integral_constant<T,A>, typename std::enable_if<is_basic_type<T>::value>::type> { // : public fixed_reg<A,T>
		static constexpr uintptr_t addr = A;
		using type = T;
		type operator=(type value) { *(volatile type*)addr = value; return value; }
	    void operator|=(type value) { *(volatile type*)addr |= value; }
	    void operator&=(type value) { *(volatile type*)addr &= value; }
	    operator type() { return *(volatile type*)addr; }
	};
	template<uintptr_t A>
	struct reg_t<std::integral_constant<uint32_t,A>, typename std::true_type> { // : public fixed_reg<A,T>
		static constexpr uintptr_t addr = A;
		using type = uint32_t;
		type operator=(type value) { *(volatile type*)addr = value; return value; }
	    void operator|=(type value) { *(volatile type*)addr |= value; }
	    void operator&=(type value) { *(volatile type*)addr &= value; }
	    operator type() { return *(volatile type*)addr; }
	};



	/* Application Program Status Register (APSR) */
	enum class APSR_FLAGS : uint32_t {
		Q           = (1 << 27), /* Bit 27: Sticky saturation flag */
		V           = (1 << 28), /* Bit 28: Overflow flag */
		C           = (1 << 29), /* Bit 29: Carry/borrow flag */
		Z           = (1 << 30), /* Bit 30: Zero flag */
		N           = static_cast<uint32_t>((1 << 31)), /* Bit 31: Negative, less than flag */
	};


	static constexpr uint32_t IPSR_ISR_SHIFT  = 0;
	static constexpr uint32_t IPSR_ISR_MASK = 0x1ff<<IPSR_ISR_SHIFT;
	static constexpr uint32_t EPSR_ICIIT1_SHIFT =10;        /* Bits 15-10: Interrupt-Continuable-Instruction/If-Then bits */
	static constexpr uint32_t EPSR_ICIIT1_MASK  =(3 << EPSR_ICIIT1_SHIFT);
	static constexpr uint32_t EPSR_T            =(1 << 24); /* Bit 24: T-bit */
	static constexpr uint32_t EPSR_ICIIT2_SHIFT =25;       /* Bits 26-25: Interrupt-Continuable-Instruction/If-Then bits */
	static constexpr uint32_t EPSR_ICIIT2_MASK  =(3 << EPSR_ICIIT2_SHIFT);
	/* Interrupt Program Status Register (IPSR) */
	namespace XPSR_FLAGS {
		static constexpr uint32_t ISR_SHIFT    =IPSR_ISR_SHIFT;
		static constexpr uint32_t ISR_MASK     =IPSR_ISR_MASK;
		static constexpr uint32_t ICIIT1_SHIFT =EPSR_ICIIT1_SHIFT;
		static constexpr uint32_t ICIIT1_MASK  =EPSR_ICIIT1_MASK;
		static constexpr uint32_t T            =EPSR_T;
		static constexpr uint32_t ICIIT2_SHIFT =EPSR_ICIIT2_SHIFT;
		static constexpr uint32_t ICIIT2_MASK  =EPSR_ICIIT2_MASK;
		static constexpr uint32_t Q            = static_cast<uint32_t>(APSR_FLAGS::Q);
		static constexpr uint32_t V            = static_cast<uint32_t>(APSR_FLAGS::V);
		static constexpr uint32_t C            = static_cast<uint32_t>(APSR_FLAGS::C);
		static constexpr uint32_t Z            = static_cast<uint32_t>(APSR_FLAGS::Z);
		static constexpr uint32_t N            = static_cast<uint32_t>(APSR_FLAGS::N);
	};

	enum class CTRL_FLAGS {
		FPCA	= (1<<2), // floating point context active
		SPSEL	= (1<<1), // active stack pointer 0 MSP 1 PSP
		PRIV	= (1<<0), // privilage level 0 = priv, 1 un
	};

	static inline CTRL_FLAGS get_control() {
		uint32_t ctrl;
		__asm__ volatile("mrs %0, control" : "=r"(ctrl) );
		return static_cast<CTRL_FLAGS>(ctrl);
	}

	static inline bool return_has_fp_context(uint32_t exec) { return (exec & EXC_RETURN_STD_CONTEXT) == 0; }

	#ifdef CONFIG_BUILD_PROTECTED
	static constexpr size_t SW_INT_REGS   = static_cast<size_t>(REG::R11)+2;
	#else
	static constexpr size_t SW_INT_REGS   = static_cast<size_t>(REG::R11)+1;
	#endif
	static constexpr size_t  SW_FPU_REGS    =  static_cast<size_t>(REG::R0)-SW_INT_REGS;
	static constexpr size_t  SW_XCPT_REGS      =  (SW_INT_REGS + SW_FPU_REGS);
	static constexpr size_t  SW_XCPT_SIZE      =  (4 * SW_XCPT_REGS);
	static constexpr size_t  HW_XCPT_REGS      =  (8);
	static constexpr size_t  HW_XCPT_SIZE      =  (4 * HW_XCPT_REGS);

	static constexpr size_t  XCPTCONTEXT_REGS  =  (HW_XCPT_REGS + SW_XCPT_REGS);
	static constexpr size_t  XCPTCONTEXT_SIZE  = (HW_XCPT_SIZE + SW_XCPT_SIZE);

#if 0
	//mrs	r12, psp		@ get the process stack
	//sub	sp, #S_FRAME_SIZE
	//stmia	sp, {r4-r12, lr}	@ push unsaved registers
	//ldmia	r12, {r0-r3, r6, r8-r10} @ load automatically saved registers
	//add	r12, sp, #S_R0
	//stmia	r12, {r0-r3, r6, r8-r10} @ fill in the rest of struct pt_regs
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

	__attribute__((always_inline)) static inline volatile uint32_t* save_context() {
		// ignore return
		__asm volatile(
		"    CPSID     I                 	\n" // Prevent interruption during context switch
		"    TST       LR, #4            	\n" // exc_return[2]=0? (it means that current is thread
		"    ITE       EQ                	\n" // has active floating point context)
		"    MRSEQ     R0, PSP           	\n" // PSP is process stack pointer
		"    MRSNE     R0, MSP           	\n" // MSP is process stack pointer
		"    TST       LR, #0x10         	\n" // exc_return[4]=0? (it means that current process
		"    ITTE      EQ                	\n" // has active floating point context)
		"    ADDEQ     R2, R0, #((8*16)*4)  \n" // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
		"    VSTMDBEQ  R0!, {S16-S31}    	\n" // if so - save it.
		"    ADDNE     R2, R0, #(8*4)  		\n" // SP is (HW_REG[8] + 32)
		"	 MRS       R3, BASEPRI			\n" // get the irq state
		"    STMDB     R0!, {R2-R11, LR} 	\n" // save remaining regs r4-11 and LR on process stack
		"    CPSIE     I                 	\n"
	 : : : );

	}
	// you can use this if you want to use the reg methiod of saving the context
	__attribute__((always_inline)) static inline void pendv_irq_context() {
		asm volatile (
			"    CPSID     I                 	\n" // Prevent interruption during context switch
			"    TST       LR, #4            	\n" // exc_return[2]=0? (it means that current is thread
			"    ITE       EQ                	\n" // has active floating point context)
			"    MRSEQ     R0, PSP           	\n" // PSP is process stack pointer
			"    MRSNE     R0, MSP           	\n" // MSP is process stack pointer
			"    TST       LR, #0x10         	\n" // exc_return[4]=0? (it means that current process
			"    ITTE      EQ                	\n" // has active floating point context)
			"    ADDEQ     R2, R0, #((8*16)*4)  \n" // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
			"    VSTMDBEQ  R0!, {S16-S31}    	\n" // if so - save it.
			"    ADDNE     R2, R0, #(8*4)  		\n" // SP is (HW_REG[8] + 32)
			"	 MRS       R3, BASEPRI			\n" // get the irq state
			"    STMDB     R0!, {R2-R11, LR} 	\n" // save remaining regs r4-11 and LR on process stack
			"    STMDB     R0!, {R2-R11, LR} 	\n" // save remaining regs r4-11 and LR on process stack
			// At this point, entire context of process has been saved
			"    LDR     R1, =os_context_switch_hook  \n"   // call os_context_switch_hook();
			"    BLX     R1                		\n"

			// R0 is new process SP;
			"    LDMIA     R0!, {R2-R11, LR} 	\n" // Restore r4-11 and LR from new process stack
			"    TST       LR, #0x10         	\n" // exc_return[4]=0? (it means that new process
			"    ITTE      EQ                	\n" // has active floating point context)
			"    VLDMIAEQ  R0!, {S16-S31}    	\n" // if so - restore it.
			"    SUBEQ     R0, R2, #((8*16)*4)  \n" // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
			"    SUBNE     R0, R2, #(8*4)  		\n" // SP is (HW_REG[8])
			"    TST       LR, #4            	\n" // exc_return[2]=0? (it means that current is thread
			"    ITE       EQ                	\n" // has active floating point context)
			"    MSREQ     PSP, R0            	\n" // PSP is process stack pointer
			"    MSRNE     MSP, R0            	\n" // MSP is process stack pointer
			"	 MSR   	   BASEPRI, R3 		    \n" // restore the base pri state
			"    CPSIE     I                 	\n"
			"    BX        LR      ");
	}
	__attribute__((always_inline)) static inline volatile uint32_t* save_context(volatile uint32_t** ctx) {
		// ignore return
		__asm volatile(
		"    CPSID     I                 	\n" // Prevent interruption during context switch
		"    TST       LR, #4            	\n" // exc_return[2]=0? (it means that current is thread
		"    ITE       EQ                	\n" // has active floating point context)
		"    MRSEQ     R12, PSP           	\n" // PSP is process stack pointer
		"    MRSNE     R12, MSP           	\n" // MSP is process stack pointer
		"    TST       LR, #0x10         	\n" // exc_return[4]=0? (it means that current process
		"    ITTE      EQ                	\n" // has active floating point context)
		"    ADDEQ     R2, R12, #((8*15)*4) \n" // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
		"    VSTMDBEQ  R12!, {S16-S31}    	\n" // if so - save it.
		"    ADDNE     R2, R12, #(8*4)  	\n" // SP is (HW_REG[8] + 32)
		"	 MRS       R3, BASEPRI			\n" // get the irq state
		"    STMDB     R12!, {R2-R11, LR} 	\n" // save remaining regs r4-11 and LR on process stack
		"    STR       R0, [R12]            \n" // save the state
		"    CPSIE     I                 	\n"
	 : : : "r0","r1","r2","r12");
#if 0
		__asm volatile("mov 	r0, %0" : : "r"(ctx) : "r0");
		__asm volatile("tst 	lr, #4");
		__asm volatile("ite 	eq");
		__asm volatile("mrseq 	r1, msp"::: "r1");
		__asm volatile("mrsne 	r1, psp"::: "r1");
		__asm volatile("add 	r2, r1, #32"::: "r2"); // before stack
		__asm volatile("mrs 	r3, basepri"::: "r3");
		__asm volatile("stmdb 	r1!, {r2-r11,lr}");		/* Save the remaining registers plus the SP value */
		__asm volatile("str 	r1, [r0]");		/* Save the remaining registers plus the SP value */
#endif
		// ignore return
	}
#pragma GCC diagnostic pop
	__attribute__((always_inline)) static inline void restore_context(volatile uint32_t** restore)  {
		// R0 is new process SP;
		__asm volatile(
		"    CPSID     I                 	\n" // Prevent interruption during context switch
		"    LDR       R12, [R0]			\n" // get the pointing stack
		"    LDMIA     R12!, {R2-R11, LR} 	\n" // Restore r4-11 and LR from new process stack
		"    TST       LR, #0x10         	\n" // exc_return[4]=0? (it means that new process
		"    ITTE      EQ                	\n" // has active floating point context)
		"    VLDMIAEQ  R12!, {S16-S31}    	\n" // if so - restore it.
		"    SUBEQ     R1, R12, #((8*15)*4) \n" // SP is (HW_REG[8] + LAZY[16] + MEFP[15])
		"    SUBNE     R1, R12, #(8*4)  	\n" // SP is (HW_REG[8])
		"    TST       LR, #4            	\n" // exc_return[2]=0? (it means that current is thread
		"    ITE       EQ                	\n" // has active floating point context)
		"    MSREQ     PSP, R1            	\n" // PSP is process stack pointer
		"    MSRNE     MSP, R1            	\n" // MSP is process stack pointer
		"	 MSR   	   BASEPRI, R3 		    \n" // restore the base pri state
		"    ADD       R1, #(7*4)*4			\n"
		"    STR       R1, [R0]				\n" // save the stack after the context return
		"    CPSIE     I                 	\n"
		);
	}
	// jumps to a context in thread
	__attribute__((always_inline)) static inline void jump_context(uint32_t* restore) {
		__asm volatile("mov sp, r0");
		__asm volatile("pop {r2-r11,lr}");		/* Recover R4-R11, r14 + 2 temp values */
		__asm volatile("pop {r0-r3, r12, lr, pc}");
		// this should work right?
	}
	/* This structure represents the return state from a system call */

	#ifdef CONFIG_LIB_SYSCALL
	struct xcpt_syscall_s
	{
	  uint32_t excreturn;   /* The EXC_RETURN value */
	  uint32_t sysreturn;   /* The return PC */
	};
	#endif
	// from scmios
#if 0
	template<uintptr_t addr, typename type = uint32_t>
	struct ioregister_t
	{
	    type operator=(type value) { *(volatile type*)addr = value; return value; }
	    void operator|=(type value) { *(volatile type*)addr |= value; }
	    void operator&=(type value) { *(volatile type*)addr &= value; }
	    operator type() { return *(volatile type*)addr; }
	};

	template<uintptr_t addr, class T>
	struct iostruct_t
	{
	    volatile T* operator->() { return (volatile T*)addr; }
	};
#endif





#if 0
		reg_t& operator=(const T value) { *_ptr = reinterpret_cast<uint32_t>(reinterpret_cast<void*>(&value)); return *this; }
		operator T&() { return reinterpret_cast<T>(reinterpret_cast<void*>(&_ptr)); }
		operator T() const { return reinterpret_cast<T>(reinterpret_cast<void*>(&_ptr)); }
		reg_t(volatile uint32_t& ptr) : _ptr(ptr) {}
		reg_t(uintptr_t ptr) : _ptr(reinterpret_cast<volatile uint32_t*>(ptr)) {}
	public:
		volatile uint32_t& _ptr;
	};
#endif
	static inline uint32_t rtoi(REG r) { return static_cast<uint32_t>(r); }
	// simple switcher
	typedef volatile uint32_t* (*os_hook_func)(volatile uint32_t*);
	void set_os_hook(os_hook_func func);
	static inline void raise_context_switch() { *((volatile uint32_t*)0xE000ED04) |= 0x10000000; }


	struct irq_state {
		template<typename T>
		static constexpr uint32_t hard_cast(T v) { return (uint32_t)((void*)v); }
		static constexpr size_t sw_reg_count = static_cast<size_t>(REG::EXC_RETURN)- static_cast<size_t>(REG::SP);
		static constexpr size_t hw_reg_count =  static_cast<size_t>(REG::XPSR) -static_cast<size_t>(REG::R0);
		static constexpr size_t total_reg_count =  sw_reg_count+hw_reg_count;

		inline volatile uint32_t& at(REG r) {
			assert(r < REG::MAX_STATE_SIZE);
			return r<=REG::EXC_RETURN
					? _sw_regs[static_cast<uint32_t>(r)- static_cast<uint32_t>(REG::SP)]
				    : _hw_regs[static_cast<uint32_t>(r)- static_cast<uint32_t>(REG::R0)];
		}
		inline uint32_t at(REG r)  const {
			assert(r < REG::MAX_STATE_SIZE);
			return r<=REG::EXC_RETURN
					? _sw_regs[static_cast<uint32_t>(r)- static_cast<uint32_t>(REG::SP)]
				    : _hw_regs[static_cast<uint32_t>(r)- static_cast<uint32_t>(REG::R0)];
		}
		inline uint32_t operator[](REG r) const { return at(r); }
		inline volatile uint32_t& operator[](REG r)  { return at(r); }

		__attribute__((always_inline)) inline void _save(irq_state* state) {
			//  register irq_state* reg0 __asm__("r0") = state;
			__asm__ __volatile__ (
				"mov 		r0, %0\n"
				"tst 		lr, #4\n" // exc_return[2]=0? (it means that current is thread
				"ite 		eq\n"
				"mrseq 		r2, msp\n"
				"mrsne 		r2, psp\n"
				"str 		r2, [r0, #0]\n" 		// save hw_regs
				"ldr		r1, [r0, #8]\n"			// load calls
				"cbnz		r1, 1f\n"				// if calls isn't zero skip sw_stack save
				"mov		sp, r2\n"				// move to stack now
				"tst	 	lr, #0x10\n"			// exc_return[4]=0? (it means that current process
				"it	 		eq\n"					// has active floating point context)
				"vstmdbeq 	sp!, {S16-S31}\n"		// if so, save it
				"stmdb 		sp!, {r2-r11, lr}\n"	// save other regs
				"mrs 		r3, basepri\n"
				"str 		sp, [r0, #4]\n"// save swregs
			//	"bic		sp, #7\n"			// 8 byte align the stack
				"1: add 	r1, #1\n"			// inc call
				"str 		r1, [r0, #8]\n"		// store call
				"str 		lr, [r0, #12]\n"	// save exlr
			 ::"r"(state) : "r0", "r2", "r3");
		}
		__attribute__((always_inline)) inline void _restore(irq_state* state) {
			 // register irq_state* reg0 __asm__("r0") = state;
			__asm__ __volatile__ (
				"mov 		r0, %0\n"
				//"ldr		r1, [r0, #8]\n"		// load calls
			//	"subs		r1, #1\n"				// decrment
			//	"bgt		1f\n"				// skip if not zero
				"ldr		r1, [r0, #4]\n" 		// get sw_regs
				"ldmia 		r1!, {r2-r11, lr}\n"	// restore the sw regs
				"tst	 	lr, #0x10\n"			// exc_return[4]=0? (it means that new process
				"it	 		eq\n"					// has active floating point context)
				"vldmiaeq 	r1!, {S16-S31}\n"	// if so - restore it.
				// r1, r2 and hw_regs should equal r2 at this point
				// we should be at the stack now but lets just use the pointer from the class
				"ldr		r1, [r0, #0]\n" 	// get hw_regs
				"tst 		lr, #4\n"
				"ite 		eq\n"
				"msreq    	msp, r2\n"
				"msrne 		psp, r2\n"
				"msr 		basepri, r3\n"
			//	"mov   		r1, #0\n"		// set calls to 0
			//	"1: str		r1, [r0, #8]\n" // save calls
			 ::"r"(state) : "r0", "r2", "r3");
		}
		__attribute__((always_inline)) inline void save(){
			_save(this); // hopefuly _calls is loaded into r0
		}

		__attribute__((always_inline)) inline void restore(){
			if(--_calls==0) {
				at(REG::EXC_RETURN) = _ret;	// be sure to use this
				_restore(this);
			} else {
				__asm__ __volatile__ (
					"mov 		r0, %0\n"
					"ldr		lr, [r0, #12]\n"  // load the return
					"ldr		sp, [r0, #0]\n"  // load stack
				::"r"(this) : "r0");
				_hw_regs+=hw_reg_count; // do we need this? this gets changed a bunch
			}
		}
		irq_state() : _hw_regs(nullptr),_sw_regs(nullptr),_calls(0) {}
		__attribute((naked)) static void do_call_return();

		// pushes the stack for a new call, we assume we don't need floating point
		void push_handler_call(uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2=0, uint32_t arg3=0);

		template<typename PC, typename ... Args>
		inline void push_handler_call(PC pc, Args ... args) { push_handler_call(hard_cast(pc),hard_cast<Args>(args)...); }

		void startup(uintptr_t sp, uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2=0, uint32_t arg3=0);
		reg_t<uint32_t*> stack() { return reg_t<uint32_t*>(at(REG::SP)); }
		reg_t<uint16_t*> inst() { return reg_t<uint16_t*>(at(REG::PC)); }
		operator  bool() const { return _hw_regs != nullptr; }
		bool operator==(const irq_state& other) const { return _hw_regs == other._hw_regs; }
		bool operator!=(const irq_state& other) const { return _hw_regs != other._hw_regs; }
	public:
		volatile uint32_t* _hw_regs;
		volatile uint32_t* _sw_regs;
		uint32_t _calls;
		ex_return _ret;
	};
	irq_state current_irq_state();
	__attribute((naked)) void switch_to(irq_state& from, irq_state& to);
	__attribute((naked)) void switch_to(irq_state& to);
};
template<>
struct enable_bitmask_operators<chip::CTRL_FLAGS>{
    static const bool enable=true;
};

#endif /* OS_REG_HPP_ */
