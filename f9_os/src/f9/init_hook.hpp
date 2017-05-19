/*
 * init_hook.hpp
 *
 *  Created on: May 17, 2017
 *      Author: Paul
 */

#ifndef F9_INIT_HOOK_HPP_
#define F9_INIT_HOOK_HPP_
#include "types.hpp"

namespace f9 {
	enum INIT_LEVEL {
		EARLIEST		= 1,
		PLATFORM_EARLY 	= 0x1000,
		PLATFORM		= 0x2000,
		KERNEL_EARLY	= 0x3000,
		KERNEL			= 0x4000,
		LAST			= 0xFFFFFFFF,
	};
	typedef void (*init_hook_t)(void);


	class init_hook {
		uint32_t _level;
		init_hook_t _hook;
		const char* _name;
	public:
		init_hook(uint32_t level,init_hook_t hook, const char* name ) :
			_level(level), _hook(hook), _name(name) {}
		init_hook(INIT_LEVEL level,init_hook_t hook, const char* name ) :
			_level(static_cast<uint32_t>(level)), _hook(hook), _name(name) {}
		static int run(uint32_t level);
		static int run(INIT_LEVEL level) { return run(static_cast<uint32_t>(level)); }
	}; // __attribute__((section(".init_hook")));
#define INIT_HOOK(_hook, _level)					\
	const init_hook _init_struct_##_hookm __attribute__((section(".init_hook")))(_level, _hook, #_hook);

} /* namespace f9 */

#endif /* F9_INIT_HOOK_HPP_ */
