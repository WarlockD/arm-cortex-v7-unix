/*
 * fpage.hpp
 *
 *  Created on: May 17, 2017
 *      Author: Paul
 */

#ifndef F9_FPAGE_HPP_
#define F9_FPAGE_HPP_

#include "types.hpp"
#ifdef CONFIG_ENABLE_FPAGE
namespace f9 {
// enums
	enum class MP {
		/* Kernel permissions flags */
		KR			=0x0001,
		KW			=0x0002,
		KX			=0x0004,

		/* Userspace permissions flags */
		UR			=0x0010,
		UW			=0x0020,
		UX			=0x0040,

		/* Fpage type */
		NO_FPAGE	=0x0000,		/*! Not mappable */
		SRAM		=0x0100,		/*! Fpage in SRAM: granularity 1 << */
		AHB_RAM		=0x0200,		/*! Fpage in AHB SRAM: granularity 64 words, bit bang mappings */
		DEVICES		=0x0400,		/*! Fpage in AHB/APB0/AHB0: granularity 16 kB */
		MEMPOOL		=0x0800,		/*! Entire mempool is mapped  */
		/* Map memory from mempool always (for example text is mapped always because
		 * without it thread couldn't run)
		 * other fpages mapped on request because we limited in MPU resources)
		 */
		MAP_ALWAYS 	= 0x1000,

		FPAGE_MASK	= 0x0F00,
	};
	enum class MPT {
		KERNEL_TEXT,
		KERNEL_DATA,
		USER_TEXT,
		USER_DATA,
		AVAILABLE,
		DEVICES,
		UNKNOWN = -1
	} ;

	enum FPAGE {
		ALWAYS    =0x1,     //! Fpage is always mapped in MPU
		CLONE     =0x2,     //! Fpage is mapped from other AS
		MAPPED    =0x4,     //! Fpage is mapped with MAP
	};
};

// way we can do this without splitting the namespace?
template<>
struct enable_bitmask_operators<f9::MP>{
	static const bool enable=true;
};
// way we can do this without splitting the namespace?
template<>
struct enable_bitmask_operators<f9::FPAGE>{
	static const bool enable=true;
};
namespace f9 {
	typedef struct {
		uint32_t 	base;	/* Last 6 bits contains poolid */
		uint32_t	size;	/* Last 6 bits contains tag */
	} kip_mem_desc_t;


	static inline MP USER_PERM(MP flags) { return static_cast<MP>((static_cast<uint32_t>(flags) & 0xF0) >> 4); }

	struct as_t;
	enum map_action_t { MAP, GRANT, UNMAP } ;
	struct fpage_t{
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

	#ifdef CONFIG_KDB
		int used;
	#endif /* CONFIG_KDB */
		uint32_t base() const { return fpage.base; }
		uint32_t size() const { return 1<< fpage.shift; }
		uint32_t end() const { return base() + size(); }
		inline bool addr_inside(memptr_t addr, bool incl_end = false) const {
			return (addr >= base() && addr < end()) || (incl_end && addr == end());
		}
#if 0
		static fpage_t *split_fpage(as_t *as, fpage_t *fpage, memptr_t split, int rl);

		static int assign_fpages(as_t *as, memptr_t base, size_t size);
		static int assign_fpages_ext(int mpid, as_t *as, memptr_t base, size_t size,
		                      fpage_t **pfirst, fpage_t **plast);

		static int map_fpage(as_t *src, as_t *dst, fpage_t *fpage, map_action_t action);
		static int unmap_fpage(as_t *as, fpage_t *fpage);
		static void destroy_fpage(fpage_t *fpage);
#endif
		static void mpu_setup_region(int j, fpage_t* f);
		//fpage_t(as_t*src, as_t *dst, fpage_t *fpage, map_action_t action); //map_fpage(as_t *src, as_t *dst, fpage_t *fpage, map_action_t action)
		static bool assign_ext(int mpid, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast);
	private:
		static void * operator new (size_t size);
		static	void operator delete (void * mem);
		static void create_chain(memptr_t base, size_t size, int mpid, fpage_t **pfirst, fpage_t **plast);
			fpage_t(memptr_t base, size_t shift, int mpid);
		friend int map_area(as_t *src, as_t *dst, memptr_t base, size_t size, map_action_t action, bool is_priviliged);
		friend int map_fpage(as_t *src, as_t *dst, fpage_t *fpage, map_action_t action);
		fpage_t() : as_next(nullptr), map_next(nullptr),mpu_next(nullptr) {}
		~fpage_t() {}
		friend struct as_t;

	};
	/*
	 * Each address space is associated with one or more thread.
	 * AS represented as linked list of fpages, mappings are the same.
	 */



