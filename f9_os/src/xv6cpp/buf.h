/*
 * buf.h
 *
 *  Created on: Apr 18, 2017
 *      Author: warlo
 */

#ifndef XV6CPP_BUF_H_
#define XV6CPP_BUF_H_

#include "device.h"
#include <sys\types.h>
#include "list.h"
#include "device.h"
#include "bitmap.h"
#define TRASHY_SLEEP
namespace xv6 {
struct device;
	using buf_head = tailq_head<class buf>;
	class buf : public bitops::simple_lock {
		//static constexpr buf* NOLIST = 0x123455; // magic
		public:
			enum buf_flags : uint16_t {
				B_WRITE	=0,	/* non-read pseudo-flag */
				B_READ	=01,	/* read when I/O occurs */
				B_DONE	=02,	/* transaction finished */
				B_ERROR	=04,	/* transaction aborted */
				B_BUSY	=010,	/* not on av_forw/back list */
				B_PHYS	=020,	/* Physical IO potentially using UNIBUS map */
				B_MAP	=040,	/* This block has the UNIBUS map allocated */
				B_WANTED =0100,	/* issue wakeup when BUSY goes off */
				B_AGE	=0200,	/* delayed write for correct aging */
				B_ASYNC	=0400,	/* don't wait for I/O completion */
				B_DELWRI =01000,	/* don't write till block leaves available list */
				B_TAPE =02000,	/* this is a magtape (no bdwrite) */
				B_PBUSY	=04000,
				B_PACK	=010000,
			};


			void write();
			void dwrite(); // delay write
			void awrite(); // async write
			void release(); // release buffer
			void iowait();
			void iodone();
			void clear(); // clear
			void flush(device* dev);

	#if 0
			buf(const buf& b) = delete;
			buf(buf&& b) = delete;
			buf(const buf& b) = delete;
			buf(buf&& b) = delete;
	#endif
			daddr_t blkno() const { return _blkno; }
			device* dev() const { return _dev; }
			void* addr() const { return _addr; }
			size_t size() const { return _size; }
			buf() : simple_lock(), _dev(nullptr), _addr(nullptr), _size(0), _blkno(-1), _flags(B_READ) {}
			buf(device* dev, void* data, size_t size, daddr_t blkno) : simple_lock(),
								_dev(dev), _addr(data), _size(size), _blkno(blkno), _flags(B_READ) {
			//	if(dev) TAILQ_INSERT_HEAD(&dev->btab, this,_link);
			}
			~buf() { //if(_dev)
				//TAILQ_REMOVE(&_dev->btab, this,_link);
			}
			// because of the link we have to make sure we cannot copy
			buf(const buf& copy) = delete;
			buf& operator=(const buf& copy) = delete;
			// move is ok though
			buf(buf&& move) = default;
			buf& operator=(buf&& move) = default;
			bool incore() const { return _dev != nullptr; }
			void link(device* dev, daddr_t blkno){
				if(dev!= _dev){
			//		if(_dev) TAILQ_REMOVE(&_dev->btab, this,_link);
			//		if(dev) TAILQ_INSERT_HEAD(&dev->btab, this,_link);
				}
				_dev = dev;
				_blkno = blkno;
			}
			static inline size_t hash(const device* dev, daddr_t blockno) { return blockno ^ (long)dev; }
			static inline size_t hash(const buf& b) { return hash(b.dev(), b.blkno()); }
			inline bool equals(const device* dev, daddr_t blkno) const { return dev == _dev && _blkno == blkno; }
			inline bool equals(const buf& b) const { return equals(b._dev,b._blkno); }
		protected:
			void notavail() {
			//	lock();
			//	TAILQ_REMOVE(this,_link);
			}
			device* _dev;
			void* _addr;
			size_t _size;
			daddr_t _blkno;
			enum_helper<buf_flags> _flags;
			buf *b_forw;		/* headed by d_tab of conf.c */
			buf *b_back;		/*  "  */
			buf *av_forw;		/* position on free list, */
			buf *av_back;		/*     if not BUSY*/

			tailq_entry<buf> _link; // for device
			tailq_entry<buf> _free; // free list if not busy
			friend struct device;
		};

	template<size_t _BSIZE>
	class static_buf : public buf {
	public:
		constexpr static size_t BSIZE = _BSIZE;
		static_buf() : buf(nullptr,&_data,BSIZE,0) {}
		static_buf(device* dev,daddr_t blkno) : buf(dev,&_data,BSIZE,blkno) {}
		// because of the link we have to make sure we cannot copy
		static_buf(const static_buf& copy) = delete;
		static_buf& operator=(const static_buf& copy) = delete;
		// move is ok though
		static_buf(static_buf&& move) = default;
		static_buf& operator=(static_buf&& move) = default;
	protected:
		uint8_t _data[BSIZE];
	};
#if 0
	template<size_t _BSIZE, size_t _CAHCESIZE>
	class buf_cache {
	public:
		constexpr static size_t BSIZE = _BSIZE;
		constexpr static size_t NBUF = _CAHCESIZE;

		//buf* getblk(xv6::device* d, daddr_t blkno);
		buf* bread(device* dev, daddr_t blkno);
		buf* breada(device* dev, daddr_t blkno,daddr_t rablkno);

