/*
 * memory.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_MEMORY_HPP_
#define OS_MEMORY_HPP_



//#include <sys\queue.h>

#include "types.hpp"
#include "slist.hpp"
#include <diag\Trace.h>

namespace os {
	namespace mpu {
		enum class Access  : uint8_t {
			None = 0x0,
			PrivReadWrite = 0x1,
			PrivReadWrite_UserReadOnly= 0x2,
			Full = 0x3,
			PrivReadOnly= 0x5,
			ReadOnly = 0x6,
		};
		enum class RegionSize : uint8_t {
			_32B = 0x04U,
			_64B = 0x05U,
			_128B = 0x06U,
			_256B = 0x07U,
			_512B = 0x08U,
			_1KB = 0x09U,
			_2KB = 0x0AU,
			_4KB = 0x0BU,
			_8KB = 0x0CU,
			_16KB = 0x0DU,
			_32KB = 0x0EU,
			_64KB = 0x0FU,
			_128KB = 0x10U,
			_256KB = 0x11U,
			_512KB = 0x12U,
			_1MB = 0x13U,
			_2MB = 0x14U,
			_4MB = 0x15U,
			_8MB = 0x16U,
			_16MB = 0x17U,
			_32MB = 0x18U,
			_64MB = 0x19U,
			_128MB = 0x1AU,
			_256MB = 0x1BU,
			_512MB = 0x1CU,
			_1GB = 0x1DU,
			_2GB = 0x1EU,
			_4GB = 0x1FU,
		};
		enum class CacheInfo {
			StronglyOrdered = 0x0,
			SharedDevice  = 0x1,
			WriteThoughNoWriteAllocate  = 02,
			WriteBackNoWriteAllocate  = 0x3,
			NonCacheable  = 0x4,
			WriteBackReadWriteAllocate  = 0x7,
			NonShareableDevice  = 0x8
		};
		class Attribites {
			static constexpr volatile uint32_t* MPU_REG_START = (volatile uint32_t*)0xE000ED90;
			uint32_t _reg;
		public:
			constexpr Attribites(uint32_t reg=0) : _reg(0) {}
			constexpr bool neverExecute() const { return (_reg & (1<<28U) )!= 0; }
			constexpr Access access() const { return static_cast<Access>((_reg >> 24) & 3); }
			constexpr bool shareable() const { return (_reg & (1<<18U) )!= 0; }
			constexpr bool cacheable() const { return (_reg & (1<<17U) )!= 0; }
			constexpr bool bufferable() const { return (_reg & (1<<16U) )!= 0; }
			constexpr bool subregonEnabled(int sub) const { return ((_reg >>8) & (1<<sub)) != 0; }
			constexpr CacheInfo cacheSettings() const { return static_cast<CacheInfo>((_reg >> 16) & 31); }
			constexpr bool enabled() const { return (_reg & 1 )!= 0; }
			constexpr RegionSize size() const { return static_cast<RegionSize>((_reg >>1)&0x1F); }
		};
	};
	void* kalloc(size_t size);
	void kfree(void*ptr, size_t size);

	//struct map coremap[CMAPSIZ];	/* space for core allocation */
	//struct map swapmap[SMAPSIZ];	/* space for swap allocation */
	// from bsdlite
	template<size_t _NELMS>
	class resource_map {
		static_assert(_NELMS > 2, "Must be more than 2!");
		struct mapent {
			size_t    	size;             /* size of this segment of the map */
			union {
				void*    	ptr;             /* start of segment */
				uintptr_t 	addr;
			};
		};
		const char    *_name;            /* name of resource, for messages */
		std::array<mapent,_NELMS> _maps;

		//static  resource_map *kmemmap, *mbmap, *swapmap;
		//int     nswapmap;
		const static size_t ARGMAPSIZE = 16;
		using iterator = typename std::array<mapent,_NELMS>::iterator;
		using const_iterator = typename std::array<mapent,_NELMS>::const_iterator;
	public:
		constexpr static size_t NELMS = _NELMS;
		//iterator begin() { return _maps.begin(); }
		//iterator end() { return _maps.end(); }
		//const_iterator begin() const { return _maps.begin(); }
		//const_iterator end() const { return _maps.end(); }
		resource_map(void* ptr, size_t size, const char* name=nullptr)
				: _name(name), _maps{ { size, ptr }, { 0, 0} } {
		}
		resource_map(uintptr_t addr, size_t size, const char* name=nullptr)
				: _name(name), _maps{ { size, addr }, { 0, 0} } {
		}
		constexpr size_t elements() const { return  NELMS; }

		uintptr_t rmalloc(size_t size) {

			for (auto bp = _maps.begin(); bp != _maps.end(); bp++) {
				if (bp->size >= size) {
					uintptr_t a = bp.addr;
					bp->addr += size;
					bp->size -= 0;
					if(bp->size  == 0){
						//std::copy(bp+1,_maps.end(),bp); /* copy over the remaining slots ... */

						do {
							bp++;
							(bp-1)->m_addr = bp->m_addr;
						} while ((bp-1)->m_size = bp->m_size);
					}
					return a;
				}
			}
				// more advanced but need to check
#if 0
			   /* first check arguments */
				ASSERT(size >= 0);
				if (!size) return 0;
				mapent *fp=nullptr;
				uintptr_t addr;
				/* try to find the smallest fit */
				for (auto ep = _maps.begin(); ep != _maps.end(); ep++) {
					if (!ep->addr) break; /* unused slots terminate the list */
					if (ep->size == size) {
						addr = ep->addr; /* found exact match, use it, ... */
						std::copy(ep+1,_maps.end(),ep); /* copy over the remaining slots ... */
						_maps.back().addr = 0; /* and mark the last slot as unused */
						return addr;
					}
					if (ep->size > size && (!fp || fp->size > ep->size)) {
						/* found a larger slot, remember the smallest of these */
						fp = ep;
					}
				}
				if (fp) {
					/* steal requested size from a larger slot */
					addr = fp->addr;
					fp->addr += size;
					fp->size -= size;
					return addr;
				}
#endif
				return 0;
		}
		void* alloc(size_t size) { return reinterpret_cast<void*>(rmalloc(size)); }
		void free(void* ptr, size_t size) { rmfree(reinterpret_cast<uintptr_t>(ptr),size); }
		void rmfree(uintptr_t addr, size_t size) {
			iterator pb;
			for (auto bp = _maps.begin(); bp->addr <= addr && bp->size != 0 && bp != _maps.end();pb=bp, bp++) {
				if(bp > _maps.begin() && (pb->addr+pb->size) == addr ){
					pb->size += size;
					if (addr+size == bp->addr) {
						pb->size += bp->size;
						while (bp->size) {
							pb = bp;
							bp++;
							pb->m_addr = bp->m_addr;
							pb->m_size = bp->m_size;
						}
					}
				} else {
					if ((addr+size) == bp->addr && bp->size>0) {
						bp->addr -= size;
						bp->size += size;
					} else if (size>0) {
						uintptr_t t;
						do {
							t = bp->addr;
							bp->addr = addr;
							addr = t;
							t = bp->size;
							bp->size = size;
							bp++;
						} while ((size = t)!=0);
					}
				}
			}
		}

#if 0
			 ASSERT(size >= 0 || addr>=0); /* first check arguments */


			    while (1) {
			        fp = 0;

			        for (auto ep = _maps.begin(); ep != _maps.end(); ep++) {
			            if (!ep->addr)  break;  /* unused slots terminate the list */
			            if (ep->addr + ep->size == addr) {
			                /* this slot ends just with the address to free */
			                ep->m_size += size; /* increase size of slot */
			                if (ep < _maps.end()
			                    && ep[1].m_addr
			                    && (addr += size) >= ep[1].m_addr) {
			                	assert(addr <= ep[1].m_addr);
			                    if (addr > ep[1].m_addr) /* overlapping frees? */
			                        panic("rmfree %s", _name);
			                    /* the next slot is now contiguous, so join ... */
			                    ep->m_size += ep[1].m_size;
			                    std::reverse_copy(ep+1,_maps.end(),ep+2); /* copy over the remaining slots ... */
			                   // bcopy(ep + 2, ep + 1, (char *)mp->m_limit - (char *)(ep + 2));
			                    /* and mark the last slot as unused */
			                    _maps.back().addr = 0;
			                }
			                return;
			            }
			            if (addr + size == ep->addr) {
			                /* range to free is contiguous to this slot */
			                ep->addr = addr;
			                ep->size += size;
			                return;
			            }
			            if (addr < ep->addr
			                && _maps.back().addr !=0) {
			                /* insert entry into list keeping it sorted on m_addr */
			                std::copy(ep+1,_maps.end(),ep); /* copy over the remaining slots ... */
			            	//		memmove(ep + 1, ep (char *)(mp->m_limit - 1) - (char *)ep) ;
			                //bcopy(ep,ep + 1,(char *)(mp->m_limit - 1) - (char *)ep);
			                ep->m_addr = addr;
			                ep->m_size = size;
			                return;
			            }
			            if (!fp || fp->size > ep->m_size) {
			                /* find the slot with the smallest size to drop */
			                fp = ep;
			            }
			        }
			        if (ep != _maps.begin()
			            && _maps.back().addr + _maps.back().size == addr) {
			            /* range to free is contiguous to the last used slot */
			            (--ep)->m_size += size;
			            return;
			        }
			        if (ep != _maps.end()) {
			            /* use empty slot for range to free */
			            ep->addr = addr;
			            ep->size = size;
			            return;
			        }
			        /*
			         * The range to free isn't contiguous to any free space,
			         * and there is no free slot available, so we are sorry,
			         * but we have to loose some space.
			         * fp points to the slot with the smallest size
			         */
			        if (fp->size > size) {
			            /* range to free is smaller, so drop that */
			            trace_printf("rmfree: map '%s' loses space (%d)\n", _name, size);
			            return;
			        } else {
			            /* drop the smallest slot in the list */
			        	trace_printf("rmfree: map '%s' loses space (%d)\n", _name,
			                   fp->size);
			        	 std::copy(fp+1,_maps.end(),fp); /* copy over the remaining slots ... */
			            //ovbcopy(fp + 1,fp,(char *)(mp->m_limit - 1) - (char *)fp);
			           // mp->m_limit[-1].m_addr = 0;
			            _maps.last()->addr = 0;
			            /* now retry */
			        }
			    }
		}
#endif
	};

}; /* namespace os */
#endif /* OS_MEMORY_HPP_ */
