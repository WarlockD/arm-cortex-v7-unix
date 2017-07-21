/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DIAG_TRACE_H_
#define DIAG_TRACE_H_

// ----------------------------------------------------------------------------

#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// ----------------------------------------------------------------------------

// The trace device is an independent output channel, intended for debug
// purposes.
//
// The API is simple, and mimics the standard output calls:
// - trace_printf()
// - trace_puts()
// - trace_putchar();
//
// The implementation is done in
// - trace_write()
//
// Trace support is enabled by adding the TRACE definition.
// By default the trace messages are forwarded to the ITM output,
// but can be rerouted via any device or completely suppressed by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//
// When TRACE is not defined, all functions are inlined to empty bodies.
// This has the advantage that the trace call do not need to be conditionally
// compiled with #ifdef TRACE/#endif

#if defined(TRACE)

// puts a timestamp at the start of each line
#define TRACE_ENABLE_TIMESTAMP

#if defined(__cplusplus)
extern "C"
{
#endif


  void trace_initialize(void);

  // Implementation dependent
  int trace_write(const char* buf, size_t nbyte);

  // ----- Portable -----
  int trace_vprintf(const char* format, va_list ap);
  int trace_printf(const char* format, ...);
  int trace_puts(const char *s);
  int trace_putchar(int c);
  void trace_dump_args(int argc, char* argv[]);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
#include <type_traits>
#include <functional>

namespace trace {
	// type printer
	template <typename T, typename = void>
	struct _format_settings_impl { static constexpr bool valid = false; };

	template<typename T>
	struct _format_settings_impl<T,typename std::enable_if<sizeof(std::decay<T>::type) == 1>::type> {
		using value_type = typename std::remove_cv<T>::type;
		static constexpr bool valid = true;
		inline static void print(value_type v) {
			trace_printf("%01X",v);
		}
	};
	template<typename T>
	struct _format_settings_impl<T,typename std::enable_if<sizeof(std::decay<T>::type) == 2>::type> {
		using value_type = typename std::remove_cv<T>::type;
		static constexpr bool valid = true;
		inline static void print(value_type v) {
			trace_printf("%04X",v);
		}
	};
	template<typename T>
	struct _format_settings_impl<T,typename std::enable_if<sizeof(std::decay<T>::type) == 4>::type> {
		using value_type = typename std::remove_cv<T>::type;
		static constexpr bool valid = true;
		inline static void print(value_type v) {
			trace_printf("%08X",v);
		}
	};
	// https://crazycpp.wordpress.com/2014/10/17/compile-time-strings-with-constexpr/
	// https://github.com/crazy-eddie/crazycpp/blob/master/20141016-constexprstr/include/string.hpp
	//https://github.com/irrequietus/typestring/blob/master/typestring.hh
	namespace priv {
		// god I love c++14
		template<size_t SIZE>
		class string
		{
			char _c_str[SIZE+1];
			  template < size_t I, size_t ... Indices >
			    constexpr string(char const (&str) [I], std::integer_sequence<size_t,Indices...>)
			        : _c_str{str[Indices]..., '\0'}
			    {}

			template <size_t I, size_t ... IS>
			   constexpr string(char const (&str)[I], std::integer_sequence<size_t,SIZE> )
				   : _c_str{str[IS]..., '\0'}
			   {}

			template <size_t ... ThisIndices
					 ,size_t ... ThatIndices
					 ,size_t I>
			constexpr string<SIZE+I-1> append( std::integer_sequence<size_t,ThisIndices...>
											 , std::integer_sequence<size_t,ThatIndices...>
											 , char const (&cstr) [I] ) const
			{
				char const newArr[] = {_c_str[ThisIndices]..., cstr[ThatIndices]..., '\0'};
				return string<SIZE+I-1>(newArr);
			}
		public:
			using data_type = char const (&) [SIZE+1];
			constexpr int size() const { return SIZE; }
			constexpr data_type c_str() const { return _c_str; }
			constexpr string(char const (&str) [SIZE+1]) : string(str, std::integer_sequence<size_t,SIZE>{} ) {}
			constexpr string() : _c_str{} {}
		    template <size_t I>
		    constexpr string<SIZE+I-1> append(char const (&cstr)[I]) const
		    {
		        return append(std::integer_sequence<size_t,SIZE>{},std::integer_sequence<size_t,I-1>{},cstr);
		    }
		};
#if 0
		template<>
		struct string<0> {
			using data_type = char const (&) [1];
			constexpr int size() const { return 0; }
			constexpr data_type c_str() const { return _c_str; }
			constexpr string(char const (&str)[1]) : _c_str("") { (void)str; }
			constexpr string() : _c_str{} {}
		    template <size_t I>
		    constexpr string<I> append(char const (&cstr)[I]) const { return cstr; }
		private:
			char _c_str[1];
		};
#endif
		template <size_t I0, size_t I1>
		constexpr string<I0+I1> operator +(string<I0> const& s0, string<I1> const& s1) { return s0.append(s1); }
		template<size_t SIZE> constexpr auto make_string(char const (&cstr) [SIZE]){ return string<SIZE-1>(cstr);}

		// we use this to figure out some basic information
		template<typename T>
		struct _type_info_t {
			using type = typename std::decay<T>::type;
			static constexpr bool is_pointer = std::is_pointer<type>::value;
			using value_type = typename std::conditional<is_pointer,typename std::remove_pointer<type>::type,type>::type;
			static constexpr bool is_arithmetic = std::is_arithmetic<value_type>::value;
			static constexpr bool is_signed = is_arithmetic && std::is_signed<value_type>::value;
			static constexpr size_t size = sizeof(type);
			static constexpr bool is_basic =  !is_pointer && size <=4;
			static constexpr bool is_uint8 =  !is_pointer && !is_signed && size == 1;
			static constexpr bool is_uint16 = !is_pointer && !is_signed && size == 2;
			static constexpr bool is_uint32 = !is_pointer && !is_signed && size == 4;
			static constexpr bool is_int8 =   !is_pointer &&  is_signed && size == 1;
			static constexpr bool is_int16 =  !is_pointer &&  is_signed && size == 2;
			static constexpr bool is_int32 =  !is_pointer &&  is_signed && size == 4;
		};
#define TRY_CONSTEXPR_STRING

		template<typename T, typename = void> struct type_info_t {};
#ifdef TRY_CONSTEXPR_STRING
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_basic>::type> {
			using ti = _type_info_t<T>;
			static constexpr auto name = (ti::is_signed ? make_string("") : make_string("u")) + make_string("int");
			static constexpr auto hex_format = make_string("%02X");
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_pointer>::type> {
			static constexpr auto name = make_string("int");
			static constexpr auto hex_format = make_string("%02X");
		};
#else
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_uint8>::type> {
			static constexpr auto name() { return "uint8"; }
			static constexpr auto hex_format() { return "%02X"; }
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_int8>::type> {

			static constexpr auto name() { return "int8"; }
			static constexpr auto hex_format() { return "%02X"; }
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_uint16>::type> {
			static constexpr auto name() { return "uint16"; }
			static constexpr auto hex_format() { return "%04X"; }
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_int16>::type> {
			static constexpr auto name() { return "int16"; }
			static constexpr auto hex_format() { return "%04X"; }
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_uint32>::type> {
			static constexpr auto name() { return "uint32"; }
			static constexpr auto hex_format() { return "%08X"; }
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_int32>::type> {
			static constexpr auto name() { return "int32"; }
			static constexpr auto hex_format() { return "%08X"; }
		};
		template<typename T>
		struct type_info_t<T,typename std::enable_if<_type_info_t<T>::is_pointer>::type> {
			using type_info = _type_info_t<T>;
			using rtype_ino = typename type_info_t<type_info::value_type>;
			static constexpr auto name() { return concat(rtype_ino::name(),"*"); }
			static constexpr auto hex_format() { return concat("[",concat(rtype_ino::hex_format(),"]")); }
		};
#endif
	}
	template<typename T>
	static inline void _print(T&& v) {
		using type_info = priv::type_info_t<T>;
	//	assert(type_info::valid);
	//	static_assert(type_info::valid,"Not a valid type? no info?");)
#ifdef TRY_CONSTEXPR_STRING
		trace_printf(type_info::hex_format.c_str(),v);
#else
		trace_printf(type_info::hex_format(),v);
#endif
	}

	template<typename T>
	static inline void _print_array(const T* a, size_t s) {
		using type_info = priv::type_info_t<T>;
#ifdef TRY_CONSTEXPR_STRING
		trace_printf("array(%08X)<%s,%i>(", a, type_info::name.c_str(), s);
#else
		trace_printf("array(%08X)<%s,%i>(", a, type_info::name(), s);
#endif
		for(size_t i=0; i < s; i++) {
			_print(a[i]);
			if(i< (s-1)) trace_putchar(' ');
		}
		trace_printf(")");
	}

	// used for arrays
	template<typename T,typename ST>
	static inline void _print(std::pair<const T*,ST> && p) {
		_print_array(p.first,static_cast<size_t>(p.second));
	}

	//template< typename T>
	static inline void _print() { }


	static inline void _print(const char* s) { trace_puts(s); }


	template<typename T,  typename ...Args>
	static inline void _print(T&& v, Args ... args) { _print(v); trace_putchar(' '); _print(std::forward<Args>(args)...); }

	template<typename ...Args>
	static inline void print(Args ... args) {
		_print(std::forward<Args>(args)...);
	}
	template<typename ...Args>
	static inline void printn(Args ... args) {
		_print(std::forward<Args>(args)...);
		trace_putchar('\n');
	}

}




#endif

#else // !defined(TRACE)

#if defined(__cplusplus)
extern "C"
{
#endif
uint32_t trace_critical_begin();
void trace_critcal_end(uint32_t flags);
  inline void
  trace_initialize(void);

  // Implementation dependent
  inline ssize_t
  trace_write(const char* buf, size_t nbyte);

  inline int
  trace_printf(const char* format, ...);

  inline int
  trace_puts(const char *s);

  inline int
  trace_putchar(int c);

  inline void
  trace_dump_args(int argc, char* argv[]);

#if defined(__cplusplus)
}
#endif

inline void
__attribute__((always_inline))
trace_initialize(void)
{
}

// Empty definitions when trace is not defined
inline ssize_t
__attribute__((always_inline))
trace_write(const char* buf __attribute__((unused)),
    size_t nbyte __attribute__((unused)))
{
  return 0;
}

inline int
__attribute__((always_inline))
trace_printf(const char* format __attribute__((unused)), ...)
  {
    return 0;
  }

inline int
__attribute__((always_inline))
trace_puts(const char *s __attribute__((unused)))
{
  return 0;
}

inline int
__attribute__((always_inline))
trace_putchar(int c)
{
  return c;
}

inline void
__attribute__((always_inline))
trace_dump_args(int argc __attribute__((unused)),
    char* argv[] __attribute__((unused)))
{
}

#endif // defined(TRACE)

// ----------------------------------------------------------------------------

#endif // DIAG_TRACE_H_
