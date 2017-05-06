/*
 * buf.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_BUF_HPP_
#define OS_BUF_HPP_

#include "types.hpp"

namespace os {
// buffer interface
	// container for data
enum class seekdir {
	begin,
	end,
	current
};
	struct file_operations {
		virtual bool open(int flags) { return false; };
		virtual bool isopen() const = 0;
		virtual void close() = 0;
		virtual bool flush() = 0;
		virtual int write(const uint8_t* data, const size_t count) = 0;
		virtual int read(uint8_t* data, const size_t count) = 0;
		virtual size_t seek(int offset, seekdir dir = seekdir::begin)=0;
		virtual size_t tell() const = 0; // position
		virtual size_t size() const = 0; // filesize
		virtual int get()=0;
		virtual int put(int c)=0;
		virtual ~file_operations(){}
	};

	class uio {
	protected:
		uint8_t* _data;  /* Base address. */
		size_t _size;   /* Length. */
	public:
		uio(uint8_t * data, size_t size) : _data(data), _size(size) {}
		uio(const uio& u, size_t offset) : _data(u.data()+offset), _size(u.size()-offset) { assert(u.size()>offset); }
		inline uint8_t* data() const { return _data; }
		inline size_t size() const { return _size; }
		bool operator==(const uio& r) const { return _data == r._data; }
		bool operator!=(const uio& r) const { return _data != r._data; }

		template<typename T=uint8_t>
		T& operator[](size_t i) {
			assert(i < (sizeof(T) * size()));
			return *(reinterpret_cast<T*>(data()) + i);
		}
		template<typename T=uint8_t>
		const T& operator[](size_t i) const {
			assert(i < (sizeof(T) * size()));
			return *(reinterpret_cast<const T*>(data()) + i);
		}
		template<typename T=uint8_t>
		T* begin() { return reinterpret_cast<T*>(data());  }
		template<typename T=uint8_t>
		T* end()  { return reinterpret_cast<T*>(data()) + size(); }
	};

	 class uio_file : public file_operations {
	protected:
		uio  _data;
		size_t _offset;
	public:
		uio_file(uint8_t * data, size_t size) : _data(data,size), _offset(0) {}
		uio_file(const uio& u) : _data(u), _offset(0) {}
		uint8_t* data() const { return _data.data(); }
		inline size_t size() const override final { return _data.size(); }
		bool open(int flags) override final { (void)flags; return false; };
		bool isopen() const override final { return true; }
		void close() override final {}
		bool flush() override final { return true; }
		int write(const uint8_t* data, const size_t count) override final {
			size_t rem = std::min(size() - _offset, count);
			uint8_t* ptr = _data.begin()+_offset;
			std::copy(data, data+ rem, ptr);
			_offset+=rem;
			return rem;
		}
		int read(uint8_t* data, const size_t count)  override final {
			size_t rem = std::min(size() - _offset, count);
			const uint8_t* ptr = _data.begin()+_offset;
			std::copy(ptr,ptr+ rem,data);
			_offset+=rem;
			return rem;
		}
		size_t tell() const  override final  { return _offset; } // position
		size_t seek(int offset, seekdir dir = seekdir::begin)  override final {
			switch(dir) {
			case seekdir::begin:
				_offset = std::min(static_cast<size_t>(to_unsigned(offset)),size());
				break;
			case seekdir::current:
				_offset = std::min(static_cast<size_t>(_offset + offset),size());
				break;
			case seekdir::end: // can't remember if its sub or +
				_offset = std::min(static_cast<size_t>(size() + offset),size());
				break;
			};
			return _offset;
		}
		int get()  override final{
			if(_offset>=size()) return -1;
			return _data[_offset++];
		}
		int put(int c)  override final{
			if(_offset>=size()) return -1;
			return _data[_offset++] =c;
		}
	};


	class uio_fifo : public file_operations {
	protected:
		uio  _data;
		size_t _count;
		size_t _first;
		size_t _last;
	public:
		uio_fifo(uint8_t * data, size_t size) : _data(data,size), _count(0),_first(0), _last(0)  {}
		uio_fifo(const uio& u) : _data(u), _count(0),_first(0), _last(0)  {}
		uint8_t* data() const { return _data.data(); }
		inline size_t size() const override final { return _data.size(); }
		bool open(int flags) override final { (void)flags; return false; };
		bool isopen() const override final { return true; }
		void close() override final {}
		bool flush() override final { return true; }


		void clear() { _count = 0; _last = _first; };
		size_t used() const { return  _count; }
		size_t tell() const  override final  { return _count; } // used
		size_t free_size() const  { return size() - _count; }
		size_t seek(int offset, seekdir dir = seekdir::begin)  override final { return -1; } // dosn't work
		int write(const uint8_t* data, const size_t count) override final {
			for(size_t i=0;i < count; i++){
				if(_count >= size()) return i;
				put(data[i]);
			}
			return (int)count;
		}
		int read(uint8_t* data, const size_t count) override final {
			for(size_t i=0;i < count; i++){
				if(_count == 0) return i;
				_data[i] = get();
			}
			return (int)count;
		}
		int get() override final {
			if(_count==0) return -1;
		    uint8_t c = _data[_first++];
		    if(_first == size()) _first = 0;
		    --_count;
		    return c;
		}
		int put(int c) override final {
			if(_count==size()) return -1;
			_data[_last++] = c;
		    if(_last == size())  _last = 0;
		    ++_count;
		    return c;
		}
	};

#if 0
	// https://github.com/fmtlib/fmt/blob/master/fmt/format.h
	// partialy yanked from there
	class ostream {
		buf& _stream;
	public:
		ostream(buf& s) : _stream(s) {}
		ostream& operator<<(int value);
		ostream& operator<<(int value);
		void putch(char ch);
			void puts(const char * s);
			void put_hex(uint8_t b);
			void put_hex(uint16_t w) __attribute__((__noinline__))
			{
				put_hex((uint8_t)(w >> 8));
				put_hex((uint8_t)w);
			}
			void put_hex(uint32_t w) __attribute__((__noinline__))
			{
				put_hex((uint16_t)(w >> 16));
				put_hex((uint16_t)w);
			}
			void put_hex(int i) { put_hex((uint32_t)i); }
			dbg_uart_t& operator<< (char value) { putch(value); return *this; }
			dbg_uart_t& operator<< (const char* value)  { puts(value); return *this; }
		//	dbg_uart_t& operator<< (int value)  { put_hex(value); return *this; }
		//	dbg_uart_t& operator<< (uint16_t value)  { put_hex(value); return *this; }
		//	dbg_uart_t& operator<< (uint32_t value)  { put_hex(value); return *this; }
			dbg_uart_t& operator<< (int value)
			{
				char buf[20];
				puts(ftoa(value, buf, 0));
				return *this;
			}
			dbg_uart_t& operator<< (uint16_t value)
			{
				char buf[20];
				puts(ftoa(value, buf, 0));
				return *this;
			}
			dbg_uart_t& operator<< (uint32_t value)
			{
				char buf[20];
				puts(ftoa(value, buf, 0));
				return *this;
			}
			dbg_uart_t& operator<< (double value)
			{
				char buf[20];
				puts(ftoa(value, buf, 1));
				return *this;
			}
			dbg_uart_t& operator<< (size_t value) { return operator<< ((uint32_t)value); }
	};
#endif


} /* namespace os */

#endif /* OS_BUF_HPP_ */
