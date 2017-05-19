#ifndef F9_THREAD_HPP_
#define F9_THREAD_HPP_

#include "types.hpp"

namespace f9 {
/**
 * @file thread.h
 * @brief Thread dispatcher definitions
 *
 * Thread ID type is declared in @file types.h and called l4_thread_t
 *
 * For Global Thread ID only high 18 bits are used and lower are reserved,
 * so we call higher meaningful value TID and use GLOBALID_TO_TID and
 * TID_TO_GLOBALID macroses for convertion.
 *
 * Constants:
 *   - L4_NILTHREAD  - nilthread
 *   - L4_ANYLOCALTHREAD - anylocalthread
 *   - L4_ANYTHREAD  - anythread
 */

static constexpr thread_id_t NILTHREAD		= 0;
static constexpr thread_id_t ANYLOCALTHREAD	= 0xFFFFFFC0;
static constexpr thread_id_t ANYTHREAD		= 0xFFFFFFFF;



 enum class THREAD : thread_id_t{
	IDLE,
	KERNEL,
	ROOT,
	INTERRUPT,
	IRQ_REQUEST,
	LOG,
	SYS	= 16,				/* Systembase */
	USER	= CONFIG_INTR_THREAD_MAX	/* Userbase */
} ;
 inline static constexpr thread_id_t GLOBALID_TO_TID(thread_id_t id)	{ return (id >> 14); }
 inline static constexpr thread_id_t TID_TO_GLOBALID(thread_id_t id)	{ return(id << 14); }
 inline static constexpr thread_id_t TID_TO_GLOBALID(THREAD id)	{ return(static_cast<thread_id_t>(id) << 14); }

enum class TSTATE {
	INACTIVE,
	RUNNABLE,
	SVC_BLOCKED,
	RECV_BLOCKED,
	SEND_BLOCKED
} ;
enum register_stack_t {
	/* Saved by hardware */
	REG_R0,
	REG_R1,
	REG_R2,
	REG_R3,
	REG_R12,
	REG_LR,
	REG_PC,
	REG_xPSR,
	REG_END
};
struct context_t {
	uint32_t sp;
	uint32_t ret;
	uint32_t ctl;
	uint32_t regs[8];
#ifdef CONFIG_FPU
	uint64_t fp_regs[8];
	uint32_t fp_flag;
#endif
	uint32_t get_reg(register_stack_t reg) const {
		const uint32_t* stack = reinterpret_cast<const uint32_t*>(sp);
		return stack[reg];
	}
#if 0
	// helper to convert a pointer to a uintptr_t
	template<typename T>
	constexpr static inline typename std::enable_if<std::is_arithmetic<T>::value, uintptr_t>::type
									  ptr_to_int(T v) { return static_cast<uintptr_t>(v); }
	template<typename T>
	constexpr static inline uintptr_t ptr_to_int(T* v) { return reinterpret_cast<uintptr_t>(v); }
	constexpr static inline uintptr_t ptr_to_int(nullptr_t) { return 0; } // match null pointers
#endif
	template<typename T>
	T cast_reg(register_stack_t index) const  {
		return int_to_ptr<T>((uintptr_t)get_reg(index));
	}
	void set_reg(register_stack_t reg, uintptr_t value)  {
		uint32_t* stack = reinterpret_cast<uint32_t*>(sp);
		stack[reg] = value;
	}
	template<typename T>
	void set_reg(register_stack_t reg, T value) const { return set_ret(reg,ptr_to_int(value)); }

	template<typename T>
	auto cast_arg(size_t index) const -> decltype(static_cast<T>(uint32_t{})) {
		return static_cast<T>(get_arg(index));
	}
	template<typename T>
	auto cast_arg(size_t index) const -> decltype(reinterpret_cast<T>(uint32_t{})) {
		return reinterpret_cast<T>(get_arg(index));
	}
	uint32_t get_arg(size_t index)  const {
		const uint32_t* stack = reinterpret_cast<const uint32_t*>(sp);
		if(index < 5) return stack[index];
		else
			// other args are pushed on the stack
			// check ret to see if we have to skip lazy floating
			return stack[REG_END + index-5];
	}
	void set_return(uintptr_t value) {
		uint32_t* stack = reinterpret_cast<uint32_t*>(sp);
		stack[REG_R0] = value;
	}
	template<typename T>
	void set_return(T value) const { return set_return(ptr_to_int(value)); }

	uint16_t inst(size_t index) const {
		return cast_reg<uint16_t*>(REG_PC)[index];
	}
	//static irq_save_NAKED

} ;
struct utcb_t {
/* +0w */
	thread_id_t t_globalid;
	uint32_t	processor_no;
	uint32_t 	user_defined_handle;	/* NOT used by kernel */
	thread_id_t	t_pager;
/* +4w */
	uint32_t	exception_handler;
	uint32_t	flags;		/* COP/PREEMPT flags (not used) */
	uint32_t	xfer_timeouts;
	uint32_t	error_code;
/* +8w */
	thread_id_t	intended_receiver;
	thread_id_t	sender;
	uint32_t	thread_word_1;
	uint32_t	thread_word_2;
/* +12w */
	uint32_t	mr[8];		/* MRs 8-15 (0-8 are laying in
					   r4..r11 [thread's context]) */
/* +20w */
	uint32_t	br[8];
/* +28w */
	uint32_t	reserved[4];
/* +32w */
} ;
constexpr static size_t UTCB_SIZE		=128;
//__attribute((aligned(128)));
//static_assert(sizeof(utcb_t) != 128, "utcb needs to be equal to 128?");
//#define


static constexpr size_t RESERVED_STACK =(8 * sizeof(uint32_t));

/**
 * Thread control block
 *
 * TCB is a tree of threads, linked by t_sibling (siblings) and t_parent/t_child
 * Contains pointers to thread's UTCB (User TCB) and address space
 */
struct as_t;
struct tcb_t {
	thread_id_t t_globalid;
	thread_id_t t_localid;

