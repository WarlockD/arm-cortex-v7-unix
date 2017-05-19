/*
 * inithook.cpp
 *
 *  Created on: May 17, 2017
 *      Author: Paul
 */

#include "init_hook.hpp"
#include <algorithm>

namespace f9 {
	extern const init_hook init_hook_start[];
	extern const init_hook init_hook_end[];
	static uint32_t last_level = 0;

	int init_hook::run(uint32_t level)
	{
		uint32_t max_called_level = last_level;

		for (const init_hook *ptr = init_hook_start;
		     ptr != init_hook_end; ++ptr)
			if ((ptr->_level > last_level) &&
			    (ptr->_level <= level)) {
				max_called_level = std::max(max_called_level, ptr->_level);
				ptr->_hook();
			}

		last_level = max_called_level;

		return last_level;
	}

} /* namespace f9 */

