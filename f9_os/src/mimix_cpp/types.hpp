#ifndef TYPE_H
#define TYPE_H

#include <cstdint>
#include <sys\types.h>
#include <climits>

#include <tuple>

#include "config.hpp"

#include <os\bitmap.hpp>
#include <sys\time.h>
#include <utility>

#include <os\printk.hpp>


// combo of lite bsd and a few links here
	// https://sourceforge.net/p/fixedptc/code/ci/default/tree/fixedptc.h
	// and a paper called 2013_cppfp_paper_osc.pdf

namespace priv{
	static constexpr size_t DEFAULT_FPINT_SHIFT = 11;
	template<typename T,typename R=T> using fixpt_valid = std::enable_if<std::is_arithmetic<T>::value,R>;
}
template<size_t _FSHIFT=priv::DEFAULT_FPINT_SHIFT>
struct fixpt_t { // simple fixed point math for load averages
	static constexpr size_t FSHIFT = _FSHIFT;
	static constexpr size_t FSCALE = 1<<FSHIFT;
	constexpr fixpt_t() : _raw(0) {}
    template<typename T,
	typename priv::fixpt_valid<T>::type* = nullptr>
    constexpr fixpt_t(const T n) : _raw(static_cast<int>(n* FSCALE)) {};
    static constexpr fixpt_t ONE = fixpt_t(1);
    static constexpr fixpt_t ZERO = fixpt_t(0);

	template<size_t PP>
	constexpr fixpt_t(const fixpt_t<PP>& x) : _raw(PP> FSHIFT ?  (_raw>>(PP-FSHIFT)):  (_raw<<(FSHIFT-PP))) {}


	//template<typename T>
	//typename priv::fixpt_valid<T>::type
	//constexpr operator T() const  { return static_cast<T>(_raw) / static_cast<T>(FSCALE); }

	constexpr operator int32_t() const {return static_cast<int32_t>(_raw) / FSCALE; }
	constexpr operator uint32_t() const {return static_cast<uint32_t>(_raw) / FSCALE; }
	constexpr operator float() const {return static_cast<float>(_raw)/ FSCALE; }
	constexpr operator double() const {return static_cast<double>(_raw)/ FSCALE; }
	template<size_t PP>
	constexpr operator fixpt_t<PP>() const { return PP> FSHIFT ?  (_raw>>(PP-FSHIFT)):  (_raw<<(FSHIFT-PP)); }
	/** Assignment of int */
	template<typename T,typename priv::fixpt_valid<T>::type* = nullptr>
	constexpr fixpt_t& operator=(const T& x){ _raw = x*FSCALE; return *this;}

	//constexpr fixpt_t& operator=(const int& x){ _raw = x*FSCALE; return *this;}
	//constexpr fixpt_t& operator=(const float& x){ _raw = x*FSCALE; return *this;}
	template<unsigned PP>
	constexpr inline  fixpt_t& operator=(const fixpt_t<PP>& x){ PP> FSHIFT ?  (x._raw>>(PP-FSHIFT)):  (x._raw<<(FSHIFT-PP)); return *this;}

	constexpr inline  fixpt_t& operator+=(const fixpt_t& x){_raw -= x._raw; return *this;}
	constexpr inline  fixpt_t& operator-=(const fixpt_t& x){_raw -= x._raw; return *this;}
	constexpr inline  fixpt_t& operator*=(const fixpt_t& x){
		int64_t v = _raw;	// expensive?
		v*= x._raw;
		v >>= FSHIFT;
		_raw = static_cast<int>(v);
		return *this;
	}
	constexpr inline  fixpt_t& operator/=(const fixpt_t& x){
		int64_t v = _raw;	// expensive?
		v <<= FSHIFT;
		v/= x._raw;
		_raw = static_cast<int>(v);
		return *this;
	}
	constexpr inline  fixpt_t& operator<<=(const uint32_t& x){ _raw <<= x; return *this; }
	constexpr inline  fixpt_t& operator>>=(const uint32_t& x){ _raw >>= x; return *this; }

