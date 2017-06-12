#ifndef _PRINTK_HPP_
#define _PRINTK_HPP_

#include <cstdint>
#include <cstddef>
#include <limits>
#include<type_traits>
#include <functional>
#include <cassert>
#include <memory>
#include <array>

#include "printk.h"
#include "cstr.hpp"
namespace dbg {
// just dicking around
	struct format_args {
		char fill;
		int width;
		int precission;
		char type;
		constexpr format_args() : fill(' '), width(0), precission(0), type(' ') {}
	};

	struct tag_unsigned {};
	struct tag_signed : tag_unsigned{};
	struct tag_pointer : tag_unsigned{};
	struct tag_string {};
	namespace priv {
		template<typename T>
		typename std::enable_if<std::is_unsigned<T>::value, size_t>::type
		constexpr count_digits(size_t base, T n) {
			return (n >= base) ? 1 + count_digits(base,n/base) : 1;
		}
		template<typename T>
		typename std::enable_if<std::is_signed<T>::value, size_t>::type
		constexpr  count_digits(size_t base, T n) {
			using unsigned_type = typename std::make_unsigned<T>::type;
			return (n < 0) ? 1 + count_digits(base, static_cast<unsigned_type>(-n)) :
					ount_digits(base, static_cast<unsigned_type>(n));
		}
		constexpr bool is_digit(char c) { return c >='0' && c <='9'; }

		template<int N>
		constexpr hel::substring<N> string_to_format(hel::substring<N> str, size_t begin, size_t end) {
			return end == str.end() || (str[end] == '%')  ? hel::substring<N>(str,begin,end) : string_to_format(str,begin,end++);
		}

		template<int N>
		constexpr hel::substring<N> string_to_format(hel::string<N> str) {
			return string_to_format(hel::substring<N>(str,str.begin(),str.end()));
		}
		template<char...cs> struct compile_time_string
		{static constexpr char str[sizeof...(cs)+1] = {cs...,'\0'};};
		template<char...cs>
		const char compile_time_string<cs...>::str[sizeof...(cs)+1];

		template<char...cs> struct compile_time_stringbuilder
		{typedef compile_time_string<cs...> string;};

		//remove leading spaces from stringbuilder
		template<char...cs> struct compile_time_stringbuilder<' ', cs...>
		{typedef typename compile_time_stringbuilder<cs...>::string string;};


		template<int N, typename ... Args>
		void print(const char (*fmt)[N], Args...args){
			hel::string<N> str(fmt);
			for(;;) {
				auto sub = string_to_format<N>(str);
				printk("%s",sub.c_str());
				if(sub.end() == str.end()) return; // done

			}

		}
		template<typename T>
		void _format_arg_tag(T&& v, tag_unsigned){ printk("%d", v);}
		template<typename T>
		void _format_arg_tag(T&& v, tag_signed){ if(v < 0) printk("-%d",-v); else printk("%d",v); }
		template<typename T>
		void _format_arg_tag(T&& v, tag_pointer){ printk("[%p]", v);}
		template<typename T>
		void _format_arg_tag(T&& v, tag_string){ printk("\"%s\"", v);}


		template<typename T, typename std::enable_if<std::is_unsigned<T>::value,int>::type=0>
		void _format_arg(T&& v) { _format_arg_tag(v, tag_unsigned{}); }
		template<typename T, typename std::enable_if<std::is_signed<T>::value,int>::type=0>
		void _format_arg(T&& v) { _format_arg_tag(v, tag_signed{}); }
		template<typename T, typename std::enable_if<(std::is_pointer<T>::value && !std::is_same<T,const char*>::value),int>::type=0>
		void _format_arg(T&& v) { _format_arg_tag(v, tag_pointer{}); }

		template<typename T, typename std::enable_if<std::is_same<T,const char*>::value,int>::type=0>
		void _format_arg(T&& v) { _format_arg_tag(v, tag_string{}); }

		template<typename T>
		void _arg_debug(T v) {
			_format_arg(std::forward<T>(v));
		}
		template<typename T,typename ...Args>
		void _arg_debug(T v, Args...args) {
			_arg_debug(std::forward<T>(v));
			printk(" ,");
			_arg_debug(std::forward<Args>(args)...);
		}
	}

	template<typename ...Args>
	void arg_debug(const char* str, Args...args) {
		printk("arg_debug:  %s(",str);
		priv::_arg_debug(std::forward<Args>(args)...);
		printk(")\n");
	}

}



















#endif
