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
#include <os\reg.hpp>


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
	using REG = chip::REG;
	//using regs_t = chip::regs_t;

#if 0
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
#endif
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
		chip::irq_state regs;
		friend void __attribute__ (( naked ))PendSV_Handler();
		virtual context* schedule_select(); // override this for schedual select
		static void set_schduler(void(*func)(void*),void*arg);
		static context* current();
		// hard simple switch, returns null if there wasn't a pending switch
		static context*  switch_to(context* ctx);

		inline volatile uint32_t& at(REG i) { return regs[i]; }
		inline uint32_t at(REG i) const { return regs[i]; }

		uint32_t* stack() { return reinterpret_cast<uint32_t*>(at(REG::SP)); }

		static void context_error_return() {
			assert(0);
			while(1); // never should get here
		}
		uint32_t _call_arg(uint32_t i) {
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
		const volatile uint32_t& operator[](REG r) { return at(r); }
		uint32_t operator[](REG r) const { return at(r); }
		uint16_t last_instruction() const {
			//uint32_t *svc_param1 = reinterpret_cast<uint32_t*>(ctx.sp);
			//uint32_t svc_num = ((char *) svc_param1[static_cast<uint32_t>(REG::PC)])[-2];
			return * (int_to_ptr<uint16_t*>(at(REG::PC))-1);
		}
		uint8_t svc_number() const { return last_instruction() & 0xFF; }

		void set_user() {
			at(REG::EXC_RETURN) |= chip::EXC_RETURN_THREAD_MODE;
		}
		void set_kernel() {
			at(REG::EXC_RETURN) &= ~chip::EXC_RETURN_THREAD_MODE;
		}
		template<typename T=uint32_t>
		inline void set_return_value(T value) { at(REG::R0) =  int_to_ptr<T>(value); }

		template<typename SP>
		static uint32_t* push_std_context(SP sp_){
			uint32_t isp = ptr_to_int(sp_)- chip::XCPTCONTEXT_SIZE; // make space for the context
			return reinterpret_cast <uint32_t*>(isp);
		}
	//	template<typename T>   void set_reg(REG r, T value) {  at(r) = ptr_to_int(value); }
	//	template<typename PCT> void set_pc(PCT pc_){ at(REG::PC) = ptr_to_int(pc_); }
	//	template<typename LRT> void set_lr(LRT lr_){  at(REG::LR) = ptr_to_int(lr_);  }


		void push_context(); // we push the current context, make a copy of the lr
		void _init(uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2 =0){
			at(REG::EXC_RETURN) = chip::EXC_RETURN_BASE; // set up the return base
			at(REG::R12) = 0x0; // link
			at(REG::LR) = ptr_to_int(context_error_return); // set error return
			at(REG::PC) = pc;
			at(REG::XPSR) = 0x1000000; /* Thumb bit on */
			at(REG::R0) = arg0;
			at(REG::R1) = arg1;
			at(REG::R2) = arg2;
		}
		template<typename SP, typename PC> //typename ... Args>
		void init(SP sp_, PC pc_, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2 =0) {
			//regs = chip::irq_state(push_std_context(sp_));
		//	_init(ptr_to_int(pc_),arg0,arg1,arg2);
			//set_user(); // user is set by default
		}
		bool is_user() const { return (at(REG::EXC_RETURN) & chip::EXC_RETURN_THREAD_MODE) != 0; }
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
		}
		template<typename SP, typename PC, typename ... Args>
		context(SP sp_, PC pc, Args ... arg) : regs(push_std_context(sp_)){
			_init(ptr_to_int(pc), ptr_to_int(arg)...);
		}

		__attribute__((always_inline)) static inline uintptr_t get_current_sp() {
			register volatile uint32_t* _sp __asm("sp");
			return reinterpret_cast<uintptr_t>(_sp);
		}
		__attribute__((always_inline))  inline
		context() : regs() {}
		__attribute__((always_inline)) void  hard_jump() {
			// we restore the context, stack, eveything and then bx to it
			__hard_restore() ;
		}


		__attribute__((naked)) static void call_return();

		// we push a new call onto the stack and return the original
		void push_call(uintptr_t pc, uint32_t arg0=0, uint32_t arg1=0, uint32_t arg2=0);
		template<typename PC, typename ... ARGS>
		inline void push_call(PC pc, ARGS ... args) {
			push_call(ptr_to_int(pc), ptr_to_int(args)...);
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
			__restore();
			__asm__ __volatile__ ("bx lr");
		}
		template<typename PC>
		__attribute__((always_inline)) void init_ctx_switch(PC pc)  {
			__asm__ __volatile__ ("mov r1, %0" : : "r" (this->regs));
			__asm__ __volatile__ ("msr msp, r1");
			__asm__ __volatile__ ("mov r0, %0" : : "r" (ptr_to_int(pc)));
			__asm__ __volatile__ ("cpsie i");
			__asm__ __volatile__ ("bx r0");
		}
		__attribute__((always_inline)) void hard_restore() {
			restore();
			__asm__ __volatile__ ("bx lr");
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

		// restores the context, outside of an irq return
		// dosn't  check if we are in the right kernel thread
		__attribute__((always_inline)) void __hard_restore() {
			assert(0); // die
			//regs -= chip::XCPTCONTEXT_REGS;
			//chip::jump_context(sp+ chip::XCPTCONTEXT_REGS);
		}
		__attribute__((always_inline)) void __save() {
			regs.save();
		}
		__attribute__((always_inline)) void __restore() {
			regs.restore();
		}
	};

} /* namespace f9 */

#endif /* MIMIX_CPP_CONTEXT_HPP_ */
