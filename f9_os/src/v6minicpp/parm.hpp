/*
 * parm.h
 *
 *  Created on: Apr 10, 2017
 *      Author: warlo
 */

#ifndef V6MINI_PARM_HPP_
#define V6MINI_PARM_HPP_

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <sys\types.h>
#include <sys\time.h>
#include <sys\queue.h>
#include <sys\stat.h>
#include <signal.h>
#include <string> // for memset
#include <type_traits>
#ifdef DEBUG
#include <cassert>
#define V6_ASSERT(X) assert(X)
#else
#define V6_ASSERT(X)
#endif

namespace v6 {
	// we include sys\queue.h as we use alot of linked lists
	// and its
	// helper for enums
    constexpr static inline size_t BIT(size_t POS) { return 1<<POS; }
	template<typename T> constexpr static inline T _FLAG_AND(T value1, T value2) {
		using P = std::underlying_type_t <T>;
		return static_cast<T>(static_cast<P>(value1) & static_cast<P>(value2));
	}
	template<typename T> constexpr static inline T _FLAG_OR(T value1, T value2) {
		using P = std::underlying_type_t <T>;
		return static_cast<T>(static_cast<P>(value1) | static_cast<P>(value2));
	}
	template<typename T> constexpr static inline T _FLAG_COMP(T value1) {
		using P = std::underlying_type_t <T>;
		return static_cast<T>(~static_cast<P>(value1));
	}
	template<typename T> constexpr static inline T ISSET(T value1, T value2) {
		return _FLAG_AND(value1,value2) == value1;
	}
#define ENUM_FLAG_HELPER(E) \
		constexpr static inline E  operator | ( E  lhs, E rhs) { return _FLAG_OR(lhs,rhs); } \
		constexpr static inline E& operator |= (E& lhs, E rhs) { return lhs=_FLAG_OR(lhs,rhs); } \
		constexpr static inline E  operator &  (E  lhs, E rhs) { return _FLAG_AND(lhs,rhs); } \
		constexpr static inline E& operator &= (E& lhs, E rhs) { return lhs= _FLAG_AND(lhs,rhs);  } \
		constexpr static inline E  operator ~  (E  lhs) { return _FLAG_COMP(lhs); }


	constexpr static inline dev_t major(dev_t d) { return (d >> 8)&0xFF; }
	constexpr static inline dev_t minor(dev_t d) { return (d)&0xFF; }
	constexpr static inline dev_t mkdev(dev_t mj, dev_t mi) { return ((mj&0xFF) <<8) | (mi&0xFF); }
	constexpr static dev_t NODEV = static_cast<dev_t>(-1);
//class tailq_head;
//class tailq_head<T>;
	// some sync defines we need
	enum irq_prio {
		clock = 0,
		bio = 5,			// buffer io
		normal = 15,

	};
	void sleep(void* chan, int prio) { assert(0); }
	void wakeup(void* chan) { (void)chan; }
	void seterrno(int e) { assert(e); }
	uint32_t splx(uint32_t pri);
	class spl_lock {
		uint32_t _lock;
	public:
		spl_lock(uint32_t v) : _lock(splx(v)) {}
		~spl_lock() { splx(_lock); }
	};
#define splnormal() splx(irq_prio::normal)
#define splbio() splx(irq_prio::bio)
#define splclock() splx(irq_prio::bio)
#define splbiolck spl_lock(irq_prio::bio)
#define splclocklck spl_lock(irq_prio::clock)
};

#endif /* V6MINI_PARM_HPP_ */
