
#ifndef F9_TYPES_HPP_
#define F9_TYPES_HPP_


#include "conf.hpp"

//#include <sys\types.h>
#include <climits>

#include "conf.hpp"
#include <cstdarg>
#include "link.hpp"

#include <sys\time.h>
#include <os\printk.h>
#include <os\bitmap.hpp>
#include <stm32f7xx.h>

extern "C" 	void panic(const char*,...);
extern "C" 	void printk(const char*,...);



namespace f9 {
	using irqn = ::IRQn_Type;
	// unfornaitnly the headerfiles for stm don't include the number of raw interrupts so we have to guess here
	constexpr size_t IRQ_COUNT = irqn::SPDIF_RX_IRQn +1;
	typedef uint32_t ptr_t;
	typedef uintptr_t memptr_t;

	typedef uint32_t thread_id_t; // or pid?
	constexpr static inline size_t ALIGNED(size_t size, size_t align) { return (size / align) + ((size & (align - 1)) != 0); }
	// helper to convert a pointer to a uintptr_t
	namespace priv {
	// silly cast
		template<typename FROM, typename TO>
		constexpr static inline TO _ptr_to_int(FROM v,std::true_type) { return reinterpret_cast<TO>(v); }
		template<typename FROM, typename TO>
		constexpr static inline TO _ptr_to_int(FROM v,std::false_type) { return static_cast<TO>(v); }

	};
	template<typename T>
	constexpr static inline uintptr_t ptr_to_int(T v) { return priv::_ptr_to_int<T,uintptr_t>(v,std::is_pointer<T>()); }
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

#if 0
	constexpr static inline typename std::enable_if<std::is_arithmetic<T>::value, uintptr_t>::type
									  ptr_to_int(T v) { return static_cast<uintptr_t>(v); }
	template<typename T>
	constexpr static inline uintptr_t ptr_to_int(T* v) { return reinterpret_cast<uintptr_t>(v); }
	constexpr static inline uintptr_t ptr_to_int(nullptr_t) { return 0; } // match null pointers

	// helper to convert a pointer to a uintptr_t
	template<typename T>
	constexpr static inline typename std::enable_if<std::is_arithmetic<T>::value, uintptr_t>::type
									  ptr_to_int(uintptr_t v) { return static_cast<uintptr_t>(v); }
	template<typename T>
	constexpr static inline uintptr_t ptr_to_int(T* v) { return reinterpret_cast<uintptr_t>(v); }
	constexpr static inline uintptr_t ptr_to_int(nullptr_t) { return 0; } // match null pointers
#endif



#define KTABLE_NAME(name) kt_## name
#define DECLARE_KTABLE(type, name, num_)			\
	static __KTABLE bitops::bitmap_table_t<type,num_> KTABLE_NAME(name); \
	void * type::operator new (size_t size){ return KTABLE_NAME(name).alloc(size); } \
	void type::operator delete (void * mem){  KTABLE_NAME(name).free(mem); }


	// one of the reasons I love C++ is becuase of stuff like this.

#if 0
#define DECLARE_KTABLE(type, name, num_)			\
	DECLARE_BITMAP(kt_ ## name ## _bitmap, num_);		\
	static __KTABLE type kt_ ## name ## _data[num_];	\
	ktable_t name = {					\
			.tname = #name,				\
			.bitmap = kt_ ## name ## _bitmap,	\
			.data = (ptr_t) kt_ ## name ## _data,	\
			.num = num_, .size = sizeof(type)	\
	}
#endif
	// debug here
	namespace dbg {
		 enum dbg_layer_t{
			DL_EMERG	= 0x0000,
			DL_BASIC	= 0x8000,
			DL_KDB 		= 0x4000,
			DL_KTABLE	= 0x0001,
			DL_SOFTIRQ	= 0x0002,
			DL_THREAD	= 0x0004,
			DL_KTIMER	= 0x0008,
			DL_SYSCALL	= 0x0010,
			DL_SCHEDULE	= 0x0020,
			DL_MEMORY	= 0x0040,
			DL_IPC		= 0x0080,
			DL_ALL		= 0xFFFF,
		} ;
		static constexpr  dbg_layer_t dbg_layer = DL_ALL;
		static inline void puts(const char* str) {
			printk("%s",str);
		}
		static inline void print(dbg_layer_t layer, const char* fmt,va_list va){

			if (layer != DL_EMERG && !(dbg_layer & layer))
				return;
			printk(fmt,va);
		}
		static inline void print(dbg_layer_t layer, const char* fmt,...){
			va_list va;
			va_start(va, fmt);
			print(layer, fmt, va);
			va_end(va);
		}
	};
};

#endif /* F9_FPAGE_HPP_ */
