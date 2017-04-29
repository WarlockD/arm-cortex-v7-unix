/*
 * buf.h
 *
 *  Created on: Apr 10, 2017
 *      Author: warlo
 */

#ifndef V6MINI_BUF_H_
#define V6MINI_BUF_H_
#include "parm.hpp"

namespace v6 {
	enum buf_flags_t : uint8_t {
		/*
		 * These flags are kept in b_flags.
		 */
		B_WRITE		= 0,	    /* non-read pseudo-flag */
		B_READ		= BIT(0),	/* read when I/O occurs */
		B_DONE		= BIT(1),	/* transaction finished */
		B_ERROR		= BIT(2),	/* transaction aborted */
		B_BUSY		= BIT(3),	/* not on av_forw/back list */
		B_WANTED 	= BIT(4),	/* issue wakeup when BUSY goes off */
		B_RELOC		= BIT(5),	/* no longer used */
		B_ASYNC		= BIT(6),	/* don't wait for I/O completion */
		B_DELWRI 	= BIT(7),	/* don't write till block leaves available list */

	};
	template<typename T>
	class buf_cast {
		using type = T;
		using poniter = type*;
		caddr_t _data;
	public:
		buf_cast(caddr_t data) :  _data(data) {}
		operator T&() { return static_cast<poniter>(_data); }
		operator const T&() const { return static_cast<poniter>(_data); }
	};
	template<typename T>
	class buf_array_cast {
		using type = T;
		using poniter = type*;
		size_t 	_size;
		caddr_t _data;
	public:
		buf_array_cast(caddr_t data, size_t data_size) : _size(data_size/sizeof(T)), _data(data) {
			V6_ASSERT((sizeof(T) % data_size) == 0);
		}
		buf_cast<T>& operator[](size_t i) { return buf_cast<T>(_data + i*sizeof(T)); }
		const buf_cast<T>& operator[](size_t i) const { return buf_cast<T>(_data + i*sizeof(T)); }
		const size_t size() const { return _size; }
	};
	ENUM_FLAG_HELPER(buf_flags_t)

class block_buf {
public:
	typedef void (*block_buf_strat)(block_buf&);
	//static void init(); // must be run to set up buffers
	// shouldn't need to run binit as the internal constructor does it
	static void install_dev(dev_t dev, block_buf_strat func);
	static constexpr size_t BKSIZE = 512;
	static constexpr size_t BUFCONT = 64; 		 	// number of BKSIZE buffers out there
	static constexpr size_t BUFSTRATCOUNT = 10; 	// count of strat function calls
public: // interface
	block_buf(dev_t dev, daddr_t blockno); // load a buffer at blockno
	virtual ~block_buf();
	size_t  size() const { return _buf->size; }
	daddr_t blkno() const { return _buf->blkno; }
	const caddr_t data() const { return _buf->data; }
	caddr_t data()  { return _buf->data; }
	dev_t  dev() const { return _buf->dev; }
	size_t  tcount() const { return _buf->tcount; }
	void flush();
	template<typename T> T& operator[](size_t i) { return static_cast<T*>(data() + i*sizeof(T)); }
protected:
	struct _ibuf {
		buf_flags_t 	 flags;	// buffer flags
		daddr_t			 blkno;// block number
		caddr_t			 data;	// buffer address
		size_t			 size;	// size of buffer
		dev_t			 dev;	// device name
		size_t			 tcount; // trasfer count
		TAILQ_ENTRY(buf) list;
		TAILQ_ENTRY(buf) hash; // used on free list or lookup on not busy
	};
	friend class _internal_buf;
	_ibuf* _buf;
	block_buf(_ibuf* b); // special cas for static_buf

};
	// used for static buffers, defiend in dirver files and hte like
class static_buf : public block_buf {
	struct _ibuf _mbuf;
public:
	static_buf(caddr_t addr, size_t size) :  block_buf(&_mbuf), _mbuf{ } {}
	static_buf(const static_buf& copy) : block_buf(&_mbuf),_mbuf(copy._mbuf) {  }
	static_buf(static_buf&& move) : block_buf(&_mbuf),_mbuf(move._mbuf) {  }
	static_buf& operator=(const static_buf& copy) { _mbuf = copy._mbuf; return *this; }
	static_buf& operator=(static_buf&& move) { _mbuf = move._mbuf; return *this; }
};
} /* namespace v6 */

#endif /* V6MINI_BUF_H_ */