	struct as_t : public std::enable_shared_from_this<as_t> {
		uint32_t as_spaceid;	/*! Space Identifier */
		fpage_t *first;	/*! head of fpage list */

		fpage_t *mpu_first;	/*! head of MPU fpage list */
		fpage_t *mpu_stack_first;	/*! head of MPU stack fpage list */
	//	uint32_t shared;	/*! shared user number */
		as_t(uint32_t spaceid) : as_spaceid(spaceid), first(nullptr), mpu_first(nullptr), mpu_stack_first(nullptr) {} //shared(0) {}
		~as_t();
		void setup_mpu(memptr_t sp, memptr_t pc, memptr_t stack_base, size_t stack_size);
		void map_user();
		void map_ktext();
		int assign_fpage(memptr_t base, size_t size);

		int unmap_fpage(fpage_t *fpage);

		static void operator delete(void* ptr);
		static void* operator new(std::size_t sz);
		bool assign(memptr_t base, size_t size);
		bool assign_ext(int mpid, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast);

		void map(fpage_t *fpage, map_action_t action, as_t* src=nullptr); // src is only used for debuging
		bool unmap(fpage_t *fpage);
		fpage_t *split(fpage_t *fpage, memptr_t split, int rl);
		void aquire();
	private:
		void insert(fpage_t *fpage);
		void remove(fpage_t *fpage);
		void insert_chain(fpage_t *first, fpage_t *last);
		int assign(int mpid, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast);
		friend int map_area(as_t *src, as_t *dst, memptr_t base, size_t size, map_action_t action, bool is_priviliged);
		friend int assign_fpages_ext(int mpid, as_t *as, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast);
		friend struct fpage_t;
	} ;
	namespace mpu {
		void enable(bool i);
		void setup_region(int n, fpage_t* fp);
		int select_lru(as_t* as, uint32_t addr);
	};
	using sas_t = std::shared_ptr<as_t>;
	// this big one
	int map_area(as_t *src, as_t *dst, memptr_t base, size_t size, map_action_t action, bool is_priviliged);
	inline int map_area(sas_t& src, sas_t& dst, memptr_t base, size_t size, map_action_t action, bool is_priviliged){
		return map_area(src.get(), dst.get(), base,size,action,is_priviliged);
	}
	int map_fpage(as_t *src, as_t *dst, fpage_t *fpage, map_action_t action);


	/**
	 * Memory pool represents area of physical address space
	 * We set flags to it (kernel & user permissions),
	 * and rules for fpage creation
	 */


	struct mempool_t {
	#ifdef CONFIG_DEBUG
		const char *name;
	#endif
		memptr_t start;
		memptr_t end;

		MP flags;
		MPT tag;

#ifdef CONFIG_DEBUG
		template<typename ST, typename ET>
		mempool_t(const char* name, ST start, ET end, MP flags, MPT tag)
		: name(name), start(ptr_to_int(start)), end(ptr_to_int(end)),
		  flags(flags),tag(tag) {}



		mempool_t(const char* name, memptr_t start, memptr_t end, MP flags, MPT tag): name(name), start(start), end(end),flags(flags),tag(tag) {}

		mempool_t(memptr_t start, memptr_t end, MP flags, MPT tag): name(""), start(start), end(end),flags(flags),tag(tag) {}
#else
		mempool_t(const char* name, memptr_t start, memptr_t end, MP flags, MPT tag): start(start), end(end),flags(flags),tag(tag) {}
		mempool_t(memptr_t start, memptr_t end, MP flags, MPT tag): start(start), end(end),flags(flags),tag(tag) {}
#endif
		static mempool_t *getbyid(int mpid);
		static int search(memptr_t base, size_t size);
		template<typename BT>
		inline static int search(BT base, size_t size) { return search(ptr_to_int(base), size); }
		static void init();
		static constexpr memptr_t addr_align(memptr_t addr, size_t size) { return (addr + (size - 1)) & ~(size - 1); }
	} ;


} /* namespace f9 */
// way we can do this without splitting the namespace?
#endif

#endif /* F9_FPAGE_HPP_ */
