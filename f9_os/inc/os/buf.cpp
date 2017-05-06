/*
 * buf.cpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#include <os/buf.hpp>

namespace os {
namespace internal {
// Static data is placed in this class template to allow header-only
	// configuration.
	template <typename T = void>
	struct BasicData {
	  static const uint32_t POWERS_OF_10_32[];
	  static const uint64_t POWERS_OF_10_64[];
	  static const char DIGITS[];
	};

	#if FMT_USE_EXTERN_TEMPLATES
	extern template struct BasicData<void>;
	#endif
	typedef BasicData<> Data;
	template <typename T>
	const char BasicData<T>::DIGITS[] =
	    "0001020304050607080910111213141516171819"
	    "2021222324252627282930313233343536373839"
	    "4041424344454647484950515253545556575859"
	    "6061626364656667686970717273747576777879"
	    "8081828384858687888990919293949596979899";

	#define FMT_POWERS_OF_10(factor) \
	  factor * 10, \
	  factor * 100, \
	  factor * 1000, \
	  factor * 10000, \
	  factor * 100000, \
	  factor * 1000000, \
	  factor * 10000000, \
	  factor * 100000000, \
	  factor * 1000000000

	template <typename T>
	const uint32_t BasicData<T>::POWERS_OF_10_32[] = {
	  0, FMT_POWERS_OF_10(1)
	};

	template <typename T>
	const uint64_t BasicData<T>::POWERS_OF_10_64[] = {
	  0,
	  FMT_POWERS_OF_10(1),
	  FMT_POWERS_OF_10(uint64_t(1000000000)),
	  // Multiply several constants instead of using a single long long constant
	  // to avoid warnings about C++98 not supporting long long.
	  uint64_t(1000000000) * uint64_t(1000000000) * 10
	};
	// Returns the number of decimal digits in n. Leading zeros are not counted
	// except for n == 0 in which case count_digits returns 1.


	inline uint32_t count_digits(uint64_t n) {
	  // Based on http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
	  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
	  int t = (64 - __builtin_clzll(n | 1)) * 1233 >> 12;
	  return to_unsigned(t) - (n < Data::POWERS_OF_10_64[t]) + 1;
	}
	inline uint32_t count_digits(uint32_t n) {
		int t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
	  return to_unsigned(t) - (n < Data::POWERS_OF_10_32[t]) + 1;
	}
	// formaters
	// A functor that doesn't add a thousands separator.
	struct NoThousandsSep {
	  template <typename Char>
	  void operator()(char*) {}
	};

	// A functor that adds a thousands separator.
	class ThousandsSep {
	 private:
	  const char* sep_;

	  // Index of a decimal digit with the least significant digit having index 0.
	  uint32_t digit_index_;

	 public:
	  explicit ThousandsSep(const char* sep) : sep_(sep), digit_index_(0) {}

	  template <typename Char>
	  void operator()(char* buffer) {
		if (++digit_index_ % 3 != 0) return;
		const char* sep = sep_;
		while(*sep) *buffer++ = *sep++;
	  }
	};

	// Formats a decimal unsigned integer value writing into buffer.
	// thousands_sep is a functor that is called after writing each char to
	// add a thousands separator if necessary.
	template <typename UInt, typename ThousandsSep>
	inline void format_decimal(file_operations &u, UInt value, size_t num_digits, ThousandsSep thousands_sep) {
		char _buffer[23]; // resonable size?
		char* buffer = _buffer;
		buffer += num_digits;
	  while (value >= 100) {
	    // Integer division is slow so do it for a group of two digits instead
	    // of for every digit. The idea comes from the talk by Alexandrescu
	    // "Three Optimization Tips for C++". See speed-test for a comparison.
	    unsigned index = static_cast<unsigned>((value % 100) * 2);
	    value /= 100;
	    *--buffer = Data::DIGITS[index + 1];
	    thousands_sep(buffer);
	    *--buffer = Data::DIGITS[index];
	    thousands_sep(buffer);
	  }
	  if (value < 10) {
	    *--buffer = static_cast<char>('0' + value);
	    return;
	  }
	  unsigned index = static_cast<unsigned>(value * 2);
	  *--buffer = Data::DIGITS[index + 1];
	  thousands_sep(buffer);
	  *--buffer = Data::DIGITS[index];
	  u.write(static_cast<uint8_t*>(buffer), num_digits);
	}

	template <typename UInt>
	inline void format_decimal(file_operations &u, UInt value, size_t num_digits) {
	  format_decimal(u, value, num_digits, NoThousandsSep());
	  return;
	}
};






} /* namespace os */
