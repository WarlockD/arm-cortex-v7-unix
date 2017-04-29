/*
 * memory.cpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#include <os/memory.hpp>
#include "bitmap.hpp"

namespace os {

	using ktable = bitops::bitmap_table_t<as_t,as_t::CONFIG_MAX_ADRESS_SPACES>;
 	static ktable as_table;
 	void* operator as_t::new(size_t size) { return as_table.allocate(size); }
 	void operator as_t::delete(void* ptr){ as_table.free(ptr);; }


};
