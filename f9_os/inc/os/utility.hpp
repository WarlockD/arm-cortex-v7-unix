/*
 * utility.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_UTILITY_HPP_
#define OS_UTILITY_HPP_

#include <cstdint>
#include <cstddef>
#include <limits>
#include <functional>
#include <cassert>
#include <memory>
#include <os\printk.h>
#include <sys\queue.h>
#include <os\atomic.h>

// utility helpers

// template string helper
namespace tstring {
// size of a string
constexpr size_t tstrlen(char const* str, size_t count= 0) { return ('\0' == str[0]) ? count : tstrlen(str+1, count+1); }

};


#endif /* OS_UTILITY_HPP_ */
