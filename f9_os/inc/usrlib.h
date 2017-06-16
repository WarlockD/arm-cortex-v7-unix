//******************************************************************************
//*
//*     FULLNAME:  Single-Chip Microcontroller Real-Time Operating System
//*
//*     NICKNAME:  scmRTOS
//*               
//*     PURPOSE:  User Suport Library Header
//*               
//*     Version: v5.1.0
//*
//*
//*     Copyright (c) 2003-2016, scmRTOS Team
//*
//*     Permission is hereby granted, free of charge, to any person 
//*     obtaining  a copy of this software and associated documentation 
//*     files (the "Software"), to deal in the Software without restriction, 
//*     including without limitation the rights to use, copy, modify, merge, 
//*     publish, distribute, sublicense, and/or sell copies of the Software, 
//*     and to permit persons to whom the Software is furnished to do so, 
//*     subject to the following conditions:
//*
//*     The above copyright notice and this permission notice shall be included 
//*     in all copies or substantial portions of the Software.
//*
//*     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
//*     EXPRESS  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
//*     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
//*     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
//*     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
//*     TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
//*     THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*
//*     =================================================================
//*     Project sources: https://github.com/scmrtos/scmrtos
//*     Documentation:   https://github.com/scmrtos/scmrtos/wiki/Documentation
//*     Wiki:            https://github.com/scmrtos/scmrtos/wiki
//*     Sample projects: https://github.com/scmrtos/scmrtos-sample-projects
//*     =================================================================
//*
//******************************************************************************

#ifndef USRLIB_H
#define USRLIB_H

#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>
#include <type_traits>
//------------------------------------------------------------------------------
//
//  DESCRIPTON: user namespace for some useful types and functions
//
//
namespace usr
{
	namespace priv {
	// https://github.com/arobenko/embxx/blob/master/embxx/util/SizeToType.h
		// wierd sizes, can I use std::enable_if for these ranges?
		template <std::size_t  _SIZE,  typename E = void> struct size_to_type;
		template<std::size_t  _SIZE>
		struct size_to_type<_SIZE, typename std::enable_if<(_SIZE > 2 && _SIZE <= 4)>> { using type = std::uint32_t; };
		template<std::size_t  _SIZE>
		struct size_to_type<_SIZE, typename std::enable_if<(_SIZE > 4 && _SIZE <= 8)>> { using type = std::uint64_t; };
		template<>
		struct size_to_type<1> { using type = std::uint8_t; };
		template<>
		struct size_to_type<2> { using type = std::uint16_t; };

	};
	// help from https://arobenko.gitbooks.io/bare_metal_cpp/content/basic_needs/queue.html
	template<typename T, typename _SIZE_TYPE = std::size_t>
	class buffer_base {
	   	using value_type = T;
		using refrence = value_type&;
		using pointer = value_type*;
		using const_refrence = const value_type&;
		using const_pointer = const value_type*;
		using iterator = pointer;
		using const_iterator = const_pointer;

    	using size_type = _SIZE_TYPE;
    	static constexpr size_t ELEMENT_SIZE = sizeof(T);
    	using type = buffer_base<T,_SIZE_TYPE>;

	protected:
    	// helper here
		template<typename U>
		using is_simple = typename std::integral_constant<bool, std::is_pod<U>::value || std::is_arithmetic<U>::value || std::is_trivial<U>::value>;
        using storage_type =  typename std::aligned_storage<sizeof(value_type),std::alignment_of<value_type>::value>::type;
    	using storage_pointer = storage_type*;

    	template<typename U>
    	bool push_back(U&& new_elm) {
    		if(_count == size()) return false;
			auto* ptr = &_data[_count++];
			new(ptr) value_type(std::forward<U>(new_elm));
    		return true;
    	}

    	void clear() { _count = _start_idx; }

    	template<typename U = T>

    	void pop_back() {
    		if(_count ==0) return;
    		auto* space = &_data[--_count];
    		auto* elm  = reinterpret_cast<value_type*>(space);
    		if(!is_simple<T>::value) elm->~T();
    	}

