#ifndef _CLIST_HPP_
#define _CLIST_HPP_

#include <cstdint>
#include <cstddef>


class clist {
	size_t _size;
	char* _first;
	char* _last;
public:
	clist() : _size(0), _first(nullptr),_last(nullptr) {}
	int get();
	int put(int c);
	void get(char* data, size_t size);
	void put(const char* data, size_t size);
	size_t size() const; // sizeof continugious charaters
	size_t ndflush(size_t count);

};


#endif