	template<typename T> constexpr inline typename priv::fixpt_valid<T,fixpt_t&>::type
		operator+=(const T& x){_raw -= fixpt_t(x); return *this;}
	template<typename T> constexpr inline typename priv::fixpt_valid<T,fixpt_t&>::type
		operator-=(const T& x){_raw -= fixpt_t(x); return *this;}
	template<typename T> constexpr inline typename priv::fixpt_valid<T,fixpt_t&>::type
		operator*=(const T& x){_raw *= fixpt_t(x); return *this;}
	template<typename T> constexpr inline typename priv::fixpt_valid<T,fixpt_t&>::type
		operator/=(const T& x){_raw *= fixpt_t(x); return *this;}

	// logicial
	bool operator==(const fixpt_t& x) const{ return _raw == x._raw; }
	bool operator!=(const fixpt_t& x) const{ return _raw != x._raw; }
	bool operator<(const fixpt_t& x) const{ return _raw < x._raw; }
	bool operator>(const fixpt_t& x) const{ return _raw > x._raw; }
	bool operator<=(const fixpt_t& x) const{ return _raw <= x._raw; }
	bool operator>=(const fixpt_t& x) const{ return _raw >= x._raw; }
	int raw() const { return _raw; }
private:
	int _raw;
};

#define fixpt_t_math_builder(OP) \
template<size_t _FSHIFT,typename T> static constexpr inline typename priv::fixpt_valid<T,fixpt_t<_FSHIFT>>::type \
	operator OP(const T& l, 				const fixpt_t<_FSHIFT>& r){ fixpt_t<_FSHIFT> ret(l); ret OP##= r; return ret; } \
template<size_t _FSHIFT,typename T> static constexpr inline typename priv::fixpt_valid<T,fixpt_t<_FSHIFT>>::type \
	operator OP(const fixpt_t<_FSHIFT>& l, const T& r)				  { fixpt_t<_FSHIFT> ret(l); ret OP##= r; return ret; } \
template<size_t _FSHIFT> static constexpr inline fixpt_t<_FSHIFT> \
	operator OP(const fixpt_t<_FSHIFT>& l, const fixpt_t<_FSHIFT>& r) { fixpt_t<_FSHIFT> ret(l); ret OP##= r; return ret; }

#define fixpt_t_logic_builder(OP) \
template<size_t _FSHIFT,typename T> static constexpr inline typename priv::fixpt_valid<T,bool>::type \
	operator OP(const T& l, 				const fixpt_t<_FSHIFT>& r){ return fixpt_t<_FSHIFT>(l) OP r; } \
template<size_t _FSHIFT,typename T> static constexpr inline typename priv::fixpt_valid<T,bool>::type \
	operator OP(const fixpt_t<_FSHIFT>& l, const T& r)				  { return l OP fixpt_t<_FSHIFT>(r); } \

fixpt_t_math_builder(*)
fixpt_t_math_builder(/)
fixpt_t_math_builder(+)
fixpt_t_math_builder(-)
fixpt_t_math_builder(>>)
fixpt_t_math_builder(<<)
fixpt_t_logic_builder(<)
fixpt_t_logic_builder(>)
fixpt_t_logic_builder(==)
fixpt_t_logic_builder(!=)
fixpt_t_logic_builder(>=)
fixpt_t_logic_builder(<=)

namespace mimx {
constexpr static inline size_t ALIGNED(size_t size, size_t align) { return (size / align) + ((size & (align - 1)) != 0); }
// helper to convert a pointer to a uintptr_t
namespace priv {
// case c++17 isn't out yet:(
	template <size_t ...I>
	struct index_sequence {};

	template <size_t N, size_t ...I>
	struct make_index_sequence : public make_index_sequence<N - 1, N - 1, I...> {};

	template <size_t ...I>
	struct make_index_sequence<0, I...> : public index_sequence<I...> {};
// https://stackoverflow.com/questions/16893992/check-if-type-can-be-explicitly-converted
// better idea for a cast
template<typename From, typename To>
struct _is_explicitly_convertible
{
    template<typename T>
    static void f(T);

    template<typename F, typename T>
    static constexpr auto test(int) ->
    decltype(f(static_cast<T>(std::declval<F>())),true) { return true; }

    template<typename F, typename T>
    static constexpr auto test(...) -> bool {  return false; }

    static bool constexpr value = test<From,To>(0);
};
template<typename From, typename To>
struct is_explicitly_convertible : std::conditional<_is_explicitly_convertible<From,To>::value,std::true_type, std::false_type>::type {
    static bool constexpr value = _is_explicitly_convertible<From,To>::value;
    using type = typename std::conditional<_is_explicitly_convertible<From,To>::value,std::true_type, std::false_type>::type;

};
// silly cast
	template<typename T>
	struct cast_info {
		using in_type = typename std::remove_reference<T>::type;
		constexpr static bool is_pointer = std::is_pointer<in_type>::value || std::is_array<in_type>::value;
		using type = typename std::conditional<std::is_pointer<in_type>::value,typename  std::remove_pointer<in_type>::type,in_type>::type;
		constexpr static bool is_intergral = std::is_integral<in_type>::value;
	};
	template<typename FROM>
	constexpr static inline uintptr_t __ptr_to_uint(FROM v,std::true_type) { return reinterpret_cast<uintptr_t>(v); }
	template<typename FROM>
	constexpr static inline uintptr_t __ptr_to_uint(FROM v,std::false_type) { return static_cast<uintptr_t>(v); }
	template<typename FROM>
	constexpr static inline uintptr_t _ptr_to_uint(FROM v) { return __ptr_to_uint(v,std::is_pointer<FROM>()); }
	template<typename FROM, typename TO>
	constexpr static inline TO _ptr_to_int(FROM v,std::true_type) { return reinterpret_cast<TO>(v); }
	template<typename FROM, typename TO>
	constexpr static inline TO _ptr_to_int(FROM v,std::false_type) { return static_cast<TO>(v); }

	template<typename FROM, typename TO>
	constexpr static inline TO __ptr_to_int(FROM&& v,std::false_type) { return reinterpret_cast<TO>(v); }
	template<typename FROM, typename TO>
	constexpr static inline TO __ptr_to_int(FROM&& v,std::true_type) { return static_cast<TO>(v); }

	//template<typename T>
	//constexpr static inline uintptr_t to_uintptr_t(T v) { return _ptr_to_int<T,uintptr_t>(v,std::is_pointer<T>()); }

	template<typename T>
	constexpr static inline uintptr_t to_uintptr_t(T&& v) { return __ptr_to_int<T,uintptr_t>(v,is_explicitly_convertible<T,uintptr_t>()); }
	template<typename T,typename P>
	constexpr static inline P to_pointer(T&& v) { return __ptr_to_int<T,uintptr_t>(v,
			typename std::conditional<std::is_pointer<P>::value,
			std::false_type,
			typename std::conditional<is_explicitly_convertible<T,uintptr_t>::value,
				std::true_type,
				std::false_type>::type>::type


			{}); }
#if 0
	template <typename... T>
	auto remove_ref_from_tuple_members(std::tuple<T...> const& t) {
	    return std::tuple<typename std::remove_reference<T>::type...>{ t };
	}
	template <class Tuple, size_t... Is>
	constexpr auto _to_uintptr(Tuple t, index_sequence<Is...>) {
		return std::make_tuple(_ptr_to_uint(std::get<Is>(t))... );

		//return std::tie(_ptr_to_uint(std::get<Is>(t)) ...);
	}
#endif
	template<typename T>
	constexpr auto _to_uintptr_truple(T&& v) { return std::make_tuple(_ptr_to_uint(v)); }
	template<typename T,typename ... Args>
	constexpr auto _to_uintptr_truple(T&& v, Args ... args) { return std::make_tuple(_ptr_to_uint(v), _to_uintptr_truple(std::forward<Args>(args)...)); }
	template <typename ... Args>
	constexpr auto  to_uintptr_truple(Args ... args) { return _to_uintptr_truple(std::forward<Args>(args)...); }
};
//template<typename T>
//constexpr static inline uintptr_t to_uintptr_t(T v) { return _ptr_to_int<T,uintptr_t>(v,std::is_pointer<T>()); }

template<typename T>
constexpr static inline uintptr_t ptr_to_int(T v) { return priv::_ptr_to_int<T,uintptr_t>(std::forward<T>(v),std::is_pointer<T>()); }


template<typename T>
constexpr static inline T int_to_ptr(uintptr_t v) { return priv::_ptr_to_int<uintptr_t,T>(v,std::is_pointer<T>()); }
//template<typename T>
constexpr static inline uintptr_t ptr_to_int(nullptr_t) { return uintptr_t{}; }
// helper to convert a pointer to a uintptr_t
template<typename T>
constexpr static inline typename std::enable_if<std::is_arithmetic<T>::value, void*>::type
to_voidp(T v) { return static_cast<void*>(v); }
template<typename T>
constexpr static inline void*
to_voidp(T* v) { return reinterpret_cast<void*>(v); }
	typedef uint32_t reg_t;
	typedef int irq_id_t;

#if 0
	typedef uint32_t bitchunk_t; /* collection of bits in a bitmap */
	/* Constants and macros for bit map manipulation. */
	constexpr static size_t BITCHUNK_BITS  =  (sizeof(bitchunk_t) * 8);


	constexpr static inline bitchunk_t& MAP_CHUNK(bitchunk_t *map, size_t bit) { return map[bit/BITCHUNK_BITS]; }
	constexpr static inline size_t CHUNK_OFFSET(size_t bit) { return bit % BITCHUNK_BITS; }
	constexpr static size_t BITMAP_CHUNKS(size_t bits) { return  ((bits+BITCHUNK_BITS-1)/BITCHUNK_BITS); }
	constexpr static inline bool GET_BIT(bitchunk_t * map, size_t bit) { return (MAP_CHUNK(map,bit) & (1 << CHUNK_OFFSET(bit))) != 0;  }
	static inline void SET_BIT(bitchunk_t * map, size_t bit) { MAP_CHUNK(map,bit) |= (1 << CHUNK_OFFSET(bit)); }
	static inline void UNSET_BIT(bitchunk_t * map, size_t bit) { MAP_CHUNK(map,bit) &= ~(1 << CHUNK_OFFSET(bit)); }
#endif

	constexpr static size_t  NR_SYS_CHUNKS	= bitops::bitmap_t<mimx::NR_SYS_PROCS>::WORDCOUNT;
	// constants

	constexpr static char IDLE_Q		 = 15;    /* lowest, only IDLE process goes here */
	constexpr static size_t NR_SCHED_QUEUES  = IDLE_Q+1;	/* MUST equal minimum priority + 1 */
	constexpr static char TASK_Q		=   0;	/* highest, used for kernel tasks */
	constexpr static char MAX_USER_Q  =	   0;    /* highest priority for user processes */
	constexpr static char USER_Q  	 =  7;    /* default (should correspond to nice 0) */
	constexpr static char MIN_USER_Q	 = 14;	/* minimum priority for user processes */
	/* Process table and system property related types. */
	typedef int proc_nr_t;			/* process table entry number */
	typedef short sys_id_t;			/* system process index */
	typedef uintptr_t phys_bytes;

	/* Process table and system property related types. */
	typedef int proc_nr_t;			/* process table entry number */
	typedef short sys_id_t;			/* system process index */
	struct sys_map_t : public bitops::bitmap_t<NR_SYS_PROCS> {};
	struct boot_image {
	  proc_nr_t proc_nr;			/* process number to use */
	 // task_t *initial_pc;			/* start function for tasks */
	  int flags;				/* process flags */
	  unsigned char quantum;		/* quantum (tick count) */
	  int priority;				/* scheduling priority */
	  int stksize;				/* stack size for tasks */
	  short trap_mask;			/* allowed system call traps */
	 // bitchunk_t ipc_to;			/* send mask protection */
	  long call_mask;			/* system call protection */
	  char proc_name[P_NAME_LEN];		/* name in process table */
	};

	struct memory {
	  size_t base;			/* start address of chunk */
	  size_t size;			/* size of memory chunk */
	};
	// special case cause eveything is fucked
	struct hw_trap {
		reg_t r0;
		reg_t r1;
		reg_t r2;
		reg_t r3;
		reg_t ip;
		reg_t lr;
		reg_t pc;
		reg_t xpsr;
#ifdef CONFIG_LAZY_FLOAT
		uint32_t fp[16];
#endif

	} __attribute__((aligned(8))) ;
	static constexpr size_t HW_TRAP_SIZE = sizeof(hw_trap);
	static constexpr size_t HW_TRAP_COUNT = HW_TRAP_SIZE/sizeof(reg_t);
	struct sw_trap {
		//uint32_t sp;
		//uint32_t basepri;
		reg_t r4;
		reg_t r5;
		reg_t r6;
		reg_t r7;
		reg_t r8;
		reg_t r9;
		reg_t r10;
		reg_t r11;
		uint32_t ret; // ex lr
	};
	static constexpr size_t  SW_TRAP_SIZE = sizeof(sw_trap);
	static constexpr size_t  SW_TRAP_COUNT = SW_TRAP_SIZE/sizeof(reg_t);
	static constexpr size_t  TRAP_SIZE = SW_TRAP_SIZE + HW_TRAP_SIZE;
	static constexpr size_t  TRAP_COUNT = HW_TRAP_COUNT+SW_TRAP_COUNT;
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
		R0=0,
		R1,
		R2,
		R3,
		R12,
		LR,
		PC,
		xPSR,
		R4,
		R5,
		R6,
		R7,
		R8,
		R9,
		R10,
		R11,
		SP,
		CONTROL,
		LR_RET
	};
	struct f9_context_t {
		static constexpr size_t RESERVED_REGS = static_cast<size_t>(REG::xPSR) +1; // count of regesters
		static constexpr size_t RESERVED_SIZE = RESERVED_REGS * sizeof(uint32_t);//sizeof the regesters
		static void context_error_return() {
			assert(0);
			while(1); // never should get here
		}
		uint32_t sp=0;
		uint32_t ret=0;
		uint32_t ctl=0;
		uint32_t regs[8];

		uint32_t& _call_arg(uint32_t i) {
			if(i < 4)
				return at(i);
			else { // the other args are on the stack
				return stack_before_context()[i-4];
			}
		}
		template<typename T=uint32_t>
		inline T get_call_arg(int arg) { return int_to_ptr<T>(_call_arg(arg)); }
		template<typename T=uint32_t>
		inline void set_call_arg(int arg, T value) { _call_arg(arg) = ptr_to_int(value); }

		inline const uint32_t at(int i) const {
			if(i < 8){
				uint32_t* ret = reinterpret_cast<uint32_t*>(sp);
				return ret[i];
			} else
				return regs[i-8];
		}
		inline const uint32_t at(REG r) const {
			switch(r){
				case REG::SP: 		return sp;
				case REG::CONTROL: 	return ctl;
				case REG::LR_RET: 	return ret;
				default:
					return at(static_cast<int>(r));
			}
		}
		inline  uint32_t& at(int i)  {
			if(i < 8){
				uint32_t* ret = reinterpret_cast<uint32_t*>(sp);
				return ret[i];
			} else
				return regs[i-8];
		}
		inline  uint32_t& at(REG r)  {
			switch(r){
				case REG::SP: 		return sp;
				case REG::CONTROL: 	return ctl;
				case REG::LR_RET: 	return ret;
				default:
					return at(static_cast<int>(r));
			}
		}
		uint32_t& operator[](REG r) { return at(r); }
		uint32_t operator[](REG r) const { return at(r); }
		uint32_t& operator[](int i) { return at(i); }
		uint32_t operator[](int i) const { return at(i); }
		uint16_t last_instruction() const {

			//uint32_t *svc_param1 = reinterpret_cast<uint32_t*>(ctx.sp);
			//uint32_t svc_num = ((char *) svc_param1[static_cast<uint32_t>(REG::PC)])[-2];
			return * (int_to_ptr<uint16_t*>(at(REG::PC))-1);
		}
		uint8_t svc_number() const { return last_instruction() & 0xFF; }
		uint32_t* stack_before_context() {
			return reinterpret_cast<uint32_t*>(sp) + RESERVED_REGS;
		}
		const uint32_t* stack_before_context() const {
			return reinterpret_cast<uint32_t*>(sp) + RESERVED_REGS;
		}
		void set_user() {
			ret = 0xFFFFFFF9;
			ctl = 0x0;
		}
		void set_kernel() {
			ret = 0xFFFFFFFD;
			ctl = 0x03;
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
		//template<class T, size_t I>
		//inline typename std::enable_if<I == sizeof...(Ts), void>::type
		//		_init_set_arg(T) {}
		template<class T, size_t I>
		inline typename std::enable_if<I >= 4, void>::type
			_init_set_arg(T&& v) {
				uint32_t* stack = reinterpret_cast<uint32_t*>(sp);
				stack[RESERVED_REGS+I-4] = ptr_to_int(v);
				sp = ptr_to_int(stack);
		}
		template<class T, size_t I>
		inline typename std::enable_if<I < 4, void>::type
			_init_set_arg(T&& v) {
				at(I) = ptr_to_int(v);
		}


	//	template<typename T>   void set_reg(REG r, T value) {  at(r) = ptr_to_int(value); }
	//	template<typename PCT> void set_pc(PCT pc_){ at(REG::PC) = ptr_to_int(pc_); }
	//	template<typename LRT> void set_lr(LRT lr_){  at(REG::LR) = ptr_to_int(lr_);  }
		template<typename SP, typename PC> //typename ... Args>
		void init(SP sp_, PC pc_) {
			sp = ptr_to_int(sp);
		//	static constexpr size_t arg_count = sizeof...(Args);
			sp -= RESERVED_REGS * sizeof(uint32_t);
		//	if(arg_count >=4) sp-= (arg_count * sizeof(uint32_t))-4; // reserve more args
		//	init_arg(priv::to_uintptr_truple(std::forward<Args>(args)...)); //std::forward<Args>(args)...));

			set_user(); // user is set by default
			at(REG::R12) = 0x0; // link
			at(REG::LR) = ptr_to_int(context_error_return); // set error return
			at(REG::PC) = ptr_to_int(pc_);
			at(REG::xPSR) = 0x1000000; /* Thumb bit on */
		}
		template<typename SP, typename PC, typename Args> //typename ... Args>
		void init(SP sp_, PC pc_, Args arg) {
			init(sp_,pc_);
			set_call_arg(0, arg);
		}
		template<typename SP, typename PC>
		void init_priv(SP sp_, PC pc_, uint32_t* regs_= nullptr) {
			init(sp_,pc_,regs_);
			set_kernel(); // user is set by default
		}
		template<typename SP, typename PC>
		void init_user(SP sp_, PC pc_, uint32_t* regs_= nullptr) {
			init(sp_,pc_,regs_);
		}
		f9_context_t() {}

		template<typename SP, typename PC, typename Args>  //... Args>
		f9_context_t(SP sp_, PC pc_, Args  arg) { init(sp_,pc_, arg); }

		template<typename SP, typename PC>
		f9_context_t(SP sp_, PC pc_) { init(sp_,pc_); }
		//f9_context_t() { init(0,0); }

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
			__asm__ __volatile__ ("cpsid i");			\
			__save();
		}
		__attribute__((always_inline)) void restore() {
			__restore();
			__asm__ __volatile__ ("cpsie i");
		}
		__attribute__((always_inline)) void init_ctx_switch(uintptr_t pc)  {
			__asm__ __volatile__ ("mov r1, %0" : : "r" (this->sp));
			__asm__ __volatile__ ("msr msp, r1");
			__asm__ __volatile__ ("mov r0, %0" : : "r" (pc));
			__asm__ __volatile__ ("cpsie i");
			__asm__ __volatile__ ("bx r0");
		}
private:
		__attribute__((always_inline)) void __save() {
			__asm__ __volatile__ ("mov r0, %0"
					: : "r" (this->regs) : "r0");
			__asm__ __volatile__ ("stm r0, {r4-r11}");
			__asm__ __volatile__ ("and r4, lr, 0xf":::"r4");
			__asm__ __volatile__ ("teq r4, #0x9");
			__asm__ __volatile__ ("ite eq");
			__asm__ __volatile__ ("mrseq r0, msp"::: "r0");
			__asm__ __volatile__ ("mrsne r0, psp"::: "r0");
			__asm__ __volatile__ ("mov %0, r0" : "=r" (this->sp));
			__asm__ __volatile__ ("mov %0, lr" : "=r" (this->ret));
			__asm__ __volatile__ ("mrs r0, control"::: "r0");
			__asm__ __volatile__ ("mov %0, r0" : "=r" (this->ctl));
		}
		__attribute__((always_inline)) void __restore() {
			__asm__ __volatile__ ("mov lr, %0" : : "r" (this->ret));
			__asm__ __volatile__ ("mov r0, %0" : : "r" (this->sp));
			__asm__ __volatile__ ("mov r2, %0" : : "r" (this->ctl));
			__asm__ __volatile__ ("and r4, lr, 0xf":::"r4");
			__asm__ __volatile__ ("teq r4, #0x9");
			__asm__ __volatile__ ("ite eq");
			__asm__ __volatile__ ("msreq msp, r0");
			__asm__ __volatile__ ("msrne psp, r0");
			__asm__ __volatile__ ("mov r0, %0" : : "r" (this->regs));
			__asm__ __volatile__ ("ldm r0, {r4-r11}");
			__asm__ __volatile__ ("msr control, r2");
		}
	};

	struct stackframe_s {
		union {
			struct {
				sw_trap sw;
				hw_trap hw;
			} r;
			uint32_t regs[TRAP_COUNT];
		};
		stackframe_s() {}
#define GETSETREG(FROM, NAME) \
		inline uint32_t& NAME() { return FROM.NAME; }\
		inline const uint32_t& NAME() const { return FROM.NAME; } \
		template<typename T> inline const void set_##NAME(const T v) { FROM.NAME = static_cast<reg_t>(v); } \
		template<typename T> inline const void set_##NAME(const T* v) { FROM.NAME = reinterpret_cast<reg_t>(v); }

#define GETSETREGA(NAME,ALLIAS) \
		inline uint32_t& ALLIAS() { return NAME; }\
		inline const uint32_t& ALLIAS() const { return NAME; }
		GETSETREG(r.hw,r0)
		GETSETREG(r.hw,r1)
		GETSETREG(r.hw,r2)
		GETSETREG(r.hw,r3)
		GETSETREG(r.hw,ip)
		GETSETREG(r.hw,lr)
		GETSETREG(r.hw,pc)
		GETSETREG(r.hw,xpsr)
		GETSETREG(r.sw,r4);
		GETSETREG(r.sw,r5);
		GETSETREG(r.sw,r6);
		GETSETREG(r.sw,r7);
		GETSETREG(r.sw,r8);
		GETSETREG(r.sw,r9);
		GETSETREG(r.sw,r10);
		GETSETREG(r.sw,r11);
		GETSETREG(r.sw,ret);
#undef GETSETREG
#undef GETSETREGA

		static constexpr reg_t PSR_V_BIT	=0x10000000;
		static constexpr reg_t PSR_C_BIT	=0x20000000;
		static constexpr reg_t PSR_Z_BIT	=0x40000000;
		static constexpr reg_t PSR_N_BIT	=0x80000000;

		void dump()
		{
			printk(" r0: %p  r1: %p  r2: %p  r3: %p  r4: %p\r\n", r0(),r1(), r2(),r3(),r4());
			printk(" r5: %p  r6: %p  r7: %p  r8: %p  r9: %p\r\n", r5(),r6(), r7(),r8(),r9());
			printk("r10: %p r11: %p  ip: %p  pc: %p  lr: %p\r\n", r10(),r11(), ip(),pc(),lr());
			printk(" sp: %p ret: %p\r\n",reinterpret_cast<uint32_t>(this), ret());
			char buf[5];
			buf[0] = xpsr() & PSR_N_BIT ? 'N' : 'n';
			buf[1] = xpsr() & PSR_Z_BIT ? 'Z' : 'z';
			buf[2] = xpsr() & PSR_C_BIT ? 'C' : 'c';
			buf[3] = xpsr() & PSR_V_BIT ? 'V' : 'v';
			buf[4] = '\0';
			printk(" xpsr: %s\r\n", buf);
		}
	} ;//__attribute__((aligned(8)));

	/* The kernel outputs diagnostic messages in a circular buffer. */
	struct kmessages {
	  int km_next;				/* next index to write */
	  int km_size;				/* current size in buffer */
	  char km_buf[KMESS_BUF_SIZE];		/* buffer for messages */
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
		irq_simple_lock(bool start_disabled=false) {
			if(irq_number() == 0) { // if we are not in interrupt
				save_flags(_status);
				if(start_disabled) cli();
			}
		}
		void enable() { sli(); }
		void disable() { cli(); }
		~irq_simple_lock() { restore_flags(_status); }
	};
	template<typename C, typename V>
	static inline void lock(C c, V v) { (void)c; (void)v; __asm volatile ("cpsid i" : : : "memory"); };
	template<typename C>
	static inline void unlock(C c) { (void)c; __asm volatile ("cpsie i" : : : "memory"); };

	/* Sizes of memory tables. The boot monitor distinguishes three memory areas,
	 * namely low mem below 1M, 1M-16M, and mem after 16M. More chunks are needed
	 * for DOS MINIX.
	 */
	constexpr static size_t  NR_MEMS    =        8;
	// software test and set
	static inline bool test_and_set(bool& value) { bool ret = value; value = true; return ret; }

};
#endif
