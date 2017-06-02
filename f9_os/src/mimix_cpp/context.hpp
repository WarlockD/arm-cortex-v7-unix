/*
 * context.hpp
 *
 *  Created on: Jun 2, 2017
 *      Author: Paul
 */

#ifndef MIMIX_CPP_CONTEXT_HPP_
#define MIMIX_CPP_CONTEXT_HPP_


#include <cstdint>
#include <sys\types.h>
#include <climits>
#include <tuple>

#include <os\bitmap.hpp>
#include <sys\time.h>
#include <utility>

#include <os\printk.hpp>
#include <os\casting.hpp>



namespace f9 {
	// some base regesters for cortex so we don't have to include much
// f9 context with code
	// https://stackoverflow.com/questions/36066546/is-the-link-register-lr-affected-by-inline-or-naked-functions/36067265
	/*
	 * Intresting info about naked and always_inline. naked gurntess it will be a bl call so lr will have th return addre
	 * but will NOT emit the prolog so be sure to save lr if you need it
	 * Always inline dosn't do antying, it just inserts the entire function right where you call it at so its like a macro
	 * in that respect
	 */
	enum class REG : int {
		/* Saved by hardware */
		R0,
		R1,
		R2,
		R3,
		R12,
		LR,
		PC,
		xPSR,
		// regs saved by software
		BASEPRI=0,
		CONTROL,
		R4,
		R5,
		R6,
		R7,
		R8,
		R9,
		R10,
		R11,
		LR_RET,
		MAX
	};
	/* Disable/ enable hardware interrupts. The parameters of lock() and unlock()
		 * are used when debugging is enabled. See debug.h for more information.
		 */
		__attribute__( ( always_inline ) ) static inline void cli() { __asm volatile ("cpsie i" : : : "memory"); }
		__attribute__( ( always_inline ) ) static inline void sli() { __asm volatile ("cpsid i" : : : "memory"); }


		template<typename T> //, typename std::enable_if<std::is_convertible<T,uint32_t>::value>::type>
		__attribute__( ( always_inline ) ) static inline void save_flags(T& flags)
		{
				static_assert(std::is_convertible<T,uint32_t>::value, "WRONG SIZE!");
			  __asm volatile ("MRS %0, primask" : "=r" (flags) );
		}
		template<typename T>
		__attribute__( ( always_inline ) ) static inline void restore_flags(T flags)
		{
			  __asm volatile ("msr primask, %0" : : "r"(flags) );
		}
		__attribute__( ( always_inline ) )static inline uint32_t irq_number(void)
		{
			uint32_t irqno;
			__asm__ __volatile__ ( "mrs %0, ipsr" : "=r" (irqno) : );
			return irqno;
		}
		class irq_simple_lock {
			int _status;
		public:
			irq_simple_lock(bool start_disabled=true) {
				if(irq_number() == 0) { // if we are not in interrupt
					save_flags(_status);
					if(start_disabled) cli();
				} else _status = 2;
			}
			void enable() { if(_status < 2)  sli(); }
			void disable() { if(_status < 2) cli(); }
			~irq_simple_lock() {
				if(_status == 1)  // if we are not in interrupt
					restore_flags(_status);
			}
		};
		template<typename C, typename V>
		static inline void lock(C c, V v) { (void)c; (void)v; __asm volatile ("cpsid i" : : : "memory"); };
		template<typename C>
		static inline void unlock(C c) { (void)c; __asm volatile ("cpsie i" : : : "memory"); };
		// this is my generic context that I use for my tests


	struct context {
		enum state {
			LAZY_FP_SAVED = chip::EXC_RETURN_STD_CONTEXT,
			THREAD_MODE	  = chip::EXC_RETURN_STD_CONTEXT,
		};
		static constexpr size_t HW_CTX_REG_COUNT = static_cast<size_t>(REG::xPSR)-static_cast<size_t>(REG::R0); // count of regesters
		static constexpr size_t HW_CTX_REG_SIZE = HW_CTX_REG_COUNT * sizeof(uint32_t);//sizeof the regesters
		static constexpr size_t SW_CTX_REG_COUNT = static_cast<size_t>(REG::MAX)-static_cast<size_t>(REG::xPSR); // count of regesters
		static constexpr size_t SW_CTX_REG_SIZE = SW_CTX_REG_COUNT * sizeof(uint32_t);//sizeof the regesters
		static constexpr size_t TOTAL_CTX_REGS = HW_CTX_REG_COUNT + SW_CTX_REG_COUNT;
		static constexpr size_t TOTAL_CTX_SIZE = TOTAL_CTX_REGS * sizeof(uint32_t);

