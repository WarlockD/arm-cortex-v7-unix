#ifndef ARCH_ARCH_HPP
#define ARCH_ARCH_HPP

#include <cstdint>
#include <cstddef>

#define CORTEX_M7
#ifdef CORTEX_M7
#include "cortex_m7.hpp"

namespace arch = cortex_m7;

namespace os {
	using reg = arch::reg;
}
#endif



#endif
