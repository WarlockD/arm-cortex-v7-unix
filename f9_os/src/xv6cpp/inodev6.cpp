/*
 * inodev6.cpp
 *
 *  Created on: Apr 19, 2017
 *      Author: Paul
 */

#include "inodev6.h"
#include "list.h"

#include <string.h>

#define ICOUNT 64 /* count of inode cache */

/* inumber to disk address */
#define	itod(x)	(daddr_t)((((unsigned)x+15)>>3))
/* inumber to disk offset */
#define	itoo(x)	(int)((x+15)&07)

namespace xv6 {
struct inode_v6_hash {
	size_t operator()(device* dev, ino_t inum) const { return inum ^ (long)dev; }
};
struct inode_v6_equals {
	bool operator()(const inode_v6& i, device* dev, ino_t inum) const { return i.inum() == inum && i.dev() == dev; }
};


#if 0
	using buffer_cache_t = static_buf_cache<uint32_t, 512, 50> ;
	static buffer_cache_t block_cache;

	struct baddr_t itob(ino_t i) { return (i + 31) / 32; }
	struct off_t itoo(ino_t i) { return (i + 31) % 32; }


	off_t inode_v6::read(void* data, off_t offset, size_t size) {

		return size;
	}
	off_t inode_v6::write(void* data, off_t offset, size_t size) {

		return size;
	}
   inode_v6::inode_v6(device& dev, ino_t inum) : inode(dev,inum) {
	   // don't even need to read the super block
	   dev.read(&_node,)
   }
#endif

} /* namespace v6 */