		uint32_t* sp;
		uint32_t regs[SW_CTX_REG_COUNT]; // reges saved by software
		friend void __attribute__ (( naked ))PendSV_Handler();
		virtual volatile context* schedule_select(); // override this for schedual select
		static void set_schduler(void(*func)(void*),void*arg);
		static volatile context* current();
		// hard simple switch, returns null if there wasn't a pending switch
		static volatile context*  switch_to(volatile context* ctx);

		size_t size() const { return regs - sp; }
		inline uint32_t& at(REG i) {
			return i>= REG::R0 && i <=REG::xPSR ?
					sp[static_cast<size_t>(i)- static_cast<size_t>(REG::R0)] :
					regs[static_cast<size_t>(i)- static_cast<size_t>(REG::xPSR)];
		}
		inline const uint32_t& at(REG i) const {
			return i>= REG::R0 && i <=REG::xPSR ?
					sp[static_cast<size_t>(i)- static_cast<size_t>(REG::R0)] :
					regs[static_cast<size_t>(i)- static_cast<size_t>(REG::xPSR)];
		}

		uint32_t* stack() {
			return (ret & LAZY_FP_SAVED) ? sp + HW_CTX_REG_COUNT + chip::HW_FP_COUNT   : sp + HW_CTX_REG_COUNT;
		} // stack befre context

		static void context_error_return() {
			assert(0);
			while(1); // never should get here
		}
		uint32_t& _call_arg(uint32_t i) {
			switch(i){
			case 0 : return at(REG::R0);
			case 1 : return at(REG::R1);
			case 2 : return at(REG::R2);
			case 3 : return at(REG::R3);
			default:
				return stack()[i-4];// the other args are on the stack
			}
		}
		template<typename T=uint32_t> inline T get_call_arg(int arg) { return int_to_ptr<T>(_call_arg(arg)); }
		template<typename T=uint32_t> inline void set_call_arg(int arg, T value) { _call_arg(arg) = ptr_to_int(value); }
		const uint32_t& operator[](REG r) { return at(r); }
		uint32_t operator[](REG r) const { return at(r); }
		uint16_t last_instruction() const {
			//uint32_t *svc_param1 = reinterpret_cast<uint32_t*>(ctx.sp);
			//uint32_t svc_num = ((char *) svc_param1[static_cast<uint32_t>(REG::PC)])[-2];
			return * (int_to_ptr<uint16_t*>(at(REG::PC))-1);
		}
		uint8_t svc_number() const { return last_instruction() & 0xFF; }

		void set_user() {
			at(REG::LR_RET) |= chip::EXC_RETURN_THREAD_MODE;
			at(REG::CONTROL)  = 0x0;
		}
		void set_kernel() {
			at(REG::LR_RET) &= ~chip::EXC_RETURN_THREAD_MODE;
			at(REG::CONTROL)  = 0x03;
		}
		template<typename T=uint32_t>
		inline void set_return_value(T value) { at(REG::R0) =  int_to_ptr<T>(value); }
		void set_regs(uint32_t* args, size_t count){
			std::copy(args,args + std::min(count,8u), reinterpret_cast<uint32_t*>(sp));
			if(count > 7) { // these are put on r4-r11.
				count-=8;
				args+=8;
				std::copy(args,args + std::min(count,7u), regs);
			}
		}


