#ifndef F9_CONF_HPP_
#define F9_CONF_HPP_

#include <cstdint>
#include <cstddef>

#define CONFIG_DEBUG

//#define CONFIG_ENABLE_FPAGE
namespace f9 {

// Limitations
	static constexpr size_t CONFIG_MAX_THREADS=32;
	static constexpr size_t CONFIG_MAX_KT_EVENTS=64;
	static constexpr size_t CONFIG_MAX_ASYNC_EVENTS=32;
	static constexpr size_t CONFIG_MAX_ADRESS_SPACES=16;
	static constexpr size_t CONFIG_MAX_FPAGES=256;

	// kernel timer
	//CONFIG_KTIMER_TICKLESS=y
	//# CONFIG_KTIMER_TICKLESS_VERIFY is not set
	static constexpr size_t CONFIG_KTIMER_TICKLESS_COMPENSATION=0;
	static constexpr size_t CONFIG_KTIMER_TICKLESS_INT_COMPENSATION=0;
	static constexpr size_t CONFIG_KTIMER_HEARTBEAT=65536;
	static constexpr size_t CONFIG_KTIMER_MINTICKS=128;

	// Flexible page tweaks
	static constexpr size_t CONFIG_LARGEST_FPAGE_SHIFT=16;
	static constexpr size_t CONFIG_LARGEST_FPAGE_SIZE = (1 << CONFIG_LARGEST_FPAGE_SHIFT);
	static constexpr size_t CONFIG_SMALLEST_FPAGE_SHIFT=8;
	static constexpr size_t CONFIG_SMALLEST_FPAGE_SIZE = (1 << CONFIG_SMALLEST_FPAGE_SHIFT);
	// Thread tweaks
	static constexpr size_t CONFIG_INTR_THREAD_MAX=256;


	// KIP tweaks

	static constexpr size_t CONFIG_KIP_EXTRA_SIZE=128;
};








#endif
