/*
 * bitmap.h
 *
 *  Created on: Apr 22, 2017
 *      Author: warlo
 */

#ifndef XV6CPP_BITMAP_H_
#define XV6CPP_BITMAP_H_

#include <cstdint>
#include <cstddef>
#include <limits>
#include<type_traits>
#include <functional>
#include <cassert>
#include <memory>
#include <array>

#include <os\printk.h>
#include <sys\queue.h>
#include <os\atomic.h>



// from https://www.justsoftwaresolutions.co.uk/files/bitmask_operators.hpp
#if 0
// example
enum class A{
    x=1,y=2
       };

enum class B:unsigned long {
    x=0x80000000,y=0x40000000
        };

template<>
struct enable_bitmask_operators<A>{
    static const bool enable=true;
};

template<>
struct enable_bitmask_operators<B>{
    static const bool enable=true;
};

enum class C{x,y};

#endif
template<typename E>
struct enable_bitmask_operators{
    static const bool enable=false;
};

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator|(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

// we use the % operator for checking if the flag is set
template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,bool>::type
operator%(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return (static_cast<underlying>(lhs) & static_cast<underlying>(rhs)) != 0;
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator&(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator^(E lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E>::type
operator~(E lhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
        ~static_cast<underlying>(lhs));
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E&>::type
operator|=(E& lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E&>::type
operator&=(E& lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,E&>::type
operator^=(E& lhs,E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    lhs=static_cast<E>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    return lhs;
}

template<typename E>
typename std::enable_if<enable_bitmask_operators<E>::enable,typename std::underlying_type<E>::type>::type
bitmask_cast(E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<underlying>(rhs);
}

// define below to make this nifty
//#define CONFIG_BITMAP_BITBAND
// well fuck me.  apperntly bitband is not on the f7
// caues hard faults that have been pissing me off
// bit map functions for the cortex m
namespace bitops {
template<typename T, size_t _BIT_POS, size_t _BIT_SIZE>
struct bit_helper {
	static_assert(_BIT_SIZE > 0, "BITSIZE IS ZERO!");
	static constexpr size_t BIT_POS = _BIT_POS;
	static constexpr size_t BIT_SIZE= _BIT_SIZE;
	static constexpr T BIT_SIZE_MASK = static_cast<T>(-1) >> ((sizeof(T) * 8) - _BIT_SIZE);
	static constexpr T BIT_MASK = BIT_SIZE_MASK << BIT_POS;
	template<typename A>
	static constexpr T mask(const A val) { return ((static_cast<T>(val)&BIT_SIZE_MASK) << BIT_POS); }
	template<typename A>
	static void set(T& val, A arg) { val = (val & BIT_SIZE_MASK) | mask(arg); }
	static T get(const T val) { return (val & BIT_SIZE_MASK)  >> BIT_POS; }

	class type {
		T& _raw;
	public:
		type(T& raw) : _raw(raw) {}
		operator T() const { return get(_raw); }
		template<typename A>
		type& operator=(const A val) { set(_raw,val); return *this; }
	};

};
#if 0
#define BITBAND_SRAM_REF   0x20000000
#define BITBAND_SRAM_BASE  0x22000000
#define BITBAND_SRAM(a,b) ((BITBAND_SRAM_BASE + ((a-BITBAND_SRAM_REF)<<5) + (b<<2)))  // Convert SRAM address
/* Bit band PERIPHERAL definitions */
#define BITBAND_PERI_REF   0x40000000
#define BITBAND_PERI_BASE  0x42000000
#define BITBAND_PERI(a,b) ((BITBAND_PERI_BASE + ((a-BITBAND_PERI_REF)<<5) + (b<<2)))  // Convert PERI address
/* Basic bit band function definitions */
#define BITBAND_SRAM_ClearBit(a,b) (*(volatile uint32_t *) (BITBAND_SRAM(a,b)) = 0)
#define BITBAND_SRAM_SetBit(a,b) (*(volatile uint32_t *) (BITBAND_SRAM(a,b)) = 1)
#define BITBAND_SRAM_GetBit(a,b) (*(volatile uint32_t *) (BITBAND_SRAM(a,b)))
#define BITBAND_PERI_ClearBit(a,b) (*(volatile uint32_t *) (BITBAND_PERI(a,b)) = 0)
#define BITBAND_PERI_SetBit(a,b) (*(volatile uint32_t *) (BITBAND_PERI(a,b)) = 1)
#define BITBAND_PERI_GetBit(a,b) (*(volatile uint32_t *) (BITBAND_PERI(a,b)))

#endif

#ifdef CONFIG_BITMAP_BITBAND
static constexpr uintptr_t BITBAND_SRAM_REF  = 0x20000000;
static constexpr uintptr_t BITBAND_SRAM_BASE  = 0x22000000;
static constexpr uintptr_t BITBAND_SRAM(uintptr_t a,uintptr_t b)
	{ return ((BITBAND_SRAM_BASE + ((a-BITBAND_SRAM_REF)<<5) + (b<<2))); }  // Convert SRAM address	;

static constexpr volatile uint32_t* BITBAND_SRAM(uint32_t* a,size_t bit){
	uintptr_t addr = reinterpret_cast<uintptr_t>(a);
	assert(addr>= BITBAND_SRAM_REF && addr < BITBAND_SRAM_BASE); // sanity check
	return reinterpret_cast<volatile uint32_t*>(BITBAND_SRAM(addr,bit));
}

#endif
	template<typename _ENUM>
	class enum_flag {
	public:
		static_assert(std::is_enum<_ENUM>::value, "NOT AN ENUM!");
		using enum_type = _ENUM;
		using type = enum_flag<enum_type>;
		using underlying = typename std::underlying_type<enum_type>::type;
		static inline constexpr underlying cast(enum_type e) { return static_cast<underlying>(e); }
		static inline constexpr enum_type cast(underlying e) { return static_cast<enum_type>(e); }

		type& operator&=(const type e) { _flags = cast(cast(_flags) & cast(e._flags)); return *this; }
		type& operator|=(const type e) { _flags = cast(cast(_flags) | cast(e._flags)); return *this; }
		type operator~() const { return type(cast(~cast(_flags)));  }

		bool operator==(const type e) const { return _flags == e._flags; }
		bool operator!=(const type e) const { return _flags != e._flags; }


		operator enum_type() const { return _flags; }
		explicit enum_flag(const _ENUM e) :_flags(e) {}
		enum_flag() : _flags{} {}
	private:
		enum_type _flags;
	};
	template<typename T> static inline enum_flag<T> operator|(enum_flag<T> l, enum_flag<T> r) { enum_flag<T> a(l); a |= r; return a; }
	template<typename T> static inline enum_flag<T> operator&(enum_flag<T> l, enum_flag<T> r) { enum_flag<T> a(l); a &= r; return a; }

	uint32_t test_and_set_bit(uint32_t *word, int bitmask);
	uint32_t test_and_set_word(uint32_t *word);
	// 	/* Bit map related macros. lightbsd
	template<typename T, typename std::enable_if<std::is_arithmetic<T>{}>::type =0>
	static inline size_t bitsizeof() { return sizeof(T) * 8; }


	template<typename T, typename U, typename std::enable_if<std::is_arithmetic<T>{}>::type =0>
	static inline void setbit(T* a, const U i)  { a[i/bitsizeof<T>()] |= (1<<(i%bitsizeof<T>())); }
	template<typename T, typename U, typename std::enable_if<std::is_arithmetic<T>{}>::type =0>
	static inline void clrbit(T* a, const U i)  { a[i/bitsizeof<T>()] &= ~(1<<(i%bitsizeof<T>())); }
	template<typename T, typename U, typename std::enable_if<std::is_arithmetic<T>{}>::type =0>
	static constexpr inline bool isset(const T* a, const U i)  { return (a[i/bitsizeof<T>()] & (1<<(i%bitsizeof<T>()))) != 0; }
	template<typename T, typename U, typename std::enable_if<std::is_arithmetic<T>{}>::type =0>
	static constexpr inline bool isclr(const T* a, const U i)  { return (a[i/bitsizeof<T>()] & (1<<(i%bitsizeof<T>()))) == 0; }

	template<typename T, typename std::enable_if<std::is_enum<T>{}>::type =0>
	static inline void setflag(T& a, const T i)  {
		using UT = typename std::underlying_type<T>::type;
		a = static_cast<T>(static_cast<UT>(a) | static_cast<UT>(i));
	}
	template<typename T, typename std::enable_if<std::is_enum<T>{}>::type =0>
	static inline void clrflag(T& a, const T i)  {
		using UT = typename std::underlying_type<T>::type;
		a = static_cast<T>(static_cast<UT>(a) & ~static_cast<UT>(i));
	}
	template<typename T>
	typename std::enable_if<std::is_enum<T>{},T>::type
	static inline constexpr  maskflag(const T a,  const T i)  {
		using UT = typename std::underlying_type<T>::type;
		return static_cast<T>(static_cast<UT>(a) & static_cast<UT>(i));
	}
	template<typename T>
	typename std::enable_if<std::is_enum<T>{},bool>::type
	static inline constexpr isflagset(const T a, const T i)  {
		using UT = typename std::underlying_type<T>::type;
		return static_cast<UT>(a) & static_cast<UT>(i) != 0;
	}
	template<typename T, typename std::enable_if<std::is_enum<T>{}>::type =0>
	static inline constexpr bool isflagclr(const T a, const T i)  {
		using UT = typename std::underlying_type<T>::type;
		return static_cast<UT>(a) & static_cast<UT>(i) == 0;
	}
	/* Macros for counting and rounding. */
	template<typename T, typename U>
	static constexpr inline T howmany(const T x, const U y)  { return (x+(y-1))/y; }
	template<typename T, typename U>
	static constexpr inline T roundup(const T x, const U y)  { return ((x+(y-1))/y)*y; }
	template<typename T>
	static constexpr inline bool powerof2(const T x)  { return ((x-1)&x)==0; }
	template<typename T>
	static constexpr inline T max(const T a, const T b)  { return a>b?a:b; }
	template<typename T>
	static constexpr inline T min(const T a, const T b)  { return  a<b?a:b; }


#if 0
	/*
	 * ARMv6 UP and SMP safe atomic ops.  We use load exclusive and
	 * store exclusive to ensure that these are atomic.  We may loop
	 * to ensure that the update happens.
	 */
	// fix these with enable if to check type sizes
	template<typename I, typename T>
	inline void atomic_add(I i, T *v)
	{
		T tmp;
		int result;
		__asm__ __volatile__("@ atomic_add\n"
				"1:	ldrex	%0, [%2]\n"
				"	add	%0, %0, %3\n"
				"	strex	%1, %0, [%2]\n"
				"	teq	%1, #0\n"
				"	bne	1b"
		: "=&r" (result), "=&r" (tmp)
		: "r" (v), "Ir" (i)
		: "cc");
	}
	template<typename I, typename T>
	inline T atomic_add_return(I i, T *v)
	{
		T tmp;
		T result;
		__asm__ __volatile__("@ atomic_add_return\n"
				"1:	ldrex	%0, [%2]\n"
				"	add	%0, %0, %3\n"
				"	strex	%1, %0, [%2]\n"
				"	teq	%1, #0\n"
				"	bne	1b"
		: "=&r" (result), "=&r" (tmp)
		: "r" (v), "Ir" (i)
		: "cc");
		return result;
	}
	template<typename I, typename T>
	inline void atomic_sub(I i, T *v)
	{
		T tmp;
		T result;
		__asm__ __volatile__("@ atomic_sub\n"
				"1:	ldrex	%0, [%2]\n"
				"	sub	%0, %0, %3\n"
				"	strex	%1, %0, [%2]\n"
				"	teq	%1, #0\n"
				"	bne	1b"
		: "=&r" (result), "=&r" (tmp)
		: "r" (v), "Ir" (i)
		: "cc");
	}
	template<typename I, typename T>
	inline T atomic_sub_return(I i, T *v)
	{
		T tmp;
		T result;
		__asm__ __volatile__("@ atomic_sub_return\n"
				"1:	ldrex	%0, [%2]\n"
				"	sub	%0, %0, %3\n"
				"	strex	%1, %0, [%2]\n"
				"	teq	%1, #0\n"
				"	bne	1b"
		: "=&r" (result), "=&r" (tmp)
		: "r" (v), "Ir" (i)
		: "cc");
		return result;
	}
	template<typename I1, typename I2,typename T>
	inline T atomic_cmpxchg(T *ptr, I1 oldv,I2 newv)
	{
		T oldval, res;
		do {
			__asm__ __volatile__("@ atomic_cmpxchg\n"
			"ldrex	%1, [%2]\n"
			"mov	%0, #0\n"
			"teq	%1, %3\n"
			"strexeq %0, %4, [%2]\n"
			    : "=&r" (res), "=&r" (oldval)
			    : "r" (ptr), "Ir" (oldv), "r" (newv)
			    : "cc");
		} while (res);
		return oldval;
	}
	template<typename T>
	inline void atomic_clear_mask(T mask, T *addr)
	{
		T tmp, tmp2;

		__asm__ __volatile__("@ atomic_clear_mask\n"
	"1:	ldrex	%0, [%2]\n"
	"	bic	%0, %0, %3\n"
	"	strex	%1, %0, [%2]\n"
	"	teq	%1, #0\n"
	"	bne	1b"
		: "=&r" (tmp), "=&r" (tmp2)
		: "r" (addr), "Ir" (mask)
		: "cc");
	}
#endif

	class simple_lock {
		uint32_t _lock;
	public:
		simple_lock() : _lock(0) {}
		void wait() const {
			// the simplest wait is ot wait for an interrupt
				__asm volatile("wfi" : :: "memory");
		}
		void lock() {
			while(test_and_set_word(&_lock)) wait();
		}
		void unlock() { _lock = 0; }
		bool locked() const { return _lock != 0; }
	};
#ifdef CONFIG_BITMAP_BITBAND
	template<size_t _BITS>
	class bitchunk_t {
	public:
		using bitword_t = uint32_t;
		constexpr static size_t BITCHUNK_BITS  =  (sizeof(bitword_t) * 8);
		constexpr static size_t BIT_COUNT = _BITS;
		constexpr static size_t WORD_COUNT = ((BIT_COUNT+BITCHUNK_BITS-1)/BITCHUNK_BITS);
		constexpr static size_t BYTE_COUNT = WORD_COUNT / 8;
		static constexpr uint32_t BITBAND_ADDR_SHIFT = 5;
		static constexpr uint32_t BIT_SHIFT = 2;
		/* Generate address in bit-band region */
		inline static constexpr uint32_t* ADDR_BITBAND(void*addr) {
			return reinterpret_cast<uint32_t*>(
					0x22000000 + (((reinterpret_cast<uintptr_t>(addr) & 0xFFFFF) << BITBAND_ADDR_SHIFT))
			);
		}
		void debug_print() {
			for(auto& a : _map) {
				printk("%p[%b] ",a,a);
			}
			printk(": ");
			for(size_t b=0;b < BIT_COUNT;b++){
				if(_bitmap[b] == 1) printk("1"); else printk("0");
			}
			printk("\r\n");
		}
		bitchunk_t(): _bitmap(ADDR_BITBAND(_map.data())) {}
		inline constexpr size_t size() const { return BIT_COUNT; }
		inline constexpr size_t max_size() const { return BIT_COUNT; }
		bool empty() const {
			for(auto i : _map) if(i) return false;
			return true;
		}
		inline bitword_t& operator[](int i) { return _bitmap[i]; }
		inline const bitword_t& operator[](int i) const { return _bitmap; }
		inline bitword_t* begin() { return _bitmap; }
		inline bitword_t* end() { return _bitmap+BIT_COUNT; }
		inline void set(int bit) { _bitmap[bit] = 1; }
		inline void clear(int bit) { _bitmap[bit] = 0; }
		inline bool is_set(int bit) const { return _bitmap[bit] != 0; }
		inline bool is_clear(int bit) const { return _bitmap[bit] == 0; }
		inline bool test_and_set(int bit) { bitword_t word = _bitmap[bit]; _bitmap[bit] = 1; return word != 0; }
		inline uint32_t first_clear_index() {
			size_t idx = 0;
			for(size_t w =0; w < WORD_COUNT; w++){
				if(_map[w] == 0) return (w * BITCHUNK_BITS);
				size_t idx=__builtin_clz(_map[w]);
				return (w * BITCHUNK_BITS) + idx;
			}
			return BIT_COUNT;
		}
		inline bitword_t* first_clear_bit() {
			return &_bitmap[ first_clear_index()];
		}
		inline bool set_first_clear_bit(uint32_t& index) {
			do {
				index = first_clear_index();
				if(index ==BIT_COUNT) return false;
			} while(cmpxchg(_bitmap[index],0,1) == 0);
			return true;
		}
		bool operator==(const bitchunk_t& r) const { return _map == r._map; }
		bool operator!=(const bitchunk_t& r) const { return !(*this==r); }
	private:

		alignas(4) std::array<bitword_t,WORD_COUNT> _map;
		uint32_t* _bitmap;
		static bool cmpxchg(uint32_t& addr, bitword_t old_value, bitword_t new_value) {
			if(addr != old_value) return false;
			addr = new_value;
			return true;
		}
	};
#endif
		// remember this can only go into sram somewhere
#ifdef CONFIG_BITMAP_BITBAND
	/*
	 * soo a quick description how this works, we cannot save a uint32_t pointer because we would skip
	 * 4 bits when we tried something like _bit++.  this all does a bit of fuckery as, from
	 * the machine level, evey time we save a word to this addres, we are setting a bit at another one
	 * but for us, we have to find the address of the bit with a bunch of math and because of c++
	 * using reinterpret_cast
	 */
	static constexpr uint32_t BITBAND_ADDR_SHIFT = 5;
	static constexpr uint32_t BIT_SHIFT = 2;
	/* Generate address in bit-band region */
	inline static constexpr uint32_t* ADDR_BITBAND(void*addr) {
		return reinterpret_cast<uint32_t*>(
				0x22000000 + (((reinterpret_cast<uintptr_t>(addr) & 0xFFFFF) << BITBAND_ADDR_SHIFT))
		);
	}
	static constexpr uintptr_t BITBAND_SRAM_REF  = 0x20000000;
	static constexpr uintptr_t BITBAND_SRAM_BASE  = 0x22000000;
	static constexpr uintptr_t BITBAND_SRAM(uintptr_t a,uintptr_t b)
		{ return ((BITBAND_SRAM_BASE + ((a-BITBAND_SRAM_REF)<<5) + (b<<2))); }  // Convert SRAM address	;

	static constexpr uint32_t* BITBAND_SRAM(volatile uint32_t* a,uint32_t b=0){
		uintptr_t addr = reinterpret_cast<uintptr_t>(a);
		assert(addr>= BITBAND_SRAM_REF && addr < BITBAND_SRAM_BASE); // sanity check
		return reinterpret_cast<uint32_t*>(BITBAND_SRAM(addr,b));
	}
#else
	static constexpr size_t BITMAP_ALIGN = 32;
	static constexpr inline uint32_t BITOFF(const uint32_t bit)  { return (bit % BITMAP_ALIGN); }		/* bit offset inside 32-bit word */
	static constexpr inline uint32_t BITMASK(const uint32_t bit)  { return (1 << BITOFF(bit)); }	 /* Mask used for bit number N */
	static constexpr inline uint32_t BITINDEX(const uint32_t bit)  { return (bit / BITMAP_ALIGN); }	/* Bit index in bitmap array */
	static inline uint32_t* BITWORD(uint32_t* map, uint32_t bit)  { return &map[BITINDEX(bit)]; }
	static inline const uint32_t* BITWORD(const uint32_t* map, uint32_t bit)  { return &map[BITINDEX(bit)]; }

	static inline void BIT_SET(uint32_t* map, uint32_t bit) { *BITWORD(map,bit) |= BITMASK(bit); }
	static inline void BIT_CLEAR(uint32_t* map, uint32_t bit) { *BITWORD(map,bit) &= ~BITMASK(bit); }
	static inline bool BIT_TEST_AND_SET(uint32_t* map, uint32_t bit) {
		return test_and_set_bit(BITWORD(map,bit),BITOFF(bit) ) !=0;
	}
	static inline uint32_t BIT_GET(const uint32_t* map, const uint32_t bit) { return *BITWORD(map,bit) & BITMASK(bit); }
	// simple bitmap
	template<size_t _BITCOUNT>
	struct bit_set {
		using mask_type = uint32_t;
		static constexpr size_t NFDBITS = sizeof(mask_type) * 8;
		static constexpr size_t SETSIZE = _BITCOUNT;
		mask_type _bits[howmany(SETSIZE, NFDBITS)];
		inline void set(mask_type n) { _bits[n/NFDBITS] |= (1 << (n % NFDBITS)); }
		inline void clr(mask_type n) { _bits[n/NFDBITS] &= ~(1 << (n % NFDBITS)); }
		inline bool isset(mask_type n) { return (_bits[n/NFDBITS] & (1 << (n % NFDBITS))) != 0; }
		inline void zero() { std::fill_n(_bits, sizeof(_bits), 0); }
	};

#endif
	class bitmap_cursor_t {
#ifdef CONFIG_BITMAP_BITBAND

		uint32_t* _bit; // better if this was a uint32_t?
	public:
		inline bitmap_cursor_t() : _bit(nullptr) {}
		inline bool get() const { return *_bit != 0; }
		inline void set() { *_bit =  1; }
		inline void clear() { *_bit = 0; }
		inline bool test_and_set() { return ARM::atomic_test_and_set(_bit); }
		inline bitmap_cursor_t(uint32_t* bitmap, uint32_t bit) : _bit(BITBAND_SRAM(bitmap,bit)){}
		inline uint32_t  id() const {
			uintptr_t p = reinterpret_cast<uintptr_t>(_bit);
			return (p & ((1 << (BITBAND_ADDR_SHIFT + BIT_SHIFT)) - 1))  >> BIT_SHIFT;
		}
		inline bool operator==(const bitmap_cursor_t& r) const { return _bit == r._bit; }
		inline bool operator!=(const bitmap_cursor_t& r) const { return _bit != r._bit; }
		inline bool operator>(const bitmap_cursor_t& r) const { return _bit > r._bit; }
		inline bool operator<(const bitmap_cursor_t& r) const { return _bit < r._bit; }
		inline bool operator>=(const bitmap_cursor_t& r) const { return _bit >= r._bit; }
		inline bool operator<=(const bitmap_cursor_t& r) const { return _bit <= r._bit; }
#else
		inline void _next() { _bit++; }
		inline void _prev() { _bit--; }
		uint32_t* _bitmap;
		uint32_t _bit;
	public:
		inline bool get() const { return BIT_GET(_bitmap,_bit) !=0; }
		inline void set() {  BIT_SET(_bitmap,_bit); }
		inline void clear() { BIT_CLEAR(_bitmap,_bit); }
		inline bool test() const  { return _bit != 0; }
		inline bool test_and_set() { return  test_and_set_bit(_bitmap,_bit); }
		inline uint32_t id() const { return _bit; }
		inline bitmap_cursor_t(uint32_t* bitmap, uint32_t bit) : _bitmap(bitmap),_bit(bit) {}
		inline bool operator==(const bitmap_cursor_t& r) const { return _bitmap == r._bitmap && _bit == r._bit; }
		inline bool operator!=(const bitmap_cursor_t& r) const { return !(*this == r); }
		inline bool operator>(const bitmap_cursor_t& r) const { return _bitmap == r._bitmap && _bit > r._bit; }
		inline bool operator<(const bitmap_cursor_t& r) const { return _bitmap == r._bitmap && _bit < r._bit; }
#endif
		operator bool() const { return get(); }
		//explicit bitmap_cursor_t& operator=(bool value) { if(value)  set(); else clear(); return *this; }
		 bitmap_cursor_t& operator=(int value) { if(value)  set(); else clear(); return *this; }
		 bitmap_cursor_t& operator=(bool value) { if(value)  set(); else clear(); return *this; }
		inline bitmap_cursor_t& operator+=(int i) {
			 _bit += i;
			 return *this;
		}
		inline bitmap_cursor_t& operator-=(int i) {
			 _bit -= i;
			 return *this;
		}
		inline bitmap_cursor_t operator++() {
			++_bit;
			return bitmap_cursor_t(*this);
		}
		inline bitmap_cursor_t operator++(int) {
			bitmap_cursor_t tmp(*this);
			++_bit;
			return tmp;
		}
		inline bitmap_cursor_t operator--() {
			--_bit;
			return bitmap_cursor_t(*this);
		}
		inline bitmap_cursor_t operator--(int) {
			bitmap_cursor_t tmp(*this);
			--_bit;
			return tmp;
		}
	};


	template<uint32_t BITS>
	class bitmap_t {
	public:
		// bits per word
		static constexpr uint32_t WORD_MAX = std::numeric_limits<uint32_t>::max();
		static constexpr uint32_t WSIZE = sizeof(uint32_t);
		static constexpr uint32_t BPW = WSIZE * 8;
		inline static constexpr uint32_t _aligned(uint32_t size) {
			return (size / BPW) + ((size & (BPW - 1)) != 0);
		}
		static constexpr uint32_t BITCOUNT = BITS;
		static constexpr uint32_t WORDCOUNT = _aligned(BITCOUNT);
		// huh, I guess constexpr can go into attributes
		uint32_t _bitmap[WORDCOUNT] __attribute__((aligned(WSIZE)));
		bitmap_cursor_t operator[](uint32_t i) { return bitmap_cursor_t(_bitmap, i); }
		bitmap_cursor_t begin()  { return bitmap_cursor_t(_bitmap, 0); }
		bitmap_cursor_t end() { return bitmap_cursor_t(_bitmap, BITCOUNT); }

		void debug_print() {
			printk("WORDS: ");
			for(auto& a : _bitmap) {
				printk("%p ",a);
			}
			printk("\r\nBITS: ");
			for(size_t b=0;b < BITCOUNT;b++){
				if(test(b)) printk("1"); else printk("0");
			}
			printk("\r\n");
		}

		inline uint32_t first_clear_index() {
			for(size_t w =0; w < WORDCOUNT; w++){
				uint32_t word = _bitmap[w];
				if(word == 0) return (w * BPW);
				else if(word == std::numeric_limits<uint32_t>::max()) continue;
				else {
					uint32_t idx=0;
#if 0
				// to get this to work we have to set the bits in from lsb to msb
				// so ineffecentcies in bit setting or in clz meh
				idx =__builtin_clz(_bitmap[w]);
#else
				// we have a bit in here somewhere, we should lock it here but we check that latter
				while(1 & word) { ++idx; word >>=1; }
				return (w * BPW) + idx;
				}
#endif
			}
			return BITCOUNT;
		}
		// searches for the first clear, position
		// returns  BITCOUNT if none found, it does this atomicly
		bool first_clear(bitmap_cursor_t& c){
			uint32_t bit=first_clear_index();
			if(bit == BITCOUNT) return false;
			 c = bitmap_cursor_t(_bitmap, bit);
			 return true;
		}

		constexpr uint32_t size() const { return BITCOUNT; }
		bitmap_t() {} //
		void clear() { memset(_bitmap,0,sizeof(_bitmap)); }
		void clear(uint32_t bit) { BIT_CLEAR(_bitmap,bit);  }
		void set(uint32_t bit) { BIT_SET(_bitmap,bit);  }
		bool test_and_set(uint32_t bit) {
			bool old  =test(bit);
			set(bit);
			return old;
		}
		bool test(uint32_t bit) const { return BIT_GET(_bitmap,bit);  }
		bool first_clear_and_set(uint32_t& bit) {
			do {
				bit=first_clear_index();
				if(bit == BITCOUNT) return false;
				 if(test_and_set(bit)) continue;  // if its set search for another one
				 return true;
			} while(1);
			return false; // never get here
		}
		// finds the first clear and tries to set it
		bool first_clear_and_set(bitmap_cursor_t& c){
			uint32_t bit;
			if(!first_clear_and_set(bit)) return false;
			 c = bitmap_cursor_t(_bitmap, bit);
			 return true;
		}
		void set() { memset(_bitmap,0xFFFFFFFF,sizeof(_bitmap)); }
	};
	// an object of this class dosn't
	template<typename T, size_t _COUNT=64, size_t _TYPE_SIZE=sizeof(T)>
	class bitmap_alloc_t {
	public:

		using size_type = size_t;
		using difference_type = long;
		static constexpr size_type ELEMENT_COUNT = _COUNT;
		static constexpr size_type TYPE_SIZE = _TYPE_SIZE;
		// we want to make sure this type is allinged to the 32 bit mark
		static constexpr size_type ELEMENT_SIZE = (TYPE_SIZE + 3) & ~0x03;
		static constexpr size_type TOTAL_SIZE = ELEMENT_SIZE*ELEMENT_COUNT;
		using value_type = T;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;
#define VAR_DEBUG(NAME) printk(#NAME " = %d\r\n",NAME)
#define BMP_DEBUG(...) do {  printk(__VA_ARGS__); } while(0);
		void debug() {
			VAR_DEBUG(ELEMENT_COUNT);
			VAR_DEBUG(TYPE_SIZE);
			VAR_DEBUG(ELEMENT_SIZE);
			VAR_DEBUG(TOTAL_SIZE);
		}
		using type = T;
		using pointer_type = type*;
		using const_pointer_type = const pointer_type;
		using id_t = size_t;
		using bitmap_type = bitmap_t<_COUNT>;

		bitmap_alloc_t() {}
		void *alloc(std::size_t sz) {
			uint32_t i;
			if(sz > ELEMENT_SIZE){
				BMP_DEBUG("bitmap_alloc_t: %d > %d size wrong! \n", sz,ELEMENT_SIZE);
				while(1);
			}
			if(_bitmap.first_clear_and_set(i)){
				void* ptr = get_slot(i);
				printk("bitmap_alloc_t: %d allocated [%p]\n", i,ptr);
				return ptr;
			}else {
				BMP_DEBUG("bitmap_table: allocated failed, out of space\n");
				return nullptr;
			}
		}
		void free(void *elm)
		{
			uint32_t i = get_index(elm);
			assert(i < TOTAL_SIZE);
			printk("bitmap_alloc_t: %d dellocated [%p]\n", i,elm);
			_bitmap.clear(i);
		}
	private:
		bitmap_type _bitmap;
		uint8_t _data[TOTAL_SIZE];
		void* get_slot(uint32_t i) {
			assert(i < ELEMENT_COUNT);
			uint8_t* ptr = _data + (i * ELEMENT_SIZE);
			printk("alloc_id: %d allocated [%p]\n", i, ptr);
			return ptr;
		}
		uint32_t get_index(void* element) {
			uintptr_t ielm = reinterpret_cast<uintptr_t>(element);
			const uintptr_t ifirst = reinterpret_cast<uintptr_t>(&_data[0]);
			const uintptr_t ilast = reinterpret_cast<uintptr_t>(&_data[TOTAL_SIZE-ELEMENT_SIZE]);
			if(ielm >= ifirst && ielm <=ilast){
				return (ielm - ifirst) / sizeof(type);
			} else {
				printk("Not found but trying to release it anyway!\r\n");
			}
			return TOTAL_SIZE;
		}
	};

	// static alocation of types using a bitmap for
	// allocation tracking
	template<typename T, size_t _COUNT=64, size_t _TYPE_SIZE=sizeof(T)>
	class bitmap_table_t {
	public:
		using size_type = size_t;
		using difference_type = long;
		static constexpr size_type ELEMENT_COUNT = _COUNT;
		static constexpr size_type TYPE_SIZE = _TYPE_SIZE;
		// we want to make sure this type is allinged to the 32 bit mark
		static constexpr size_type ELEMENT_SIZE = (TYPE_SIZE / sizeof(uint32_t)) + ((TYPE_SIZE & (sizeof(uint32_t) - 1)) != 0);
		static constexpr size_type TOTAL_SIZE = ELEMENT_SIZE*ELEMENT_COUNT;

		using value_type = T;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;

		using type = T;
		using pointer_type = type*;
		using const_pointer_type = const pointer_type;
		using id_t = size_t;
		class iterator {
			bitmap_table_t& _table;
			T* _ptr;
			bitmap_cursor_t _current;
			inline void check_current()  {
				while(_current < _table._cursor_end && _current.get()) {
					_current++; _ptr++;
				}
			}
		public:
			iterator(bitmap_table_t& table, const bitmap_cursor_t& pos) :
				_table(table),
				_ptr(reinterpret_cast<T*>(table._data)),
				_current(pos) { check_current(); }
			inline bitmap_cursor_t operator++() {
				++_current++; ++_ptr;
				check_current();
				return bitmap_cursor_t(*this);
			}
			inline bitmap_cursor_t operator++(int) {
				iterator tmp(*this);
				++_current++; ++_ptr;
				check_current();
				return tmp;
			}
			operator T*() { return _ptr; }
			operator const T*() const { return _ptr; }
			bool operator==(const iterator& r) const { return _current == r._current; }
			bool operator!=(const iterator& r) const { return _current != r._current; }
		};

		template <class U>
		struct rebind { typedef bitmap_table_t<U,ELEMENT_COUNT,TYPE_SIZE> other; };
		// gcc thing here, otherwise have to call the deconstructor
		static constexpr bool is_trivialy_deconstructed = __has_trivial_destructor(T);

		constexpr size_t size() const { return ELEMENT_COUNT; }
		bool is_allocated(id_t i) { return (i < ELEMENT_COUNT) ? _bitmap[i] : false; }
		pointer_type at(id_t i) {
			if(i >= ELEMENT_COUNT) return nullptr;
			bitmap_cursor_t	c = _bitmap[i];
			return _bitmap[i] ? reinterpret_cast<T*>(_data + (i * ELEMENT_SIZE)) : nullptr;
		}
		const_pointer_type at(id_t i) const {
			if(i >= ELEMENT_COUNT) return nullptr;
			bitmap_cursor_t	c = _bitmap[i];
			return _bitmap[i] ? reinterpret_cast<T*>(_data + (i * ELEMENT_SIZE)) : nullptr;
		}
		pointer_type operator[](id_t i) { return at(i); }
		const_pointer_type operator[](id_t i) const { return at(i); }

		void *alloc_id(id_t i)
		{
			if(i < ELEMENT_COUNT) {
				bitmap_cursor_t	cursor = _bitmap[i];
				if(cursor.test_and_set()){
					uint8_t* ptr = _data + (i * ELEMENT_SIZE);
					printk("alloc_id: %d allocated [%p]\n", i, ptr);
					return ptr;
				}
				printk("alloc_id: %d already allocated! \n", i);
			}
			return nullptr;
		}
		void *alloc(std::size_t sz) {
			uint32_t i;
			if(sz > ELEMENT_SIZE){
				BMP_DEBUG("bitmap_alloc_t: %d > %d size wrong! \n", sz,ELEMENT_SIZE);
				while(1);
			}
			if(_bitmap.first_clear_and_set(i)){
				void* ptr = alloc_id(i);
				printk("bitmap_table: %d allocated [%p]\n", i,ptr);
				return ptr;
			}else {
				BMP_DEBUG("bitmap_table: allocated failed, out of space\n");
				return nullptr;
			}
		}
		// allocator interface
		template<typename F>
		pointer allocate(size_t size) {
		//	static_assert(F == T || F == void*, "Incorrect pointers");
			if(size<=ELEMENT_SIZE) {
				return reinterpret_cast<pointer>(alloc(size));
			} else {
				printk("watch this allocate!\r\n");
				size_t block_count = size / ELEMENT_SIZE;
				if(size % ELEMENT_SIZE) block_count++;
				uint32_t s = 0;
					uint32_t b =0;
					for(uint32_t c =0; c < ELEMENT_COUNT;c++,b++) {
						if(_bitmap[c].test_and_set()) {
							if(b == block_count) {
								uint8_t* ptr = _data + (s * ELEMENT_SIZE);
								printk("allocate(%d): %d blocks allocated [%p]\n", size, block_count, ptr);
								return reinterpret_cast<pointer>(ptr);
							}
						}
						else {
							// revert changes
							while(b) { _bitmap[s++].clear(); --b; }
						}
					}
					return nullptr;// could not find free  block
			}
		}
		void deallocate(pointer* ptr, size_t size) {
			if(size <=ELEMENT_SIZE) free(ptr);
			else {
				uint32_t s = getid(ptr);
				assert(s < ELEMENT_COUNT);
				size_t block_count = size / ELEMENT_SIZE;
				if(size % ELEMENT_SIZE) block_count++;
				while(block_count--) _bitmap[s++].clear();
				printk("deallocate(%p): %d blocks  [%p]\n", ptr, block_count);
			}
		}
		inline size_type max_size() const {  return TOTAL_SIZE; }
		inline pointer address(reference r) { return &r; }
		inline const_pointer address(const_reference r) { return &r; }
		inline void construct(pointer p, const T& t) { new(p) T(t); }
		inline void destroy(pointer p) { p->~T(); }
		// end allocator interface
		// iliterator interface
		iterator begin() { return iterator(this, _cursor_begin);  }
		iterator end() { return iterator(this, _cursor_end); }
		// end ilt interface
		void free(void *elm)
		{
			free_id(getid(elm));
		}
		void free_id(id_t id)
		{
			if(id < ELEMENT_COUNT){
				bitmap_cursor_t	c = _bitmap[id];
				if(!c.test_and_set())
					printk("free_id: trying to free something already freed!\n");
				c.clear();
			}
		}
		template<typename...Args>
		T *construct(Args...args){
			void* ptr = alloc(sizeof(T));
			if(ptr) {
				T* obj = new(ptr) T(std::forward<Args>(args)...);
				return obj;
			}
			return nullptr;
		}
		// we recreate the object at id
		template<typename...Args>
		T *construct_id(id_t id, Args...args){
			void* ptr = alloc_id(id);
			if(ptr == nullptr) return nullptr;
			T* obj = new(ptr) T(std::forward<Args>(args)...);
			return obj;
		}
		void destruct(T *elm)
		{
			bitmap_cursor_t c;
			if(getid(elm,c)){
				if(!is_trivialy_deconstructed) ~(*elm);
				c.clear();
			} else {
				printk("bitmap_table: trying to free something not on map!\n");
				assert(0);
			}
		}
		uint32_t getid(void* element) {
			uintptr_t ielm = reinterpret_cast<uintptr_t>(element);
			static const uintptr_t ifirst = reinterpret_cast<uintptr_t>(&_data[0]);
			static const uintptr_t ilast = reinterpret_cast<uintptr_t>(&_data[TOTAL_SIZE-ELEMENT_SIZE]);
			return (ielm >= ifirst && ielm <=ilast) ? (ielm - ifirst) / ELEMENT_SIZE : ELEMENT_COUNT;
		}
		bool getid(void *element,bitmap_cursor_t& c)
		{
			uint32_t i = getid(element);
			if(i < ELEMENT_COUNT){
				c = _bitmap[i];
				return true;
			} else return false;
		}
		bitmap_table_t() : _cursor_begin(bitmap_cursor_t(_bitmap[0])),  _cursor_end(bitmap_cursor_t(_bitmap[ELEMENT_COUNT])) {}
	protected:

		bitmap_t<ELEMENT_COUNT> _bitmap;
		uint8_t _data[TOTAL_SIZE];
		const bitmap_cursor_t _cursor_begin;
		const bitmap_cursor_t _cursor_end;
	};

	// this class needs to be defined as static, mostly

	template<typename T, size_t _COUNT, size_t _TYPE_SIZE>
	constexpr bool operator== (const bitmap_table_t<T,_COUNT,_TYPE_SIZE>&, const bitmap_table_t<T,_COUNT,_TYPE_SIZE>&) noexcept {
	  return true;
	}
	template<typename T, size_t _COUNT, size_t _TYPE_SIZE>
	constexpr bool operator!= (const bitmap_table_t<T,_COUNT,_TYPE_SIZE>&, const bitmap_table_t<T,_COUNT,_TYPE_SIZE>&) noexcept {
	  return false;
	}
	template<typename T, size_t _COUNT, size_t _TYPE_SIZE>
	using RebindAlloc = typename std::allocator_traits<bitmap_table_t<T,_COUNT,_TYPE_SIZE>>::template bitmap_table_t<T>;


	// simple refrence counter
		template<typename T, size_t _COUNT=64>
	   class shared_alloc_t {
	   public:
			using size_type = size_t;
			using difference_type = long;
			static constexpr size_type ELEMENT_COUNT = _COUNT;
			static constexpr size_type TYPE_SIZE = sizeof(T);
			using value_type = T;
			using pointer = value_type*;
			using const_pointer = const value_type*;
			using reference = value_type&;
			using const_reference = const value_type&;

			using type = T;
			using pointer_type = type*;
			using const_pointer_type = const pointer_type;
			using id_t = size_t;

	   private:
			struct gc_type {
				__attribute__ ((aligned(4))) uint8_t data[TYPE_SIZE];
				uint32_t ref;
			} __attribute__ ((aligned(4)));

		public:
			static constexpr size_type ELEMENT_SIZE = sizeof(gc_type);
			// we want to make sure this type is allinged to the 32 bit mark
			static constexpr size_type TOTAL_SIZE = ELEMENT_SIZE*ELEMENT_COUNT;

			void *alloc(std::size_t sz) {
				if(sz > ELEMENT_SIZE){
					BMP_DEBUG("bitmap_alloc_t: %d > %d size wrong! \n", sz,ELEMENT_SIZE);
					while(1);
				}
				while(true) {
					auto last_free = _last_free;
					auto it=last_free;
					do {
						gc_type* gc = &(*it);
						if(++gc->ref == 1){
							++last_free;// this is just a hint so its ok to inc
							return gc;
						}
						--gc->ref;
						if(++it == _objs.end()) it = _objs.begin();
					} while(it!=last_free);
					BMP_DEBUG("bitmap_table: allocated failed, out of space\n");
					ARM::wfi(); // wait for an interrupt
				}
			}

			void aquire(id_t id){
				assert(id < ELEMENT_COUNT);
				if(++_objs[id].ref == 1) {
					printk("tryiing to refrence a deleted object %d!\r\n",id);
				}
			}
			void aquire(void *elm){
				aquire(getid(elm));
			}
			void free(void *elm){
				release(getid(elm));
			}
			void release(id_t id){
				if(id < ELEMENT_COUNT){
					if(_objs[id].ref >=  0) --_objs[id].ref;
					if(_objs[id].ref == 0){
						printk("freed %d!\n, id");
						_last_free = _objs.begin() + id;
					}
				}
			}
			uint32_t refs(id_t id) const { assert(id < ELEMENT_COUNT); return _objs[id].ref; }
			uint32_t refs(void *elm) const { return refs(getid(elm)); }

			inline constexpr uint32_t getid(void* e) const {
				return (ielm(e) >= ifirst() && ielm(e) <=ilast()) ?  ((ielm(e) - ifirst()) / ELEMENT_SIZE) : ELEMENT_COUNT;
			}
		protected:
			using array_t = typename std::array<gc_type,ELEMENT_COUNT>;
			array_t _objs;
			typename array_t::iterator _last_free; // saves some time
			inline constexpr uintptr_t ifirst() const { return reinterpret_cast<uintptr_t>(&_objs[0]); }
			inline constexpr uintptr_t ilast() const { return reinterpret_cast<uintptr_t>(&_objs[TOTAL_SIZE-ELEMENT_SIZE]); }
			inline constexpr uintptr_t ielm(void* element) const { return reinterpret_cast<uintptr_t>(element); }


		};

}; /* namespace xv6 */

#endif /* XV6CPP_BITMAP_H_ */
