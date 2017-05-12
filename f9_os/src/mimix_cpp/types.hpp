#ifndef TYPE_H
#define TYPE_H

#include <cstdint>
#include <sys\types.h>
#include <climits>

#include "config.hpp"


#include <sys\time.h>

typedef struct ::timeval timeval_t;

static  inline timeval_t operator+(const timeval_t& l, const timeval_t& r){
	timeval_t tv = { l.tv_sec + r.tv_sec, l.tv_usec + r.tv_usec };
	if (tv.tv_usec >= 1000000) { tv.tv_sec++; tv.tv_usec -= 1000000; }
	return tv;
}
static inline timeval_t operator-(const timeval_t& l, const timeval_t& r){
	timeval_t tv = { l.tv_sec - r.tv_sec, l.tv_usec - r.tv_usec };
	if (tv.tv_usec < 0) { tv.tv_sec--; tv.tv_usec += 1000000; }
	return tv;
}
static inline timeval_t& operator+=(timeval_t& l, const timeval_t& r){
	l.tv_sec += r.tv_sec;
	l.tv_usec += r.tv_usec;
	if (l.tv_usec >= 1000000) { l.tv_sec++; l.tv_usec -= 1000000; }
	return l;
}
static inline timeval_t& operator-=(timeval_t& l, const timeval_t& r){
	l.tv_sec -= r.tv_sec;
	l.tv_usec -= r.tv_usec;
	if (l.tv_usec < 0) { l.tv_sec--; l.tv_usec += 1000000; }
	return l;
}
static constexpr inline bool operator<(const timeval_t& l, const timeval_t& r){
	return l.tv_sec == r.tv_sec ? l.tv_usec < r.tv_usec : l.tv_sec < r.tv_sec;
}
static constexpr inline bool operator>(const timeval_t& l, const timeval_t& r){
	return l.tv_sec == r.tv_sec ? l.tv_usec > r.tv_usec : l.tv_sec > r.tv_sec;
}
static constexpr inline bool operator==(const timeval_t& l, const timeval_t& r){
	return l.tv_usec == r.tv_usec && l.tv_sec == r.tv_sec;
}
static constexpr inline bool operator!=(const timeval_t& l, const timeval_t& r){
	return l.tv_usec != r.tv_usec || l.tv_sec != r.tv_sec;
}
static constexpr inline bool operator>=(const timeval_t& l, const timeval_t& r){
	return l == r || l > r;
}
static constexpr inline bool operator<=(const timeval_t& l, const timeval_t& r){
	return l == r || l < r;
}
extern "C" 	void panic(const char*,...);
extern "C" 	void printk(const char*,...);

namespace mimx {
	typedef uint32_t bitchunk_t; /* collection of bits in a bitmap */
	typedef uint32_t reg_t;
	typedef int irq_id_t;
	/* Constants and macros for bit map manipulation. */
	constexpr static size_t BITCHUNK_BITS  =  (sizeof(bitchunk_t) * 8);
	constexpr static inline size_t BITMAP_CHUNKS(size_t nr_bits) { return ((nr_bits+BITCHUNK_BITS-1)/BITCHUNK_BITS); }

	constexpr static inline bitchunk_t& MAP_CHUNK(bitchunk_t *map, size_t bit) { return map[bit/BITCHUNK_BITS]; }
	constexpr static inline size_t CHUNK_OFFSET(size_t bit) { return bit % BITCHUNK_BITS; }

	constexpr static inline bool GET_BIT(bitchunk_t * map, size_t bit) { return (MAP_CHUNK(map,bit) & (1 << CHUNK_OFFSET(bit))) != 0;  }
	static inline void SET_BIT(bitchunk_t * map, size_t bit) { MAP_CHUNK(map,bit) |= (1 << CHUNK_OFFSET(bit)); }
	static inline void UNSET_BIT(bitchunk_t * map, size_t bit) { MAP_CHUNK(map,bit) &= ~(1 << CHUNK_OFFSET(bit)); }


	constexpr static size_t  NR_SYS_CHUNKS	= BITMAP_CHUNKS(mimx::NR_SYS_PROCS);
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


	/* Process table and system property related types. */
	typedef int proc_nr_t;			/* process table entry number */
	typedef short sys_id_t;			/* system process index */
	typedef struct {			/* bitmap for system indexes */
	  bitchunk_t chunk[BITMAP_CHUNKS(NR_SYS_PROCS)];
	  inline bool get_bit(size_t bit) { return GET_BIT(chunk,bit); }
	  inline void set_bit(size_t bit) { SET_BIT(chunk,bit); }
	  inline void  unset_bit(size_t bit) { UNSET_BIT(chunk,bit); }
	} sys_map_t;

	struct boot_image {
	  proc_nr_t proc_nr;			/* process number to use */
	 // task_t *initial_pc;			/* start function for tasks */
	  int flags;				/* process flags */
	  unsigned char quantum;		/* quantum (tick count) */
	  int priority;				/* scheduling priority */
	  int stksize;				/* stack size for tasks */
	  short trap_mask;			/* allowed system call traps */
	  bitchunk_t ipc_to;			/* send mask protection */
	  long call_mask;			/* system call protection */
	  char proc_name[P_NAME_LEN];		/* name in process table */
	};

	struct memory {
	  size_t base;			/* start address of chunk */
	  size_t size;			/* size of memory chunk */
	};


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

	__attribute__( ( always_inline ) ) static inline void save_flags(uint32_t& flags)
	{
		  __asm volatile ("MRS %0, primask" : "=r" (flags) );
	}
	__attribute__( ( always_inline ) ) static inline void restore_flags(uint32_t flags)
	{
		  __asm volatile ("msr primask, %0" : : "r"(flags) );
	}
	class irq_simple_lock {
		uint32_t _status;
	public:
		irq_simple_lock() { save_flags(_status); cli(); }
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
