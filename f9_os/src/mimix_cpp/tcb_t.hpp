/*
 * tcb_t.hpp
 *
 *  Created on: May 28, 2017
 *      Author: warlo
 */
#if 0
#ifndef MIMIX_CPP_TCB_T_HPP_
#define MIMIX_CPP_TCB_T_HPP_

namespace mimx {

static inline void request_schedule(){ SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; }
class tcb_t {
	enum stack_type_t {
		ST_STATIC =0,		// tcb_t is allocated outside of the stack
		ST_MALLOC,			// tcb_t was made with create
		ST_STACK			// tcb_t struc is below the stack that we don't control
	};
public:
	friend void __svc_handler(void);
	static constexpr tcb_t * MAGIC_MARK = reinterpret_cast<tcb_t*>(0x97654321);
	tailq::entry<tcb_t> t_link;		// link in the running queue
	using queue_type = tailq::head<tcb_t, &tcb_t::t_link>;
	queue_type* t_queue; // we know what queue we are in
	void remove_from_queue() {
		if(t_queue != nullptr) {
			t_queue->remove(this);
			t_queue = nullptr;
		}
	}
	void add_to_queue(queue_type& q) {
		if(t_queue != nullptr) {
			if(t_queue == &q) return; // its already in there
			t_queue->remove(this); // remove from exisiting queue
		}
		q.push(this);
		t_queue = &q;
	}

	list::entry<tcb_t> t_hash;		// link in the look up que

	bool isrunnable() const { return state == T_RUNNABLE; }

	thread_state_t state=thread_state_t::T_INACTIVE;

	pid_t t_globalid=0;
	uintptr_t stack_base;
	size_t stack_size;
	f9_context_t ctx;
	tcb_t *t_sibling=nullptr;
	tcb_t *t_parent=nullptr;
	tcb_t *t_child=nullptr;
	void* t_chan = nullptr;
	//as_t *as;
//	struct utcb *utcb;

	pid_t ipc_from=0;



	uint32_t timeout_event =0;
	// this copys the context to here
	// hopeful the parrents are previously setup
	void fork_copy(const tcb_t& copy) {
		// we just copy the context and stack to here
		assert(stack_size >= copy.stack_size);
		uint32_t offset = stack_size - copy.stack_size;
		uint8_t* stack = reinterpret_cast<uint8_t*>(ctx.sp)+offset;
		const uint8_t* copy_stack = reinterpret_cast<uint8_t*>(ctx.sp);
		std::copy(copy_stack,copy_stack+copy.stack_size,stack);
		ctx = copy.ctx; // copy the context
		ctx.sp = reinterpret_cast<uint32_t>(stack); // setup the new return
	}
	struct hasher {
		list::int_hasher<pid_t> int_hasher;
		constexpr size_t operator()(const pid_t t) const {
			return int_hasher(t);
		}
		constexpr size_t operator()(const tcb_t& t) const {
			return int_hasher(t.t_globalid);
		}
	};
	struct equaler {
		constexpr size_t operator()(const tcb_t& a,const tcb_t& b) const {
			return a.t_globalid == a.t_globalid;
		}
		constexpr size_t operator()(const tcb_t& t, pid_t pid) const {
			return t.t_globalid == pid;
		}
	};
	using hash_type = list::hash<tcb_t,&tcb_t::t_hash, 16, hasher,equaler>;



	friend queue_type;
	friend class kerenel;

	tcb_t *thread_select(){
		tcb_t *thr = t_child;
		if (thr == nullptr) return nullptr;

		while (1) {
			if (thr->isrunnable())
				return thr;

			if (thr->t_child != nullptr) {
				thr = thr->t_child;
				continue;
			}

			if (thr->t_sibling != nullptr) {
				thr = thr->t_sibling;
				continue;
			}

			do {
				if (thr->t_parent == this)
					return nullptr;
				thr = thr->t_parent;
			} while (thr->t_sibling == nullptr);

			thr = thr->t_sibling;
		}
	}
	template<typename PC, typename AT>
	void init(PC pc, AT stack, size_t stack_size_) {
		uint32_t base = ptr_to_int(stack);
		stack_base= base;
		stack_size= stack_size_;
		ctx.init(base+stack_size_, pc);
		t_link.tqe_next = MAGIC_MARK;
	}
	template<typename PC, typename AT>
	tcb_t(PC pc, AT stack, size_t stack_size) :
		t_queue(nullptr),
		stack_base(ptr_to_int(stack)),
		stack_size(stack_size),
		ctx(ptr_to_int(stack)+stack_size, pc){
		t_link.tqe_next = MAGIC_MARK;
	}
	template<typename PC, typename AT, typename ARGT>
	tcb_t(PC pc, AT stack, size_t stack_size, ARGT arg) :
		t_queue(nullptr),
		stack_base(ptr_to_int(stack)),
		stack_size(stack_size),
		ctx(ptr_to_int(stack)+stack_size, pc,arg){
		t_link.tqe_next = MAGIC_MARK;
	}
	template<typename PC, typename AT>
	static tcb_t* create(PC pc, AT stack, size_t stack_size) {
		uint32_t base = ptr_to_int(stack);
		uint32_t after_tcb = base + stack_size - sizeof(tcb_t);
		tcb_t* ptr = new(reinterpret_cast <uint8_t*>(after_tcb)) tcb_t();
		assert(ptr);
		ptr->stack_base = base;
		ptr->stack_base = stack_size - sizeof(tcb_t);
		ptr->ctx.init(pc,after_tcb-sizeof(uint32_t));
		return ptr;
	}
	template<typename PC, typename AT, typename ARGT>
	static tcb_t* create(PC pc, AT stack, size_t stack_size,ARGT arg) {
		tcb_t* ptr =_create(pc,stack,stack_size);
		ptr->ctx[REG::R0] = ptr_to_int(arg);
		return ptr;
	}
protected:
	tcb_t() : t_queue(nullptr),stack_base(0),stack_size(0) {}
} __attribute__((aligned(8)));



}



#endif /* MIMIX_CPP_TCB_T_HPP_ */
#endif
