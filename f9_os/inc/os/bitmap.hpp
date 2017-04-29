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
#include <functional>
#include <cassert>
#include <memory>
#include <os\printk.h>
#include <sys\queue.h>
#include <os\atomic.h>


// define below to make this nifty
#define CONFIG_BITMAP_BITBAND
// bit map functions for the cortex m
namespace bitops {
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


		// remember this can only go into sram somewhere
	class bitmap_cursor_t {
#ifdef CONFIG_BITMAP_BITBAND
		static constexpr uint32_t BITBAND_ADDR_SHIFT = 5;
		static constexpr uint32_t BIT_SHIFT = 2;
		/* Generate address in bit-band region */
		inline static constexpr uint32_t* ADDR_BITBAND(void*addr) {
			return reinterpret_cast<uint32_t*>(
					0x22000000 + (((reinterpret_cast<uintptr_t>(addr) & 0xFFFFF) << BITBAND_ADDR_SHIFT))
			);
		}
		/*
		 * soo a quick description how this works, we cannot save a uint32_t pointer because we would skip
		 * 4 bits when we tried something like _bit++.  this all does a bit of fuckery as, from
		 * the machine level, evey time we save a word to this addres, we are setting a bit at another one
		 * but for us, we have to find the address of the bit with a bunch of math and because of c++
		 * using reinterpret_cast
		 */
		uint32_t* _bit; // better if this was a uint32_t?
	public:
		inline bitmap_cursor_t() : _bit(nullptr) {}
		inline bool get() const { return *_bit != 0; }
		inline void set() { *_bit =  1; }
		inline void clear() { *_bit = 0; }
		inline bool test_and_set() { return ARM::atomic_test_and_set(_bit); }
		inline bitmap_cursor_t(uint32_t* bitmap, uint32_t bit) : _bit(((ADDR_BITBAND(bitmap) + bit))){}
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
		static constexpr size_t BITMAP_ALIGN = 32;
		constexpr inline uint32_t BITOFF() const { return (_bit % BITMAP_ALIGN); }		/* bit offset inside 32-bit word */
		constexpr inline uint32_t BITMASK() const { return (1 << BITOFF(_bit)); }	 /* Mask used for bit number N */
		constexpr inline uint32_t BITINDEX() const { return (_bit / BITMAP_ALIGN); }	/* Bit index in bitmap array */
		inline uint32_t& BITWORD()  { return _bitmap[BITINDEX()]; }
		inline const uint32_t& BITWORD()  const { return _bitmap[BITINDEX()]; }
		inline void _next() { _bit++; }
		inline void _prev() { _bit--; }
		uint32_t* _bitmap;
		uint32_t _bit;
	public:
		inline bool get() const { return ((BITWORD() >> BITOFF()) & 0x1) != 0; }
		inline void set() {  BITWORD() |= BITMASK(); }
		inline void clear() { BITWORD() &= ~BITMASK(); }
		inline bool test() const  { return _bit != 0; }
		inline bool test_and_set() { return  test_and_set_bit(&BITWORD(), BITMASK()); }
		inline uint32_t id() const { _bit; }
		inline bitmap_cursor_t(uint32_t* bitmap, uint32_t bit) : _bitmap(bitmap),_bit(bit) {}
		inline bool operator==(const bitmap_cursor_t& r) const { return _bitmap == r._bitmap && _bit == r._bit; }
		inline bool operator!=(const bitmap_cursor_t& r) const { return !(*this == r); }
		inline bool operator>(const bitmap_cursor_t& r) const { return _bitmap > r._bitmap && _bit > r._bit; }
		inline bool operator<(const bitmap_cursor_t& r) const { return _bitmap > r._bitmap && _bit < r._bit; }
		inline bool operator>=(const bitmap_cursor_t& r) const { return _bitmap <= r._bitmap && _bit >= r._bit; }
		inline bool operator<=(const bitmap_cursor_t& r) const { return _bitmap <= r._bitmap && _bit <= r._bit; }
#endif
		operator bool() const { return get(); }
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

		// searches for the first clear, position
		// returns  BITCOUNT if none found, it does this atomicly
		uint32_t first_clear(bitmap_cursor_t& c){
			uint32_t bit=0;
			for(;bit < BITCOUNT && _bitmap[bit/BPW] == WORD_MAX;bit+=BPW);
			 c = bitmap_cursor_t(_bitmap, bit);
			while(bit < BITCOUNT){
				if(c == false) break;
				++bit;++c;
			}
			return bit;
		}
		// returns  BITCOUNT if none found, it does this atomicly, only one pass though
		uint32_t first_clear_atomic(bitmap_cursor_t& c){
			uint32_t bit;
			c = begin();
			for(bit=0;bit < BITCOUNT ; bit++,c++){
				if(!c.test_and_set()) return bit;
				c.clear();
			}
			return bit;
		}
		constexpr uint32_t size() const { return BITCOUNT; }
		bitmap_t() {} //
		void clear() { memset(_bitmap,0,sizeof(_bitmap)); }
	};
	// an object of this class dosn't
	class gc_object {

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

		void *alloc() {
			bitmap_cursor_t c;
			id_t i;
			if((i=_bitmap.first_clear_atomic(c)) < ELEMENT_COUNT)
				return alloc_id(i);
			printk("bitmap_table: %d allocated failed\n", i);
			return nullptr;
		}
		// allocator interface
		pointer allocate(size_t size) {
			if(size<=ELEMENT_SIZE) {
				return alloc();
			} else {
				size_t block_count = size / ELEMENT_SIZE;
				if(size % ELEMENT_SIZE) block_count++;
				uint32_t s = 0;
					uint32_t c =0;
					uint32_t b =0;
					for(uint32_t c =0; c < ELEMENT_COUNT;c++,b++) {
						if(_bitmap[c].test_and_set()) {
							if(b == block_count) {
								uint8_t* ptr = _data + (s * ELEMENT_SIZE);
								printk("allocate(%d): %d blocks allocated [%p]\n", size, block_count, ptr);
								return ptr;
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
			void* ptr = alloc();
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
			if(ielm >= ifirst && ielm <=ilast){
				return (ielm - ifirst) / sizeof(type);
			}
			return ELEMENT_COUNT;
		}
		bool getid(void *element,bitmap_cursor_t& c)
		{
			uint32_t i = getid(element);
			if(i < ELEMENT_COUNT){
				c = _bitmap[i];
				return true;
			} else return false;
		}
		bitmap_table_t() : _cursor_begin(bitmap_cursor_t(&_bitmap,0)),  _cursor_end(bitmap_cursor_t(&_bitmap,ELEMENT_COUNT)) {}
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
} /* namespace xv6 */

#endif /* XV6CPP_BITMAP_H_ */