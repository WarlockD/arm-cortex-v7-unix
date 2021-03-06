/*
 * memory.cpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#include <os/memory.hpp>
#include <os/atomic.hpp>
#include <os/conf.hpp>
#include "bitmap.hpp"
#include <cstdint>
#include <cstddef>
#include <cassert>
// memory.c stuff
#include <cstdlib>
#include <cstring>
#include <cerrno>
#undef errrno
extern int errno;

static volatile uintptr_t memory_start=0;
static volatile uintptr_t memory_end=0;
static volatile uintptr_t memory_size=0;
static volatile uintptr_t memory_page_count=0;
static volatile uintptr_t memory_heap_start=0;
static volatile uintptr_t memory_heap_end=0;
static volatile  bool memory_inited = false;

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t MAX_MEMOREY = 16 * 1024 * 1024; // 16 megabyte support for right now

struct bitops::bitmap_t<MAX_MEMOREY/PAGE_SIZE> page_bitmap;
extern "C" void kmemory_init(){
	if(memory_inited) return;
	memory_inited = true;
	extern char __end __asm("_end");
	memory_start =  (reinterpret_cast<uintptr_t>(&__end)+(PAGE_SIZE-1)) & ~PAGE_SIZE;
	memory_end = os::TRUE_SRAM_END;
	memory_size=memory_end-memory_start;
	memory_page_count=memory_size/PAGE_SIZE;
	memory_heap_end=memory_heap_start=memory_start;
	// and just ot make sure the math works up right
	//ASSERT((memory_page_count*PAGE_SIZE)==memory_size);
	trace_printf("kmemory start=%p end=%p size=%d pages=%d\r\n",memory_start,memory_end, memory_end-memory_start,memory_page_count);
//	uint32_t i;
	//for(i=0;i<memory_page_count&&i < page_bitmap.size();i++)
	//	page_bitmap[i].clear();
	//for(;i < page_bitmap.size(); i++)
	//	page_bitmap[i].set();
}

namespace os{

		// so kalloc reserves stuff by page so we need to be sure size == page
	void* kalloc(size_t size){
		ASSERT((size % PAGE_SIZE)==0);
		return nullptr;
	}
	void kfree(void*ptr, size_t size){

	}
};
// hack for right now
void * operator new(size_t size)
{       // try to allocate size bytes
   void *p;
   while ((p = malloc(size)) == 0);
   trace_printf("new called! ptr=%p size =%d\n",p,size);
   return (p);
}
#if 0
extern "C" caddr_t _sbrk(int incr)
{
	if(!memory_inited) kmemory_init();
	uintptr_t prev_heap_end = memory_heap_end;

	if ((prev_heap_end + incr) > memory_end)
	{
		trace_printf("Heap and stack collision\n");
		errno = ENOMEM;
		return (caddr_t) -1;
	} else {
		memory_heap_end += incr;
		trace_printf("_sbrk: heap increased=%d heap_end=%p\n", incr, memory_heap_end);
	}

	return reinterpret_cast<caddr_t>(prev_heap_end);
}
#endif

