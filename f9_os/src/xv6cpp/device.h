/*
 * device.h
 *
 *  Created on: Apr 19, 2017
 *      Author: warlo
 */

#ifndef XV6CPP_DEVICE_H_
#define XV6CPP_DEVICE_H_

#include "os.h"
#include "list.h"

namespace xv6 {
// basic device,
	using baddr_t = size_t; // block number
	enum class DeviceStatus {
		Completed = 0,
		Working,
		Error
	};
	class buf;
	class bref;
	struct device {
		static constexpr device* NODEV = (device* )nullptr;
		virtual dev_t dev() const = 0;
		virtual DeviceStatus read(void* data, size_t size, daddr_t offset)=0;
		virtual DeviceStatus write(const void* data, size_t size, daddr_t offset)=0;
		virtual void strategy(buf* bp)=0;
		tailq_head<buf> btab;
		buf* incore(daddr_t blkno);
		virtual ~device() {}
	};
	template<size_t _BSIZE>
	struct block_device : public device {
			static constexpr size_t BSIZE = _BSIZE;
			virtual DeviceStatus readb(void* data, baddr_t blockno){
				return read(data,BSIZE,static_cast<daddr_t>(blockno*BSIZE));
			}
			virtual DeviceStatus writeb(const void* data, baddr_t blockno){
				return write(data,BSIZE,static_cast<daddr_t>(blockno*BSIZE));
			}
			//virtual void strategy(buf& b); // defined in buf.c
			virtual ~block_device() {}
		//	buf_head cache; // cached blocks, like super block or such
	};

	static constexpr dev_t major(dev_t a) { return (a >> 8 & 0xFF); }
	static constexpr dev_t minor(dev_t a) { return (a& 0xFF); }
	static constexpr dev_t mkdev(dev_t major, dev_t minor) { return (major&0xFF) << 8 | (minor&0xFF); }


} /* namespace v6 */

#endif /* XV6CPP_DEVICE_H_ */
