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
#include <diag\Trace.h>
#include <sys\queue.h>
#include <os\atomic.h>

// utility helpers

// template string helper
namespace tstring {
// size of a string
	constexpr size_t tstrlen(char const* str, size_t count= 0) { return ('\0' == str[0]) ? count : tstrlen(str+1, count+1); }


	// just a string wrapper
	class strref {
		const char* _begin;
		const char* _end;
		constexpr static size_t _strlen(const char * str, size_t count= 0) { return ('\0' == str[0]) ? count : tstrlen(str+1, count+1); }
		constexpr static bool _strequal(const char * r,const char * l, size_t size) { return *r == *l ? (size==0 ? true : _strequal(++r,++l,--size)) : false;   }
	public:
		constexpr strref(const char* b, const char* e) : _begin(b), _end(e) {}
		constexpr strref(const char* str, size_t size) :strref(str, str+ _strlen(str)) {}
		constexpr strref(const char* str) : strref(str,_strlen(str)) {}
		constexpr size_t size() const { return static_cast<size_t>(_end-_begin); }

		constexpr const char* begin() const { return _begin; }
		constexpr const char* end() const { return _begin; }
		constexpr bool operator==(const strref& r) const { return (_begin == r._begin && _begin == r._end) || (size() == r.size() && _strequal(_begin,r._end,size())); }
		constexpr bool operator!=(const strref& r) const { return (_begin != r._begin || _begin != r._end) || size() != r.size() || !_strequal(_begin,r._end,size()); }
	};
};


#endif /* OS_UTILITY_HPP_ */