    	template<typename U>
    	bool pop_back(U&& v) {
    		if(_count == 0) return false;
    		auto* space = &_data[--_count];
    		auto* elm  = reinterpret_cast<value_type*>(space);
    		v = std::move(*elm);
    		return true;
    	}
    	constexpr inline size_type size() const { return _capacity; }
    	constexpr inline size_type count() const { return _count; }
    	constexpr inline size_type free() const { return _capacity - _count - _start_idx; }
    	pointer begin() { return _data; }
    	pointer end() { return _data + _count; }
    	const pointer begin() const{ return _data; }
    	const pointer end() const{ return _data+ _count; }
    	constexpr buffer_base(storage_pointer data, size_type capacity) : _data(data), _capacity(capacity), _start_idx(0), _count(0) {}
    	uint8_t* data() { return _data; }
    	const uint8_t* data() const { return _data; }
	private:
    	storage_pointer _data;
    	size_type _capacity;
    	size_type _start_idx;
    	size_type _count;		// elements in queue
	};
    //------------------------------------------------------------------------------
    //
    ///     The Circular Buffer
    //
    ///         Byte-wide FIFO.
    //
    ///         Allows to:
    ///             add byte,
    ///             get byte,
    ///             write bytes from array,
    ///             read bytes to array,
    ///             and some other service actions.
    //
    class TCbuf
    {
    public:
    	TCbuf() : _buf(nullptr), _size(0), _count(0),_first(0), _last(0) {}
        TCbuf(uint8_t* const Address, const size_t Size);
        bool write(const uint8_t* data, const size_t Count);
        void read(uint8_t* const data, const size_t Count);
        size_t get_count() const { return _count; }
        size_t get_free_size() const { return _size - _count; }
        uint8_t get_byte(const size_t index) const;
        void clear() { _count = 0; _last = _first; }
        bool put(const uint8_t item);
        int get();
        size_t size() const { return _size; }
    private:
       //------------------------------------------------------------------------------
       //
       //  DESCRIPTON: For internal purposes
       //
        void push(const uint8_t item); ///< Use this function with care - it doesn't perform free size check
        uint8_t pop();                 ///< Use this function with care - it doesn't perform count check
       //------------------------------------------------------------------------------

    private:
        uint8_t* _buf;
        size_t  _size;
        volatile size_t _count;
        size_t  _first;
        size_t  _last;
    };
#if 0
    //------------------------------------------------------------------------------
    class TBuf {
        uint8_t* _buf;
    	const size_t  _size;
    	volatile size_t  _pos;
    public:
    	uint8_t* begin() { return _buf; }
    	uint8_t* end() { return _buf+ _pos; }
    	const uint8_t* begin() const{ return _buf; }
    	const uint8_t* end() const{ return _buf+ _pos; }
    	uint8_t& operator[](size_t i) { return _buf[i]; }
    	uint8_t operator[](size_t i) const { return _buf[i]; }
    	uint8_t* data() { return _buf; }
    	const uint8_t* data() const { return _buf; }
        TBuf() : _buf(nullptr), _size(0), _pos(0)  {}
        TBuf(uint8_t* buf, size_t size) : _buf(buf), _size(size), _pos(0)  {}
    	template<size_t N>
    	TBuf(uint8_t (&arr)[N]): _buf(&arr[0]), _size(N), _pos(0)  {}
    	template<size_t N>
    	TBuf(char (&arr)[N]): _buf(static_cast<char*>(&arr[0])), _size(N), _pos(0)  {}
    	// no copy
    	TBuf(const TBuf& copy) = delete;
    	TBuf& operator=(const TBuf& copy) = delete;
    	// move is fine though
    	TBuf(TBuf&& move) = default;
    	TBuf& operator=(TBuf&& move) = default;

        size_t count() const { return _pos; }
        void clear() { _pos = 0; }
        size_t size() const { return _size; }
        size_t write(const uint8_t* data, const size_t cnt) {
    		const size_t items = cnt <= count() ? cnt : size();
    	    std::copy_n(data,items,&_buf[_pos]);
    	    _pos+=items;
    	    return items;
        }
        // write helpers
        inline size_t write(const char* data, const size_t cnt) { return write(reinterpret_cast<const uint8_t*>(data),cnt); }
    	template<typename T>
    	size_t write(const T* data, const size_t cnt) { return write(reinterpret_cast<const uint8_t*>(data),cnt*sizeof(T))/sizeof(T);  }
    	template<typename T, size_t N>
    	size_t write(const T (&arr)[N]) { return write(reinterpret_cast<const uint8_t*>(&arr[0]),N*sizeof(T))/sizeof(T);  }