	//	template<typename T>   void set_reg(REG r, T value) {  at(r) = ptr_to_int(value); }
	//	template<typename PCT> void set_pc(PCT pc_){ at(REG::PC) = ptr_to_int(pc_); }
	//	template<typename LRT> void set_lr(LRT lr_){  at(REG::LR) = ptr_to_int(lr_);  }
		template<typename SP, typename PC> //typename ... Args>
		void init(SP sp_, PC pc_, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2 =0) {
			uint32_t isp = ptr_to_int(sp_)- TOTAL_CTX_SIZE; // make space for the context
			sp = reinterpret_cast <uint32_t*>(ptr_to_int(sp_));
			regs = sp + chip::HW_REGS_COUNT;
			at(REG::LR_RET) = chip::EXC_RETURN_BASE; // set up the return base
			set_user(); // user is set by default
			at(REG::R12) = 0x0; // link
			at(REG::LR) = ptr_to_int(context_error_return); // set error return
			at(REG::PC) = ptr_to_int(pc_);
			at(REG::xPSR) = 0x1000000; /* Thumb bit on */
			at(REG::R0) = arg0;
			at(REG::R1) = arg1;
			at(REG::R2) = arg2;
		}
		template<typename SP>
		static uint32_t* push_std_context(SP sp_){
			uint32_t isp = ptr_to_int(sp_)- TOTAL_CTX_SIZE; // make space for the context
			return reinterpret_cast <uint32_t*>(ptr_to_int(sp_));
		}
		void push_context(); // we push the current context, make a copy of the lr
		void _init(uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2 =0){
			at(REG::LR_RET) = chip::EXC_RETURN_BASE; // set up the return base
			at(REG::R12) = 0x0; // link
			at(REG::LR) = ptr_to_int(context_error_return); // set error return
			at(REG::PC) = pc;
			at(REG::xPSR) = 0x1000000; /* Thumb bit on */
			at(REG::R0) = arg0;
			at(REG::R1) = arg1;
			at(REG::R2) = arg2;
		}
		bool is_user() const { return (at(REG::LR_RET) & chip::EXC_RETURN_THREAD_MODE) != 0; }
		template<typename PC, typename ... Args>
		void init_user(PC pc, Args ... arg) {
			_init(ptr_to_int(pc), ptr_to_int(arg)...);
			set_user() ;
		}
		template<typename PC, typename ... Args>
		void init_priv(PC pc, Args ... arg) {
			_init(ptr_to_int(pc), ptr_to_int(arg)...);
			set_kernel(); // user is set by default
		}
		template<typename PC, typename ... Args>
		void init(PC pc, Args ... arg) {
			_init(ptr_to_int(pc), ptr_to_int(arg)...);
			if(__get_CONTROL() == 0x3)  // we are a unprivliaged thread
				set_user() ;
			else
				set_kernel(); // user is set by default
		}
		template<typename SP, typename PC, typename ... Args>
		context(SP sp_, PC pc, Args ... arg) : sp(push_std_context(sp_)), regs(sp + chip::HW_REGS_COUNT) {
			_init(ptr_to_int(pc), ptr_to_int(arg)...);
		}

		__attribute__((always_inline)) static inline uintptr_t get_current_sp() {
			register volatile uint32_t* _sp __asm("sp");
			return reinterpret_cast<uintptr_t>(_sp);
		}
		__attribute__((always_inline))  inline
		context() : sp(reinterpret_cast<uint32_t*>(get_current_sp())), regs(nullptr) {}

		__attribute__((always_inline)) void  hard_jump() {
			// we restore the context, stack, eveything and then bx to it
			__hard_restore() ;
		}

		static __attribute__((nakad))  void push_call_return( void(*func)(void*), void* arg,context*ctx ) {
		//	__asm__ __volatile__ ("push {lr}");
			func(arg);
		//	__asm__ __volatile__ ("pop {lr}");
			ctx->__hard_restore();

			//__asm__ __volatile__ ("bx lr");
		}
		__attribute__((naked)) static void call_return();

