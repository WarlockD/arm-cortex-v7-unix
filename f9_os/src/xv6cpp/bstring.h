/*
 * bstring.h
 *
 *  Created on: Apr 25, 2017
 *      Author: Paul
 */

#ifndef XV6CPP_BSTRING_H_
#define XV6CPP_BSTRING_H_

#include <cstdint>
#include <cstddef>

namespace blib {


class string {
	struct _istring {
		uint8_t _size;
		char  _refs; // -1 means its permanently interned
		_istring* _hash_link;
	};
	static constexpr size_t SIZE_OFFSET = 0;
	static constexpr size_t REF_OFFSET = 1;
	static constexpr size_t DATA_OFFSET = 2;
	const char* _data;
public:
	string(const char* str);
	string() : _data(nullptr) {}
	~string();
	const char* c_str() const { return _data + DATA_OFFSET; }
	size_t size() const { return static_cast<uint8_t>(_data[SIZE_OFFSET]); }
	bool operator==(const string& r) const { return _data == r._data; }
	bool operator!=(const string& r) const { return _data != r._data; }
};

} /* namespace blib */

#endif /* XV6CPP_BSTRING_H_ */