    	size_t read(uint8_t* data, const size_t cnt)
    	{
    		size_t items = cnt <= count() ? cnt : size();
    	    std::copy_n(_buf,items,data);
    		if(items < count())
        	    std::copy_n(&_buf[_pos],items,&_buf[0]);
    		_pos-=items;
    		return items;
    	}

		// read helpers
		inline size_t read(char* data, const size_t cnt) { return read(reinterpret_cast<uint8_t*>(data),cnt); }
		template<typename T>
		size_t read(T* data, const size_t cnt) { return read(reinterpret_cast<uint8_t*>(data),cnt*sizeof(T))/sizeof(T);  }
		template<typename T, size_t N>
		size_t read(T (&arr)[N]) { return read(reinterpret_cast<uint8_t*>(&arr[0]),N*sizeof(T))/sizeof(T);  }

    	bool push_back(uint8_t v) {
    		if(_pos == size()) return false;
    		_buf[_pos++] = v;
    		return true;
    	}
    	bool pop_back(uint8_t& v) {
    		if(_pos == 0) return false;
    		v = std::move(_buf[--_pos]);
    		return true;
    	}
    	bool pop_back(char& v) { return pop_back(reinterpret_cast<uint8_t&>(v)); }
    	bool push_back(char v) { return push_back(static_cast<uint8_t>(v)); }

    	template<typename T>
    	bool pop_back(T& v) { return read(&v,1) == 1; }

    	template<typename T>
    	bool push_back(T v) { return write(&v,1) == 1; }

    	template<typename T>
    	bool push_back(const T& v) {
    		return 1 == write(v,1);
    	}
    	template<typename T>
    	bool pop_back(const T& v) {
    		return 1 == read(v,1);
    	}
    };
    // one way buffer
    template<typename T, std::size_t _COUNT, typename _SIZE_TYPE = std::size_t>
    class buffer {

    public:
    	using size_type = _SIZE_TYPE;
    	static constexpr size_t COUNT = _COUNT;
    	static constexpr size_t ELEMENT_SIZE = sizeof(T);
    	static constexpr size_t ARRAY_SIZE = ELEMENT_SIZE * COUNT;
    	using type = buffer<T,_COUNT,_SIZE_TYPE>;

    	using value_type = T;
    	using refrence = value_type&;
    	using pointer = value_type*;
    	using const_refrence = const value_type&;
    	using const_pointer = const value_type*;
    	using iterator = pointer;
       	using const_iterator = const_pointer;

    	constexpr size_t size() const { return _buf.size(); }
    	size_type count() const { return _pos; }
    	size_type free() const { return size() - _pos; }
    	iterator begin() { return  _buf.begin(); }
    	const_iterator begin() const { return  _buf.begin(); }
    	iterator end() { return  _buf.begin()  + _pos;}
    	const_iterator end() const { return  _buf.begin()  + _pos; }
    	refrence operator[](size_t i) { return _buf[i]; }
    	const_refrence operator[](size_t i) const { return _buf[i]; }
    	const_pointer data() const { return _buf.data(); }

    	template<typename U>
    	bool push_back(U&& new_elm) {
    		if(_pos == size()) return false;
			auto* ptr = &_buf[_pos++];
			new(ptr) value_type(std::forward<U>(new_elm));
    		return true;
    	}
    	template<typename U>
    	bool pop_back(U&& v) {
    		if(_pos == 0) return false;
    		auto* space = &_buf[--_pos];
    		auto* elm  = reinterpret_cast<value_type*>(space);
    		v = std::move(*elm);
    		return true;
    	}
    	void clear() { _pos = 0; }

