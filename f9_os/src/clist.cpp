#include "clist.hpp"

constexpr static size_t NCLIST = 50;
constexpr static size_t CBSIZE = 28;
constexpr static size_t CROUND  =sizeof(void*) + CBSIZE-1;             /* clist rounding: sizeof(int *) + CBSIZE - 1*/

struct cblock {
	cblock* next;
	char info[CBSIZE];
} __attribute__((aligned(32)));


class _internal_clist {
	cblock* cfreelist;
	cblock cfree[NCLIST];
public:
	_internal_clist() : cfreelist(nullptr) {
		for(cblock& cp : cfree){
			cp.next = cfreelist;
			cfreelist = &cp;
		}
	}
	operator cblock*() { return cfreelist; }
	void release(cblock* bp) {
		 bp->next = cfreelist;
		 cfreelist = bp;
	}
	cblock* aquire() {
		cblock* bp = cfreelist;
		if(bp != nullptr){
			cfreelist = bp->next;
			bp->next = nullptr;
		}
		return bp;
	}
};

static _internal_clist cfreelist;
//#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)
//#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

template<typename T>
constexpr static inline T* ALIGN(T* o,size_t s){
	return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(o) +(s-1))  &~(s-1));
}
template<typename T>
constexpr static inline bool ALIGNED(T* o,size_t s){
	return (reinterpret_cast<uintptr_t>(o) & (s-1)) != 0;
}

int clist::get(){

	int c=-1;
	if(_size > 0){
		c = *_first++ & 0xFF;
		if(--_size == 0) {
			cblock* bp = reinterpret_cast<cblock*>(_first-1);
			 bp = ALIGN(bp,sizeof(cblock));
			 cfreelist.release(bp);
		} else if(ALIGNED(_first, sizeof(cblock))){
			cblock* bp = reinterpret_cast<cblock*>(_first);
			bp--;
			_first = bp->next->info;
			cfreelist.release(bp);
		}
	}
	return c;
}
int clist::put(int c){

}
void clist::get(char* data, size_t size){

}
void clist::put(const char* data, size_t size){

}
size_t clist::size() const{

}
size_t clist::ndflush(size_t count){

}
