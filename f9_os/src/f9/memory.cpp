#include "fpage.hpp"
#include "link.hpp"
#ifdef CONFIG_ENABLE_FPAGE

#define DECLARE_MEMPOOL(name_, start_, end_, flags_, tag_) mempool_t(name_,start_,end_,flags_,tag_)

#define DECLARE_MEMPOOL_2(name, prefix, flags, tag) DECLARE_MEMPOOL(name, &(prefix ## _start), &(prefix ## _end), flags, tag)
namespace f9 {
	extern kip_mem_desc_t *mem_desc;
	extern char *kip_extra;
	/**
	 * Memory map of MPU.
	 * Translated into memdesc array in KIP by memory_init
	 */
	static mempool_t memmap[] = {
		DECLARE_MEMPOOL_2("KTEXT", kernel_text,
			MP::KR | MP::KX | MP::NO_FPAGE, MPT::KERNEL_TEXT),
		DECLARE_MEMPOOL_2("UTEXT", user_text,
			MP::UR | MP::UX | MP::MEMPOOL | MP::MAP_ALWAYS, MPT::USER_TEXT),
		DECLARE_MEMPOOL_2("KIP", kip,
			MP::KR | MP::KW | MP::UR | MP::SRAM, MPT::KERNEL_DATA),
		DECLARE_MEMPOOL("KDATA", &kip_end, &kernel_data_end,
			MP::KR | MP::KW | MP::NO_FPAGE, MPT::KERNEL_DATA),
		DECLARE_MEMPOOL_2("KBSS",  kernel_bss,
			MP::KR | MP::KW | MP::NO_FPAGE, MPT::KERNEL_DATA),
		DECLARE_MEMPOOL_2("UDATA", user_data,
			MP::UR | MP::UW | MP::MEMPOOL | MP::MAP_ALWAYS, MPT::USER_DATA),
		DECLARE_MEMPOOL_2("UBSS",  user_bss,
			MP::UR | MP::UW | MP::MEMPOOL  | MP::MAP_ALWAYS, MPT::USER_DATA),
		DECLARE_MEMPOOL("MEM0",  &mem0_start, 0x2001c000,
			MP::UR | MP::UW | MP::SRAM, MPT::AVAILABLE),
	#ifdef CONFIG_BITMAP_BITBAND
		DECLARE_MEMPOOL("KBITMAP",  &bitmap_bitband_start, &bitmap_bitband_end,
			MP::KR | MP::KW | MP::NO_FPAGE, MPT::KERNEL_DATA),
	#else
		DECLARE_MEMPOOL("KBITMAP",  &bitmap_start, &bitmap_end,
			MP::KR | MP::KW | MP::NO_FPAGE, MPT::KERNEL_DATA),
	#endif
	#ifndef CONFIG_BOARD_STM32P103
		DECLARE_MEMPOOL("MEM1",   &mem1_start, 0x10010000,
			MP::UR | MP::UW | MP::AHB_RAM, MPT::AVAILABLE),
	#endif
		DECLARE_MEMPOOL("APB1DEV", 0x40000000, 0x40007800,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("APB2_1DEV", 0x40010000, 0x40014c00,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("APB2_2DEV", 0x40014000, 0x40014c00,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("AHB1_1DEV", 0x40020000, 0x40023c00,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("AHB1_2DEV", 0x40023c00, 0x40040000,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("AHB2DEV", 0x50000000, 0x50061000,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("AHB3DEV", 0x60000000, 0xA0001000,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
	#ifdef CONFIG_BOARD_STM32F429DISCOVERY
		DECLARE_MEMPOOL("APB2_3DEV", 0x40015000, 0x40015c00,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("APB2_4DEV", 0x40016800, 0x40017900,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("CR_PLLSAION_BB", 0x42470000, 0x42470c00,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("LCD_FRAME_BUFFER_1", 0xD0000000, 0xD00A0000,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
		DECLARE_MEMPOOL("LCD_FRAME_BUFFER_2", 0xD00A0000, 0xD0140000,
			MP::UR | MP::UW | MP::DEVICES, MPT::DEVICES),
	#endif
	};

	 mempool_t *mempool_t::getbyid(int mpid){
		if (mpid == -1) return nullptr;
		return memmap + mpid;
	 }
	 int mempool_t::search(memptr_t base, size_t size){
		for (size_t i = 0; i < sizeof(memmap) / sizeof(mempool_t); ++i) {
			if ((memmap[i].start <= base) &&
				(memmap[i].end >= (base + size))) {
				return i;
			}
		}
		return -1;
	}
	  void mempool_t::init(){
			int j = 0;
		//	uint32_t *shcsr = (uint32_t *) 0xE000ED24;

			//fpages_init();

			//ktable_init(&as_table);

			mem_desc = (kip_mem_desc_t *) kip_extra;

			/* Initialize mempool table in KIP */
			for (size_t i = 0; i < sizeof(memmap) / sizeof(mempool_t); ++i) {
				switch (memmap[i].tag) {
				case MPT::USER_DATA:
				case MPT::USER_TEXT:
				case MPT::DEVICES:
				case MPT::AVAILABLE:
					mem_desc[j].base = mempool_t::addr_align(
							(memmap[i].start),
					                CONFIG_SMALLEST_FPAGE_SIZE) | i;
					mem_desc[j].size = mempool_t::addr_align(
							(memmap[i].end - memmap[i].start),
							CONFIG_SMALLEST_FPAGE_SIZE) | static_cast<uint32_t>(memmap[i].tag);
					j++;
					break;
				default: break;
				}
			}
			//*shcsr |= 1 << 16;	/* Enable memfault */
	  }

	//  using bitmap_type_t = bitops::bitmap_table_t<as_t, CONFIG_MAX_ADRESS_SPACES>;

#define remove_fpage_from_list(as, fpage, first, next) {	\
	fpage_t *fpprev = (as)->first;				\
	int end;						\
	if (fpprev == (fpage)) {				\
		(as)->first = fpprev->next;			\
	}							\
	else {							\
		while (!end && fpprev->next != (fpage)) {	\
			if (fpprev->next == NULL)		\
				end = 1;			\
			fpprev = fpprev->next;			\
		}						\
		fpprev->next = (fpage)->next;			\
	}							\
}
	  DECLARE_KTABLE(as_t, as_table, CONFIG_MAX_ADRESS_SPACES);

	as_t::~as_t(){
		fpage_t *fp, *fpnext;
		 fp = this->first;

		/*
		 * FIXME: What if a CLONED fpage which is MAPPED is to be deleted
		 */
		while (fp) {
			if (static_cast<FPAGE>(fp->fpage.flags) % FPAGE::CLONE)
				this->unmap(fp);
				//unmap_fpage(fp);
			else
				delete fp;
			fpnext = fp->as_next;
			fp = fpnext;
		}
	}
#if 0
	bool as_t::unmap(fpage_t *fpage){

	}
#endif
	void as_t::map_user()
	{
		for(const auto& map: memmap){
			switch(map.tag){
			case MPT::USER_DATA:
			case MPT::USER_TEXT:
			case MPT::DEVICES:
				/* Map user text, data and hardware device memory */
				assign(map.start, (map.end - map.start));
				break;
			default:break;
			}
		}
	}
	void as_t::map_ktext()
	{
		for(const auto& map: memmap){
			if(map.tag == MPT::KERNEL_TEXT)
				assign(map.start, (map.end - map.start));
		}
	}



	bool as_t::assign_ext(int mpid, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast){
		fpage_t **fp;
		memptr_t  end;
		if (size <= 0) return -1;
		/* if mpid is unknown, search using base addr */
		if (mpid == -1) {
			if ((mpid = mempool_t::search(base, size)) == -1) {
				/* Cannot find appropriate mempool, return error */
				return false;
			}
		}
		end = base + size;

		/* find unmapped space */
		fp = &this->first;
		while (base < end && *fp) {
			if (base < (*fp)->base()) {
				fpage_t *first = nullptr, *last = nullptr;
				size = (end < (*fp)->base() ? end : (*fp)->base()) - base;
#ifdef CONFIG_DEBUG
				dbg::print(dbg::DL_MEMORY,
						   "MEM: fpage chain %s [b:%p, sz:%p] as %p\n",
						   mempool_t::getbyid(mpid)->name, base, size, this);
#endif
				fpage_t::create_chain(mempool_t::addr_align(mpid, base),
						mempool_t::addr_align(mpid, size),
								   mpid, &first, &last);

				last->as_next = *fp;
				*fp = first;
				fp = &last->as_next;

				if (!*pfirst)
					*pfirst = first;
				*plast = last;

				base = (*fp)->end();
			} else if (base < (*fp)->end()) {
				if (!*pfirst)
					*pfirst = *fp;
				*plast = *fp;

				base = (*fp)->end();
			}

			fp = &(*fp)->as_next;
		}

		if (base < end) {
			fpage_t *first = nullptr, *last = nullptr;
			size = end - base;
#ifdef CONFIG_DEBUG
			dbg::print(dbg::DL_MEMORY,
					   "MEM: fpage chain %s [b:%p, sz:%p] as %p\n",
					   mempool_t::getbyid(mpid)->name, base, size, this);
#endif
			fpage_t::create_chain(mempool_t::addr_align(mpid, base),
					mempool_t::addr_align(mpid, size),
							   mpid, &first, &last);

			*fp = first;

			if (!*pfirst)
				*pfirst = first;
			*plast = last;
		}
		return true;
	}

	int assign_fpages(as_t *as, memptr_t base, size_t size)
	{
		fpage_t *first = nullptr, *last = nullptr;
		return assign_fpages_ext(-1, as, base, size, &first, &last);
	}
	void as_t::insert_chain(fpage_t *first, fpage_t *last){
		fpage_t *fp = this->first;

		if (!fp) {
			/* First chain in AS */
			this->first = first;
			return;
		}

		if (last->fpage.base < fp->fpage.base) {
			/* Add chain into beginning */
			last->as_next = this->first;
			this->first = first;
		} else {
			/* Search for chain in the middle */
			while (fp->as_next != NULL) {
				if (last->base() < fp->as_next->base()) {
					last->as_next = fp->as_next;
					break;
				}
				fp = fp->as_next;
			}
			fp->as_next = first;
		}
	}
	void as_t::insert(fpage_t *fpage){
		insert_chain(fpage,fpage);
	}
	void as_t::remove(fpage_t *fpage){
		remove_fpage_from_list(this, fpage, first, as_next);
		remove_fpage_from_list(this, fpage, mpu_first, mpu_next);
	}
	void as_t::map(fpage_t *fpage, map_action_t action, as_t* src){// src is only used for debuging
		fpage_t *fpmap = new fpage_t();

		/* FIXME: check for fpmap == NULL */
		fpmap->as_next = nullptr;
		fpmap->mpu_next = nullptr;

		/* Copy fpage description */
		fpmap->raw[0] = fpage->raw[0];
		fpmap->raw[1] = fpage->raw[1];

		/* Set flags correctly */
		if (action == MAP)
			fpage->fpage.flags |= FPAGE::MAPPED;
		fpmap->fpage.flags = FPAGE::CLONE;

		/* Insert into mapee list */
		fpmap->map_next = fpage->map_next;
		fpage->map_next = fpmap;

		/* Insert into AS */
		insert(fpmap);

		dbg::print(dbg::DL_MEMORY, "MEM: %s fpage %p from %p to %p\n",
		           (action == MAP) ? "mapped" : "granted", fpage, src, this);
	}
	bool as_t::unmap(fpage_t *fpage){
		fpage_t *fpprev = fpage;

		dbg::print(dbg::DL_MEMORY, "MEM: unmapped fpage %p from %p\n", fpage, this);

		/* Fpages that are not mapped or granted
		 * are destroyed with its AS
		 */
		if (!(fpage->fpage.flags & bitmask_cast(FPAGE::CLONE)))
			return false;

		while (fpprev->map_next != fpage)
			fpprev = fpprev->map_next;

		/* Clear flags */
		fpprev->fpage.flags &= ~ bitmask_cast(FPAGE::MAPPED);

		fpprev->map_next = fpage->map_next;
		remove(fpage);
		delete fpage;

		return true;
	}
	fpage_t *as_t::split(fpage_t *fpage, memptr_t split, int rl) {
		memptr_t base = fpage->fpage.base,
				 end = fpage->fpage.base + (1 << fpage->fpage.shift);
		fpage_t *lfirst = nullptr, *llast = nullptr, *rfirst = nullptr, *rlast = nullptr;
		split = mempool_t::addr_align(fpage->fpage.mpid, split);


		/* Check if we can split something */
		if (split == base || split == end) {
			return fpage;
		}

		if (fpage->map_next != fpage) {
			/* Splitting not supported for mapped pages */
			/* UNIMPLIMENTED */
			return NULL;
		}

		/* Split fpage into two chains of fpages */
		fpage_t::create_chain(base, (split - base), fpage->fpage.mpid, &lfirst, &llast);
		fpage_t::create_chain(split, (end - split), fpage->fpage.mpid, &rfirst, &rlast);

		remove(fpage);
		delete fpage;
		insert_chain(lfirst, llast);
		insert_chain(rfirst, rlast);
		if (rl == 0) return llast;
		return rfirst;
	}
	void as_t::setup_mpu(memptr_t sp, memptr_t pc, memptr_t stack_base, size_t stack_size){
		as_t* as = this;
		fpage_t *mpu[8] = { nullptr };
		fpage_t *fp;
		int mpu_first_i;
		int i, j;

		fpage_t *mpu_stack_first = nullptr;
		memptr_t start = stack_base;
		memptr_t end = stack_base + stack_size;

		/* Find stack fpages */
		fp = as->first;
		i = 0;
		while (i < 8 && fp != NULL && start < end) {
			if (fp->addr_inside(start,false)) {
				if (!mpu_stack_first)
					mpu_stack_first = fp;

				mpu[i++] = fp;
				start =fp->end();
			}
			fp = fp->as_next;
		}

		as->mpu_stack_first = mpu_stack_first;
		mpu_first_i = i;

		/*
		 * We walk through fpage list
		 * mpu_fp[0] are pc
		 * mpu_fp[1] are always-mapped fpages
		 * mpu_fp[2] are others
		 */
		fp = as->mpu_first;
		if (fp == nullptr) {
			fpage_t *mpu_first[3] = {nullptr};
			fpage_t *mpu_fp[3] = {nullptr};

			fp = as->first;
			while (fp != nullptr) {
				int priv = 2;

				if (fp->addr_inside(pc,false)) {
					priv = 0;
				} else if (static_cast<FPAGE>(fp->fpage.flags) & FPAGE::ALWAYS) {
					priv = 1;
				}

				if (mpu_first[priv] == nullptr) {
					mpu_first[priv] = fp;
					mpu_fp[priv] = fp;
				} else {
					mpu_fp[priv]->mpu_next = fp;
					mpu_fp[priv] = fp;
				}

				fp = fp->as_next;
			}

			if (mpu_first[1]) {
				mpu_fp[1]->mpu_next = mpu_first[2];
			} else {
				mpu_first[1] = mpu_first[2];
			}
			if (mpu_first[0]) {
				mpu_fp[0]->mpu_next = mpu_first[1];
			} else {
				mpu_first[0] = mpu_first[1];
			}
			as->mpu_first = mpu_first[0];
		}

		/* Prevent link to stack pages */
		for (fp = as->mpu_first; i < 8 && fp != NULL; fp = fp->mpu_next) {
			for (j = 0; j < mpu_first_i; j++) {
				if (fp == mpu[j]) {
					break;
				}
			}

			if (j == mpu_first_i) {
				mpu[i++] = fp;
			}
		}

		as->mpu_first = mpu[mpu_first_i];

		/* Setup MPU stack regions */
		for (j = 0; j < mpu_first_i; ++j) {
			fpage_t::mpu_setup_region(j, mpu[j]);

			if (j < mpu_first_i - 1)
				mpu[j]->mpu_next = mpu[j + 1];
			else
				mpu[j]->mpu_next = nullptr;
		}

		/* Setup MPU fifo regions */
		for (; j < i; ++j) {
			fpage_t::mpu_setup_region(j, mpu[j]);

			if (j < i - 1)
				mpu[j]->mpu_next = mpu[j + 1];
		}

		/* Clean unused MPU regions */
		for (; j < 8; ++j) {
			fpage_t::mpu_setup_region(j, nullptr);
		}
	}
	int map_area(as_t *src, as_t *dst, memptr_t base, size_t size, map_action_t action, bool is_priviliged) {
		/* Most complicated part of mapping subsystem */
			memptr_t end = base + size, probe = base;
			fpage_t *fp = src->first, *first = NULL, *last = NULL;
			int last_invalid = 0;

			/* FIXME: reverse mappings (i.e. thread 1 maps 0x1000 to thread 2,
			 * than thread 2 does the same to thread 1).
			 */

			/* For priviliged thread (ROOT), we use shadowed mapping,
			 * so first we will check if that fpages exist and then
			 * create them.
			 */

			/* FIXME: checking existence of fpages */

			if (is_priviliged) {
				if(src) src->assign_ext(-1,base,size,&first,&last);
				else fpage_t::assign_ext(-1,base,size,&first,&last);
				if (src == dst) {
					/* Maps to itself, ignore other actions */
					return 0;
				}
			} else {
				if (src == dst) {
					/* Maps to itself, ignore other actions */
					return 0;
				}
				/* We should determine first and last fpage we will map to:
				 *
				 * +----------+    +----------+    +----------+
				 * |   first  | -> |          | -> |  last    |
				 * +----------+    +----------+    +----------+
				 *     ^base            ^    +size      =    ^end
				 *                      | probe
				 *
				 * probe checks that addresses in fpage are sequental
				 */
				while (fp) {
					if (!first && fp->addr_inside(base,false)) {
						first = fp;
					}

					if (!last && fp->addr_inside(base,true)) {
						last = fp;
						break;
					}

					if (first) {
						/* Check weather if addresses in fpage list
						 * are sequental
						 */
						if (!fp->addr_inside(probe,true))
							return -1;
						probe += (1 << fp->fpage.shift);
					}

					fp = fp->as_next;
				}
			}

			if (!last || !first) {
				/* Not in address space or error */
				return -1;
			}

			if (first == last)
				last_invalid = 1;

			/* That is a problem because we should split
			 * fpages into two (and split all mappings too)
			 */

			first = src->split(first, base, 1);

			/* If first and last were same pages, after first split,
			 * last fpage will be invalidated, so we search it again
			 */
			if (last_invalid) {
				fp = first;
				while (fp) {
					if (fp->addr_inside(end, true)) {
						last = fp;
						break;
					}
					fp = fp->as_next;
				}
			}

			last  = src->split(last, end, 0);

			if (!last || !first) {
				/* Splitting not supported for mapped pages */
				/* UNIMPLIMENTED */
				return -1;
			}

			/* Map chain of fpages */

			fp = first;
			while (fp != last) {
				dst->map(fp,action,src);
				//map_fpage(src, dst, fp, action);
				fp = fp->as_next;
			}
			dst->map(fp,action,src);

			return 0;
	}


};
#endif
