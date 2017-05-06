/*
 * driver.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_DRIVER_HPP_
#define OS_DRIVER_HPP_

#include "types.hpp"
#include "hash.hpp"
#include <scm\scmRTOS.h>

namespace os {
	enum class open_mode {
		read = (1<<0),
		write = (1<<1),
		binary = (1<<2),
		async = (1<<3),
	};
	struct device_driver {
		virtual bool open(open_mode mode,void* args) = 0;
		virtual bool isopen() const = 0;
		virtual bool close()  = 0;
		virtual size_t read(void *dst, size_t bytes) = 0;
		virtual size_t write(const void *src, size_t bytes) = 0;
		virtual size_t ioctrl(int flags, int mode, void* args) = 0;
		virtual ~device_driver() {}
	};
};

#endif /* OS_DRIVER_HPP_ */
