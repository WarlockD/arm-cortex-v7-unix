/*
 * memory.hpp
 *
 *  Created on: Apr 27, 2017
 *      Author: Paul
 */

#ifndef OS_MEMORY_HPP_
#define OS_MEMORY_HPP_


#include "bitmap.hpp"
#include <sys\queue.h>

#include "types.hpp"
namespace os {
	class as_t;
	/*
	 * Each address space is associated with one or more thread.
	 * AS represented as linked list of fpages, mappings are the same.
	 */
	enum class map_action_t{ MAP, GRANT, UNMAP } ;
	enum class mempool_tag_t {
		MPT_KERNEL_TEXT,
		MPT_KERNEL_DATA,
		MPT_USER_TEXT,
		MPT_USER_DATA,
		MPT_AVAILABLE,
		MPT_DEVICES,
		MPT_UNKNOWN = -1
	} ;
	enum class mpu_state_t {
		DISABLED,
		ENABLED
	} ;

	/**
	 * Flexible page (fpage_t)
	 *
	 * as_next - next in address space chain
	 * map_next - next in mappings chain (cycle list)
	 *
	 * base - base address of fpage
	 * shift - size of fpage == 1 << shift
	 * rwx - access bits
	 * mpid - id of memory pool
	 * flags - flags
	 */
	struct fpage_t {
		static constexpr uint32_t CONFIG_MAX_FPAGES = 20;
		static constexpr uint32_t FPAGE_ALWAYS    = 0x1;     /*! Fpage is always mapped in MPU */
		static constexpr uint32_t FPAGE_CLONE     = 0x2;     /*! Fpage is mapped from other AS */
		static constexpr uint32_t FPAGE_MAPPED    = 0x4;     /*! Fpage is mapped with MAP (  unavailable in original AS) */
		fpage_t *as_next;
		fpage_t *map_next;
		fpage_t *mpu_next;
		union {
			struct {
				uint32_t base;
				uint32_t mpid : 6;
				uint32_t flags : 6;
				uint32_t shift : 16;
				uint32_t rwx : 4;
			} fpage;
			uint32_t raw[2];
		};
#if 0
		template<typename FT, typename FN>
		void remove_fpage_from_list(as_t* as, fpage_t* fpage, FT _first, FN _next) {
			auto& first = std::mem_fn(_first);
			auto& next = std::mem_fn(_next);
			fpage_t *fpprev = first(as);
			int end;
			if(fpage != fpprev){
				first(as) = next(fpprev);
			} else {
				while(!end && next(fpprev) != fpage) {
					if(next(fpprev) == nullptr) end = 1;
					fpprev = next(fpprev);
				}
				next(fpprev) = next(fpage);
			}
		}
#endif
		inline uint32_t base() const { return fpage.base; }
		inline uint32_t size() const { return 1<<fpage.shift; }
		inline uint32_t begin() const { return fpage.base; }
		inline uint32_t end() const { return base() + size(); }
		bool addr_in(memptr_t addr, int incl_end) const {
			return ((addr >= base() && addr <  end()) || (incl_end && addr ==  end()));
		}
		static void fpages_init(void);
		static  fpage_t *split_fpage(as_t *as, fpage_t *fpage, memptr_t split, int rl);

		static int assign_fpages(as_t *as, memptr_t base, size_t size);
		static int assign_fpages_ext(int mpid, as_t *as, memptr_t base, size_t size,
		                      fpage_t **pfirst, fpage_t **plast);

		static int map_fpage(as_t *src, as_t *dst, fpage_t *fpage, map_action_t action);
		static int unmap_fpage(as_t *as, fpage_t *fpage);
		static void destroy_fpage(fpage_t *fpage);
	#ifdef CONFIG_KDB
		int used;
	#endif /* CONFIG_KDB */
	};

	struct as_t{
		uint32_t as_spaceid;	/*! Space Identifier */
		fpage_t *first;	/*! head of fpage list */

		fpage_t *mpu_first;	/*! head of MPU fpage list */
		fpage_t *mpu_stack_first;	/*! head of MPU stack fpage list */
		uint32_t shared;	/*! shared user number */
		static int map_area(as_t& src, as_t& dst, memptr_t base, size_t size, map_action_t action, bool is_priviliged);
		static as_t *as_create(uint32_t as_spaceid);
		static void as_destroy(as_t *as);
		static void as_setup_mpu(as_t& as, memptr_t sp, memptr_t pc, memptr_t stack_base, size_t stack_size);
		void map_user();
		void map_ktext();
		static void mpu_enable(mpu_state_t i);
		static void mpu_setup_region(int n, fpage_t *fp);
		int mpu_select_lru(uint32_t addr);
	} ;
	/**
	 * Memory pool represents area of physical address space
	 * We set flags to it (kernel & user permissions),
	 * and rules for fpage creation
	 */
	struct mempool_t {
		const char *name;
		const memptr_t start;
		const memptr_t end;
		const uint32_t flags;
		const uint32_t tag;
		template<typename START_T, typename END_T>
		constexpr mempool_t(const char* name, START_T start, END_T end, uint32_t flags, uint32_t tag)
		: name(name),start(reinterpret_cast<memptr_t>(start)), start(reinterpret_cast<memptr_t>(end)), flags(flags),tag(tag) {}
	} ;


} /* namespace os */
}
#endif /* OS_MEMORY_HPP_ */