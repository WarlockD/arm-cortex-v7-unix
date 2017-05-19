#ifndef TYPE_H
#define TYPE_H

#include <cstdint>
#include <sys\types.h>
#include <climits>

#include "config.hpp"

#include <os\bitmap.hpp>
#include <sys\time.h>

extern "C" 	void panic(const char*,...);
extern "C" 	void printk(const char*,...);

namespace mimx {

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