    	template<size_t N>
    	size_type write(value_type (&arr)[N]) {
    		const size_t items = N <= count() ? N : size();
    	    std::copy(data,data+items,&_buf[_pos]);
    	    _pos+=N;
    	    return items;
    	}
    	size_type write(const_pointer data, const size_t cnt)
    	{
    		const size_type items = cnt <= count() ? cnt : size();
    	    std::copy(data,data+items,&_buf[_pos]);
    	    _pos+=items;
    	    return items;
    	}
    	template<size_t N>
    	size_type read(value_type (&arr)[N]) {
    		const size_type items = N <= count() ? N : SIZE;
    		std::copy(begin(),begin() + items, data);
    		if(items <= count()){
        		std::copy(begin()+ N,end(), begin()); // move the data
    		}
    		return items;
    	}
    	size_type read(pointer data, const size_t cnt)
    	{
    		size_t items = cnt <= count() ? cnt : SIZE;
    		std::copy(begin(),begin() + items, data);
    		if(items <= count()){
        		std::copy(begin()+ cnt,end(), begin()); // move the data
    		}
    		return items;
    	}
    private:
        using storage_type =  typename std::aligned_storage<sizeof(value_type),std::alignment_of<value_type>::value>::type;
        using array_type = std::array<T,_COUNT>;
        size_type _pos;
    	array_type _buf;
    };
#endif
    //-----------------------------------------------------------------------
    //
    ///     The Ring Buffer Template
    ///
    ///         Carries out FIFO functionality for
    ///         arbitrary data types
    ///
    ///         Allows to:
    ///             add item to back (default),
    ///             add item to front,
    ///             get item at front (default),
    ///             get item from back,
    ///             write items from array,
    ///             read items to array and some other actions
    //
    //
    //
    template<typename T, size_t Size, typename S = size_t>
    class ring_buffer
    {
    public:
        ring_buffer() : Count(0), First(0), Last(0) { }

        //----------------------------------------------------------------
        //
        //    Data transfer functions
        //
        bool write(const T* data, const S cnt);
        void read(T* const data, const S cnt);

        bool push_back(const T item);
        bool push_front(const T item);

        T pop_front();
        T pop_back();

        bool push(const T item) { return push_back(item); }
        T pop() { return pop_front(); }

        //----------------------------------------------------------------
        //
        //    Service functions
        //
        S get_count() const { return Count; }
        S get_free_size() const { return Size - Count; }
        T& operator[](const S index);
        void flush() { Count = 0; Last = First; }

    private:
        //--------------------------------------------------------------
        //  DESCRIPTON: For internal purposes
        //              Use this functions with care: it don't perform
        //              free size and count check
        //
        void push_item(const T item);
        void push_item_front(const T item);
        T pop_item();
        T pop_item_back();
        //--------------------------------------------------------------

    private:
        S  Count;
        S  First;
        S  Last;
        T  Buf[Size];
    };
    //------------------------------------------------------------------
}
//---------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
//    The ring buffer function-member definitions
//
//
//
template<typename T, size_t Size, typename S>
bool usr::ring_buffer<T, Size, S>::write(const T* data, const S cnt)
{
    if( cnt > (Size - Count) )
        return false;

    for(S i = 0; i < cnt; i++)
        push_item(*(data++));

    return true;
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
void usr::ring_buffer<T, Size, S>::read(T* data, const S cnt)
{
    S nItems = cnt <= Count ? cnt : Count;

    for(S i = 0; i < nItems; i++)
        data[i] = pop_item();
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
T& usr::ring_buffer<T, Size, S>::operator[](const S index)
{
    S x = First + index;

    if(x < Size)
        return Buf[x];
    else
        return Buf[x - Size];
}

//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
bool usr::ring_buffer<T, Size, S>::push_back(const T item)
{
    if(Count == Size)
        return false;

    push_item(item);
    return true;
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
bool usr::ring_buffer<T, Size, S>::push_front(const T item)
{
    if(Count == Size)
        return false;

    push_item_front(item);
    return true;
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_front()
{
    if(Count)
        return pop_item();
    else
        return Buf[First];
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_back()
{
    if(Count)
        return pop_item_back();
    else
        return Buf[First];
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
void usr::ring_buffer<T, Size, S>::push_item(const T item)
{
    Buf[Last] = item;
    Last++;
    Count++;

    if(Last == Size)
        Last = 0;
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
void usr::ring_buffer<T, Size, S>::push_item_front(const T item)
{
    if(First == 0)
        First = Size - 1;
    else
        --First;
    Buf[First] = item;
    Count++;
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_item()
{
    T item = Buf[First];

    Count--;
    First++;
    if(First == Size)
        First = 0;

    return item;
}
//------------------------------------------------------------------------------
template<typename T, size_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_item_back()
{

    if(Last == 0)
        Last = Size - 1;
    else
        --Last;

    Count--;
    return Buf[Last];;
}
//------------------------------------------------------------------------------


#endif // USRLIB_H