		// we push a new call onto the stack and return the original
		void push_call(uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2=0);
		template<typename PC, typename ... ARGS>
		inline void push_call(PC pc, ARGS ... args) {
			push_call(ptr_to_int(pc), ptr_to_int(args)...);
		}
		// clone context using stack as a base
		// we are assuming the stack has been copyed already
		context clone(uintptr_t stack) const {
			context ret = *this;
			ret.sp = stack;
			return ret;
		}
	#ifdef CONFIG_FPU
		uint64_t fp_regs[8];
		uint32_t fp_flag;
		__attribute__((always_inline)) void save() {
			__asm__ __volatile__ ("cpsid i");
			this->fp_flag = 0;
			__save(ctx);
			__asm__ __volatile__ ("tst lr, 0x10");
			__asm__ __volatile__ ("bne no_fp");
			__asm__ __volatile__ ("mov r3, %0"
					: : "r" (this->fp_regs) : "r3");
			__asm__ __volatile__ ("vstm r3!, {d8-d15}"
					::: "r3");
			__asm__ __volatile__ ("mov r4, 0x1": : :"r4");
			__asm__ __volatile__ ("stm r3, {r4}");
			__asm__ __volatile__ ("no_fp:");
		}
		__attribute__((always_inline)) void restore() {
			__restore();
		if ((ctx)->fp_flag) {
			__asm__ __volatile__ ("mov r0, %0"
					: : "r" ((ctx)->fp_regs): "r0");
			__asm__ __volatile__ ("vldm r0, {d8-d15}");
		}								\
		__asm__ __volatile__ ("cpsie i");
		}
	#endif
		__attribute__((always_inline)) void save() {
			__asm__ __volatile__ ("cpsid i");
			__save();
		}
		__attribute__((always_inline)) void restore() {
			__restore();
			__asm__ __volatile__ ("cpsie i");
		}
		__attribute__((always_inline)) void restore_return() {
			restore();
			__asm__ __volatile__ ("bx lr");
		}
		template<typename PC>
		__attribute__((always_inline)) void init_ctx_switch(PC pc)  {
			__asm__ __volatile__ ("mov r1, %0" : : "r" (this->sp));
			__asm__ __volatile__ ("msr msp, r1");
			__asm__ __volatile__ ("mov r0, %0" : : "r" (ptr_to_int(pc)));
			__asm__ __volatile__ ("cpsie i");
			__asm__ __volatile__ ("bx r0");
		}

private:
#if 0
		R0=0,
		R1,
		R2,
		R3,
		R12,
		LR,
		PC,
		xPSR,
#endif
#define REG_TOSTR(V) #V
#define REG_R0 	REG_TOSTR(0*4)
#define REG_R1 	REG_TOSTR(1*4)
#define REG_R2 	REG_TOSTR(2*4)
#define REG_R3 	REG_TOSTR(3*4)
#define REG_R12 REG_TOSTR(4*4)
#define REG_LR 	REG_TOSTR(5*4)
#define REG_PC 	REG_TOSTR(6*4)
#define REG_PSR REG_TOSTR(7*4)
		// restores the context, outside of an irq return
		// dosn't  check if we are in the right kernel thread
		__attribute__((always_inline)) void __hard_restore() {
			__asm__ __volatile__ ("mov r0, %0" : : "r" (this->regs));
			__asm__ __volatile__ ("ldm r0, {r4-r11}");  // restore the easy stuff
			__asm__ __volatile__ ("mov sp, %0" : : "r" (this->sp));
			__asm__ __volatile__ ("ldr r0, [sp, #" REG_PSR  "]" : : : "r0"); // r0 = xPSR
			__asm__ __volatile__ ("msr psr, r0");	 // restore the psr
			__asm__ __volatile__ ("ldr r0, [sp, #" REG_PC  "]" : : : "r0"); // r0 = pc
			__asm__ __volatile__ ("str r0, [sp, #" REG_PSR  "]" : : : "r0"); // copy it for pop
			__asm__ __volatile__ ("ldmia sp, {r0-r3, r12, lr }");	// restore the temp regs
			__asm__ __volatile__ ("add sp, #(7*4)");			// rstore the oringal stack
			__asm__ __volatile__ ("pop { pc } ");			//  branch!
		}
		__attribute__((always_inline)) void __save() {
			__asm__ __volatile__ ("mov r0, %0": : "r" (this->regs) : "r0");
			__asm__ __volatile__ ("mrseq r2, basepri"::: "r2");
			__asm__ __volatile__ ("mrseq r3, control"::: "r3");
			__asm__ __volatile__ ("stm r0, {r2-r11, lr}");
			__asm__ __volatile__ ("tst lr, #4");
			__asm__ __volatile__ ("mrseq r0, msp"::: "r0");
			__asm__ __volatile__ ("mrsne r0, psp"::: "r0");
			__asm__ __volatile__ ("mov %0, r0" : "=r" (this->sp));
		}
		__attribute__((always_inline)) void __restore() {
			__asm__ __volatile__ ("mov r0, %0": : "r" (this->regs) : "r0");
			__asm__ __volatile__ ("ldm r0, {r2-r11, lr}");
			__asm__ __volatile__ ("msr basepri, r2");
			__asm__ __volatile__ ("msr control, r3");
			__asm__ __volatile__ ("mov r0, %0" : : "r" (this->sp));
			__asm__ __volatile__ ("tst lr, #4");
			__asm__ __volatile__ ("msreq msp, r0");
			__asm__ __volatile__ ("msrne psp, r0");
		}
	};

} /* namespace f9 */

#endif /* MIMIX_CPP_CONTEXT_HPP_ */
