/*
 * device.cpp
 *
 *  Created on: Apr 19, 2017
 *      Author: warlo
 */

#include "device.h"
#include "buf.h"

namespace xv6 {
	buf* device::incore(daddr_t blkno){
		buf* b;
		TAILQ_FOREACH(b,&btab,_link) if(b->blkno() == blkno) return b;
		return nullptr;
	}

} /* namespace v6 */
