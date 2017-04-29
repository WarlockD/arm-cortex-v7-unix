/*
 * buf.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: warlo
 */

#include "buf.h"
#include "bitmap.h"
#include "sem.h"
#include <string.h>

namespace {

};
//#define BSIZE 512 /* buffer size */
//#define BCOUNT 64 /* count of buffers */
namespace xv6 {


#if 0
	struct buf_hash {
		size_t operator()(device* dev, daddr_t blockno) const { return blockno ^ (long)dev; }
	};
	struct buf_equals {
		bool operator()(const buf& b, device* dev, daddr_t blockno) const { return b.blkno() == blockno && b.dev() == dev; }
	};

	using buf_cache_t = static_cache<buf, buf_equals, buf_hash, BCOUNT>;
	uint8_t buffer[BCOUNT][BSIZE];
	static buf_cache_t buf_cache;
	void buf::binit(){
		for(size_t i=0; i < BCOUNT;i++) {
			buf_cache[i]._addr = &buffer[i];
		}

	}
	buf* buf::getblk(device* dev, daddr_t blkno){
		buf*  bp = buf_cache.aquire(dev,blkno);
		if(bp->_flags == B_DELWRI) {
			bp->_flags |= B_ASYNC;
			bp->write();
		}
		return bp;
	}
	 buf* buf::bread(device* dev, daddr_t blkno){
		 buf* bp = getblk(dev, blkno);
		 if(bp->_flags == B_DONE) return bp;
		 bp->_flags |= B_READ;
		 bp->_size = BSIZE;
		 dev->strategy(bp);
		 bp->iowait();
		 return bp;
	}
	 buf* buf::breada(device* dev, daddr_t blkno,daddr_t rablkno){
		 assert(0);
		return nullptr;
	}
	 buf* buf::geteblk(){
		return buf_cache.aquire(((device*)nullptr),0);
	}
	bool buf::incore(device* dev, daddr_t blkno){
		return false;
	}
	void buf::write(){
		auto flag = _flags;
		_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI | B_AGE);
		_dev->strategy(this);
		if(flag != B_ASYNC) {
			iowait();
			release();
		}else if(flag == B_DELWRI)
			_flags |= B_AGE;
	}
	void buf::dwrite(){

	}
	void buf::awrite(){
		_flags |= B_ASYNC;
		write();
	}
	void buf::release(){
		buf_cache.lock(this);
		_flags &= ~(B_WANTED|B_BUSY|B_ASYNC|B_AGE);
		buf_cache.unlock(this);
		buf_cache.release(this);
	}
	void buf::iowait(){
		while (_flags != B_DONE);
			//sleep((caddr_t)bp, PRIBIO);
	}
	void buf::iodone(){
		_flags |= B_DONE;
		if(_flags ==B_ASYNC) release();
		else buf_cache.unlock(this);
	}
	void buf::clear(){
		memset(_addr,0,_size);
	}
	void buf::flush(device* dev){

	}
	void buf::notavail(){

	}
#if 0
	struct buf_t {
		uint8_t data[disk_buf::BSIZE];
		dev_t dev;
		daddr_t sector;
		flag<buf::B> flags;
		buf_t(dev_t dev, daddr_t sector) : dev(dev), sector(sector), flags(buf::B::BUSY) {}
		buf_t(): dev(0), sector(0), flags(buf::B::FREE) {}
	};
	constexpr static buf_t* bufcast(void* ptr) { return static_cast<buf_t*>(ptr); }
	//constexpr static const buf_t* bufcast(const void* ptr) { return static_cast<const buf_t*>(ptr); }
	struct buf_hash {
		std::size_t operator()(dev_t dev, daddr_t sector) const {
			return dev<<16 ^ sector;
		}
	};
	struct buf_equals {
		bool operator()(const buf_t& b, dev_t dev, daddr_t sector) const {
			return dev == b.dev && sector == b.sector;
		}
	};
	using buf_cache_t = static_cache<buf_t,buf_equals, buf_hash, disk_buf::CACHE_SIZE>;
	static buf_cache_t buf_cache;


	disk_buf::disk_buf(dev_t dev, daddr_t sector) : buf(buf_cache.aquire(dev,sector),BSIZE){ }
	disk_buf::~disk_buf(){
		buf_cache.release(bufcast(_data));
	}
	dev_t disk_buf::dev() const { return bufcast(_data)->dev; }
	daddr_t disk_buf::sector() const { return bufcast(_data)->sector; }

	void disk_buf::sync() {
		buf_t* bp =  bufcast(_data);
		bdevsw* drv = bdevsw::drivers[major(bp->dev)];
		assert(drv);
		drv->strategy(*this);
		_flags.clear(B::DIRTY);
		_flags.set(B::VALID);
	}
	void* disk_buf::data()  {
		if(!_flags.contains(B::VALID)) sync();
		return _data;
	}
#endif

#endif
} /* namespace xv6 */
