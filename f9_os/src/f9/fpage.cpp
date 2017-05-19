/*
 * fpage.cpp
 *
 *  Created on: May 17, 2017
 *      Author: Paul
 */

#include "fpage.hpp"

#ifdef CONFIG_ENABLE_FPAGE
#if 0
// change this to template somehow?
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
#endif

namespace f9 {
	/*
	 * Helper functions
	 */
	#if 0
	static int fp_addr_log2(memptr_t addr)
	{
		int shift = 0;

		while ((addr <<= 1) != 0)
			++shift;

		return 31 - shift;
	}
	#else
	static constexpr size_t fp_addr_log2(memptr_t n) { return ( (n<2) ? 1 : 1+fp_addr_log2(n/2));}
	#endif
	extern int assign_fpages_ext(int mpid, as_t *as, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast);


	DECLARE_KTABLE(fpage_t, fpage_table, CONFIG_MAX_FPAGES);
#if 0
	using bitmap_type = bitops::bitmap_table_t<fpage_t,CONFIG_MAX_FPAGES>;
    static bitmap_type g_fpage_table; // be sure to find a place to put this
	void * fpage_t::operator new (size_t size){ return g_fpage_table.alloc(size); }
	void fpage_t::operator delete (void * mem){  g_fpage_table.free(mem); }

#endif
	fpage_t::fpage_t(memptr_t base, size_t shift, int mpid) : as_next(nullptr), map_next(this), mpu_next(nullptr) {
		fpage.mpid = mpid;
		fpage.flags = 0;
		fpage.rwx = static_cast<uint32_t>(USER_PERM(mempool_t::getbyid(mpid)->flags));
		fpage.base = base;
		fpage.shift = shift;
		if (mempool_t::getbyid(mpid)->flags % MP::MAP_ALWAYS)
			fpage.flags |= bitmask_cast(FPAGE::ALWAYS);

	}
	bool fpage_t::assign_ext(int mpid, memptr_t base, size_t size, fpage_t **pfirst, fpage_t **plast){
		if (size <= 0) return -1;
		/* if mpid is unknown, search using base addr */
		if (mpid == -1) {
			if ((mpid = mempool_t::search(base, size)) == -1) {
				/* Cannot find appropriate mempool, return error */
				return false;
			}
		}
#ifdef CONFIG_DEBUG
		dbg::print(dbg::DL_MEMORY,
		           "MEM: fpage chain %s [b:%p, sz:%p] as %p\n",
		           mempool_t::getbyid(mpid)->name, base, size, nullptr);
#endif
		create_chain(mempool_t::addr_align(mpid, base),
				mempool_t::addr_align(mpid, size),
		                   mpid, pfirst, plast);
		return true;

	}
	void fpage_t::create_chain(memptr_t base, size_t size, int mpid, fpage_t **pfirst, fpage_t **plast){
		int shift, sshift, bshift;
		fpage_t *fpage = NULL;

		while (size) {
			/* Select least of log2(base), log2(size).
			 * Needed to make regions with correct align
			 */
			bshift = fp_addr_log2(base);
			sshift = fp_addr_log2(size);

			shift = (size_t)((1 << bshift) > size) ? sshift : bshift;

			if (!*pfirst) {
				/* Create first page */
				fpage = new fpage_t(base, shift, mpid);
				*pfirst = fpage;
				*plast = fpage;
			} else {
				/* Build chain */
				fpage->as_next = new fpage_t(base, shift, mpid);
				fpage = fpage->as_next;
				*plast = fpage;
			}
			size -= (1 << shift);
			base += (1 << shift);
		}
	}

};
#if 0
	using bitmap_type_t = typename  bitops::bitmap_table_t<fpage_t, CONFIG_MAX_FPAGES>;
	static bitmap_type_t g_fpage_alloc;
	void f_page::operator delete(void* ptr){

	}
	void* f_page::operator new(std::size_t sz){

	}
	fpage_t *f_page::split(as_t *as, memptr_t split, int rl){

	}


 void as_t::insert_fpage_chain_to_as(as_t *as, fpage_t *first, fpage_t *last){
		fpage_t *fp = as->first;

		if (!fp) {
			/* First chain in AS */
			as->first = first;
			return;
		}

		if (last->fpage.base < fp->fpage.base) {
			/* Add chain into beginning */
			last->as_next = as->first;
			as->first = first;
		} else {
			/* Search for chain in the middle */
			while (fp->as_next != nullptr) {
				if (last->base() < fp->as_next->base()) {
					last->as_next = fp->as_next;
					break;
				}
				fp = fp->as_next;
			}
			fp->as_next = first;
}

 }

} /* namespace f9 */

#endif
#endif