		buf* getblk (device* dev, daddr_t blkno)
		{
			if(dev == nullptr)
				return geteblk();
			else {
				buf* bp = dev->incore(blkno);
				if((bp = dev->incore(blkno))) {
					bp->lock();
					return bp;
				}
				bp = geteblk();
				bp->link(dev,blkno); // link it to the dev
				bp->lock();
				return bp;
			}
		}
		buf* geteblk(){ // get an empty block
			bitmap_cursor_t f;
			uint32_t i;
			buf* bp;
			do {
				_freelist_lock.lock();
				bp = TAILQ_FIRST(&_freelist);
				if(bp == nullptr) {
					// first time?  alloc a buffer from the _buffs.
					bp = _bufs.create();
					if(bp == nullptr) {
						_freelist_lock.unlock();
						_freelist_lock.wait();
						continue; // in this case we have to wait for somone to release something
					}
				} else
					TAILQ_REMOVE(&_freelist, bp, _link);
				_freelist_lock.unlock();
				bp->lock();
			} while(0);
			return bp;
		}
	protected:

		bitmap_table_t<static_buf<BSIZE>,NBUF> _bufs;
	//	bitmap_t<NBUF> _free;
		//static_buf<BSIZE> _bufs[NBUF];
		simple_lock _freelist_lock;
		tailq_head<buf> _freelist;
	};
#endif
#if 0
	};
	template<size_t SIZE>
	class fixed_buf : public buf {
	public:
		static constexpr size_t BSIZE = SIZE;
		fixed_buf() : buf(_fixed, BSIZE) {}
	protected:
		uint8_t _fixed[BSIZE];
	};

	template<typename T, size_t S = sizeof(T)>
	class cast_buf {
	public:
		using type = T;
		using const_type = const T;
		using pointer = type*;
		using const_pointer = const_type*;
		using refrence = type&;
		using const_refrence = const_type&;
		static constexpr size_t type_size = S;
		cast_buf(buf& b) : _buf(b) ,_elements(type_size) {}
		refrence operator[](size_t i) { return *get_elm(i); }
		const_refrence operator[](size_t i) const { return *get_elm(i); }
		size_t size() const { return _elements; }
		pointer begin() { return get_elm(0); }
		pointer end() { return get_elm(_elements); }
		const_pointer begin() const { return get_elm(0); }
		const_pointer end() const { return get_elm(_elements); }
	protected:
		pointer get_elm(size_t i) const {
			if(i >= _elements) return nullptr;
			return static_cast<pointer>(_buf.data() + type_size*i);
		}
		size_t _elements;
		buf& _buf;

	};



	template<size_t BUFFER_SIZE, size_t STATIC_CACHE_SIZE>
	class static_buf_cache  {
	public:
		constexpr static size_t BSIZE = BUFFER_SIZE;
		constexpr static size_t BUCKETS_SHIFT = 2;
		constexpr static size_t CACHE_SIZE = STATIC_CACHE_SIZE;
		constexpr static size_t BUCKET_SIZE = STATIC_CACHE_SIZE >> BUCKETS_SHIFT;
		class cached_buf;
		class cached_buf : public fixed_buf<BUFFER_SIZE>  {
			cached_buf() : _dev(nullptr), _blockno(-1), _age(0), _ref(0) {}
			daddr_t  blockno() const { return  _blockno; }
			// we take and own this buffer, its removed from the freelist and put on the list aquire has
			// this prevents it from being reused
			virtual void sync() {
				if(_dev && _blockno>=0){
				switch(this->_state) {
					case buf::B::EMPTY:
						assert(_dev->read(this->data(),this->size() * this->blockno(),this->size())==this->size());
						break;
					break;
					case buf::B::VALID:
					break;
					case buf::B::DIRTY:
						assert(_dev->write(this->data(),this->size() * this->blockno(),this->size())==this->size());
						break;
				}
				this->_state = buf::B::VALID;
				}

			}
		protected:
			tailq_entry<cached_buf> free;
			list_entry<cached_buf> hash;
			device 	*_dev;
			daddr_t  _blockno;
			time_t _age;
			int _ref;

			friend static_buf_cache;
		};
		cached_buf _nodes[STATIC_CACHE_SIZE];
		list_head<cached_buf>  _buckets[BUCKET_SIZE];
		list_head<cached_buf> _freelist; // intialy hash list, if key is invalidated its here

		static_buf_cache() {
			for(cached_buf& b : _nodes){
				TAILQ_INSERT_HEAD(&_freelist,&b,free);
				b._ref = 0;
			}
		}
		cached_buf* bread(device&dev, baddr_t blockno) {
			assert(blockno>=0);
			cached_buf* c = nullptr;
			size_t hash = dev.dev() << 8 ^ blockno >> 4;
			list_head<cached_buf>& bucket = _buckets[hash % BUCKET_SIZE];
			LIST_FOREACH(c, &bucket,hash) {
				if(c->blockno() == blockno) {
					c->_ref++;
					c->sync();
					return c;
				}
			}
			c =_alloc();
			c->_blockno = blockno;
			c->_dev = &dev;
			LIST_REMOVE(c,hash);				   // removed from hashlist too
			LIST_INSERT_HEAD(&bucket,c,hash);	   // but back in the right bucket
			c->sync(); // sync it valid
			return c;
		}
		void bwrite(cached_buf* bp) {
			bp->invalidate(();
			bp->sync();
		}
		void brelse(cached_buf* b){
			if(--b->_ref == 0){
				TAILQ_REMOVE(&_freelist, b, free);
				TAILQ_INSERT_TAIL(&_freelist,b, free); // move to end of the free list
			}
		}
		void free(cached_buf* b) {
			b->lock(); // lock it for free

			b->unlock();
		}
		private:
		// retreve new buffer from top of free list, then shove in in the back
		// all the time it being locked
		cached_buf* _alloc() {
			cached_buf* b,*n;
#ifdef TRASHY_SLEEP
			for(;;)
#endif
			TAILQ_FOREACH_SAFE(b, &_freelist,free,n) {
				if(++b->_ref == 1) return b;
				--b->ref;
			}
			return nullptr;
		}

	};
#endif
} /* namespace xv6 */

#endif /* XV6CPP_BUF_H_ */