	TSTATE state;

	memptr_t stack_base;
	size_t stack_size;

	context_t ctx;
#ifdef CONFIG_ENABLE_FPAGE
	std::shared_ptr<as_t> as;
#endif
	utcb_t *utcb;

	thread_id_t ipc_from;

	tcb_t *t_sibling;
	tcb_t *t_parent;
	tcb_t *t_child;
	inline uint32_t ipc_read(int i){
		return (i >= 8) ?  utcb->mr[i - 8] : ctx.regs[i];
	}
	void ipc_write(int i, uint32_t data){
		if (i >= 8)
			utcb->mr[i - 8] = data;
		else
			ctx.regs[i] = data;
	}
	uint32_t timeout_event;
	bool ispriviliged() const { return GLOBALID_TO_TID(t_globalid) == static_cast<uint32_t>(THREAD::ROOT); }
	bool isrunnable() const { return state == TSTATE::RUNNABLE; }



	template<typename SPT, typename PCT, typename REGST>
	inline void init_ctx(SPT sp, PCT pc, REGST regs) { _init_ctx(ptr_to_int(sp), ptr_to_int(pc), ptr_to_int(regs)); }
	template<typename SPT>
	inline void init_kernel_ctx(SPT sp) { _init_kernel_ctx(ptr_to_int(sp)); }
	void switch_to();
	void free_space();
	void space(thread_id_t spaceid, utcb_t *utcb);
	static void * operator new (size_t size);
	static	void operator delete (void * mem);

	~tcb_t();
	static tcb_t* create(thread_id_t globalid, utcb_t *utcb);
	static volatile tcb_t* current();
	static void create_root_thread(void);
	static void create_kernel_thread(void);
	static void create_idle_thread(void);
	static void switch_to_kernel(void); // startup?
	static void startup();
private:
	void _init_ctx(uintptr_t sp, uintptr_t pc, uintptr_t regs);
	void _init_kernel_ctx(uintptr_t sp);
	tcb_t(thread_id_t globalid, utcb_t *utcb);
	friend void switch_to_kernel();
};

tcb_t *thread_by_globalid(thread_id_t globalid);
inline static tcb_t* THREAD_BY_TID(thread_id_t id){ return thread_by_globalid(TID_TO_GLOBALID(id)); }



/**
	 * Userspace errors
	 */
	enum class UE{
		NO_PRIVILIGE 	= 1,
		OUT_OF_MEM		= 8,

		/**
		 * ThreadControl errors
		 * see L4 X.2 R7 Reference, Page 24-25
		 *
		 * Invalid UTCB is not used because we have no Utcb Area
		 */
		TC_NOT_AVAILABLE	= 2,
		TC_INVAL_SPACE	= 3,
		TC_INVAL_SCHED	= 4,
		TC_INVAL_UTCB	= 6,		/* Not used */

		/**
		 * Ipc errors
		 * see L4 X.2 R7 Reference, Page 67-68
		 *
		 * NOTE: Message overflow may also occur if MR's are not enough
		 */

		IPC_PHASE_SEND	= 0,
		IPC_PHASE_RECV	= 1,

		IPC_TIMEOUT		= 1 << 1,
		IPC_NOT_EXIST	= 2 << 1,
		IPC_CANCELED		= 3 << 1,

		IPC_MSG_OVERFLOW	= 4 << 1,
		IPC_XFER_TIMEOUT	= 5 << 1,
		IPC_XFER_TIMEOUT2	= 6 << 1,
		IPC_ABORTED		= 7 << 1
	};

	void set_caller_error(UE error);
	void set_user_error(tcb_t *thread, UE error);
	void panic_impl(char *panicfmt, ...);
	void assert_impl(int cond, const char *condstr, const char *funcname);

	// this was in sys_thread
	void create_root_thread(void);
	void create_kernel_thread(void);
	void create_idle_thread(void);

	void switch_to_kernel(void);
	void set_kernel_state(TSTATE state);

	// interrupt system
	/* interrupt ipc message */
	enum class IRQ_IPC{
	    IRQN = 0,
	    IPC_TID = 1,
	    IPC_HANDLER = 2,
	    IPC_ACTION = 3,
	    IPC_PRIORITY = 4
	};

	#define IRQ_IPC_MSG_NUM	IRQ_IPC_PRIORITY

	/* irq actions */
	enum USER_IRQ {
	    ENABLE = 0,
	    IRQ_DISABLE = 1,
	    IRQ_FREE = 2
	};

	#define USER_INTERRUPT_LABEL	0x928

	void user_interrupt_config(tcb_t *from);
	void user_interrupt_handler_update(tcb_t *thr);

	/* Platform depended */
	void user_irq_enable(irqn irq);
	void user_irq_disable(irqn irq);
	void user_irq_set_pending(irqn irq);
	void user_irq_clear_pending(irqn irq);
	void user_log(tcb_t *from);

};

#define DECLARE_THREAD(name, sub) \
	void name(void) __attribute__ ((naked));	\
	void __USER_TEXT name(void)			\
	{						\
		register void *kip_ptr asm ("r0");	\
		register void *utcb_ptr asm ("r1");	\
		sub(kip_ptr, utcb_ptr);			\
		while (1);				\
	}
template<>
struct enable_bitmask_operators<f9::UE>{
	static const bool enable=true;
};

#endif
