#ifndef _STAILQ_HPP_
#define _STAILQ_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <cassert>


template<typename T> struct stailq_entry;
template<typename T> using stailq_member = stailq_entry<T> T::*;

template<typename T, stailq_member<T> FIELD> struct stailq_head;
template<typename T, stailq_member<T> FIELD,bool _is_const> struct stailq_iterator;

template<typename T, stailq_member<T> FIELD> struct stailq_pointer;

template<typename T, stailq_member<T> FIELD=nullptr>
struct stailq_traits {
	using difference_type = ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using const_value_type =  const typename std::remove_const<T>::type;
	using pointer = T*;
	using reference = T&;
	using const_pointer = const T*;
	using const_reference = const T&;
	using entry_type = stailq_entry<T>;
	using field_type = stailq_member<T>;

	constexpr static field_type member=FIELD;
	constexpr static field_type member_offset=member == nullptr ? 0 : (size_t) &( reinterpret_cast<pointer>(0)->*member);
	constexpr static inline  pointer& next(pointer p)  {
		static_assert(member!=nullptr,"Don't know what field you want!");
		return (p->*member).stqe_next;
	}
	template<typename M>
	constexpr static inline pointer container_of_entry(M* ptr) { return (pointer)( (char*)ptr - member_offset); }
	template<typename M>
	constexpr static inline const_pointer container_of_entry(const M* ptr) { return (pointer)( (char*)ptr - member_offset); }
#if 0
	template<class P, class M>
	constexpr static inline std::size_t offsetof_entry() { return (size_t) &( reinterpret_cast<P*>(0)->*member); }
	template<class P, class M>
	constexpr static inline std::size_t offsetof_entry(const M P::*member)
	{
		//__builtin_offsetof (TYPE, MEMBER)
	    return (size_t) &( reinterpret_cast<P*>(0)->*member);
	}
	template<class P, class M>
	constexpr static inline P* container_of_entry(M* ptr, const M P::*member)
	{
	    return (P*)( (char*)ptr - offsetof_entry(member));
	}
#endif
};
// container class so we can access the pointer next functions
template<typename T, stailq_member<T> FIELD=nullptr>
struct stailq_pointer {
	using traits = stailq_traits<T,FIELD>;
	using type = typename traits::head_type;
	using difference_type = typename traits::difference_type;
	using iterator_category = typename traits::iterator_category;
	using value_type =  typename traits::value_type;
	using pointer =  typename traits::pointer;
	using reference = typename traits::reference;;
	using const_pointer = typename traits::const_pointer;
	using const_reference = typename traits::const_reference;
	using entry_type = typename traits::entry_type;
	using head_type =  typename traits::head_type;
	using field_type = typename traits::field_type;
	using iterator =  stailq_iterator<T,FIELD,false>;
	using const_iterator =  stailq_iterator<T,FIELD,true>;
	constexpr stailq_pointer(pointer current = nullptr) noexcept : _current(current){
		static_assert(FIELD!=nullptr,"Don't know what field you want!");
	}
	constexpr inline pointer& next()  { return traits::next(_current); }
	constexpr inline pointer next() const { return traits::next(_current); }
	  // Must downcast from _List_node_base to _List_node to get to _M_data.
	constexpr inline value_type& operator*()  const noexcept { return *_current; }

	//const_value_type& operator*()  noexcept const { return *_current; }
	constexpr inline value_type& operator&()  const noexcept { return *_current; }
	constexpr inline pointer operator->()  const noexcept { return _current; }
protected:
	pointer _current;
};
template<typename T>
struct stailq_entry  {
	using traits = stailq_traits<T>;
	using type = typename traits::entry_type;
	using difference_type = typename traits::difference_type;
	using iterator_category = typename traits::iterator_category;
	using value_type =  typename traits::value_type;
	using pointer =  typename traits::pointer;
	using reference = typename traits::reference;;
	using const_pointer = typename traits::const_pointer;
	using const_reference = typename traits::const_reference;
	using entry_type = typename traits::entry_type;

	pointer stqe_next;
	constexpr stailq_entry() : stqe_next(nullptr) {}
};


//template<typename T>
template<typename T, stailq_member<T> FIELD>
struct stailq_head  {
	using traits = stailq_traits<T,FIELD>;
	using type = stailq_head<T,FIELD>;
	using difference_type = typename traits::difference_type;
	using iterator_category = typename traits::iterator_category;
	using value_type =  typename traits::value_type;
	using pointer =  typename traits::pointer;
	using reference = typename traits::reference;;
	using const_pointer = typename traits::const_pointer;
	using const_reference = typename traits::const_reference;
	using entry_type = typename traits::entry_type;
	using head_type =  stailq_head<T,FIELD>;
	using field_type = typename traits::field_type;
	using iterator =  stailq_iterator<T,FIELD,false>;
	using const_iterator =  stailq_iterator<T,FIELD,true>;
	using container = stailq_pointer<T,FIELD>;


	constexpr stailq_head() noexcept: stqh_first(nullptr),stqh_last(&stqh_first) {}

	constexpr inline reference front() noexcept {return *stqh_first;}
	constexpr inline const_reference front() const noexcept {return *stqh_first;}
	constexpr inline reference last()  noexcept {return *traits::container_of_entry(stqh_last); }
	constexpr inline const_reference last() const noexcept { return *traits::container_of_entry(stqh_last); }

	constexpr inline void remove_all() noexcept{ stqh_first = nullptr;stqh_last=&stqh_first; }
	constexpr inline bool empty()const noexcept  { return stqh_first == nullptr; }


	inline void insert_sorted_chain(pointer f, pointer l) noexcept {
		insert_sorted_chain(f,l, [](const_reference l, const_reference r) { return l < r; });
	}
	inline void insert_after(iterator tqelm, pointer elm) noexcept {
		insert_after(tqelm._current,elm);
	}
	inline void insert_after(pointer tqelm, pointer elm) noexcept {
		if ((traits::next(elm) = traits::next(tqelm)) == nullptr)
				stqh_last = &traits::next(elm);
		traits::next(tqelm) = elm;
	}
	inline void insert_head(pointer elm) noexcept{ //FT&& field){
		if ((traits::next(elm) = stqh_first) == nullptr)
					stqh_last = &traits::next(elm);
		stqh_first = elm;
	}
	inline void insert_tail(pointer elm) noexcept{ //FT&& field){
		traits::next(elm) = nullptr;
		*stqh_last = elm;
		stqh_last= &traits::next(elm);
	}
	inline iterator remove_head() noexcept{
		iterator it = stqh_first;
		if((stqh_first = traits::next(stqh_first))==nullptr)
			stqh_last=&stqh_first;
		return it;
	}
	inline void remove_after(iterator it) noexcept {
		remove_after(it._current);
	}
	inline void remove_after(pointer elm)noexcept{
		if((traits::next(elm) = traits::next(traits::next(elm)))==nullptr)
			stqh_last=&traits::next(elm);
	}
	inline void remove(iterator it) noexcept {
		remove(it._current);
	}
	inline void remove(pointer elm) noexcept{
		//QMD_SAVELINK(oldnext, (elm)->field.sle_next);
		if(elm == nullptr) return;
		else if(stqh_first == elm) {
			remove_head();
		} else {
			pointer curelm = stqh_first;
			while(curelm!= nullptr && traits::next(curelm) != elm)
				curelm = traits::next(curelm);
			assert(curelm != nullptr); // it should be in here, otherwise its our fault
			remove_after(curelm);
		}
		//TRASHIT(*oldnext);
	}
	inline iterator erase(iterator it) {
		pointer curelm = it._current;
		if(curelm!= nullptr) {
			++it;
			remove(curelm);
		}
		return it;
	}
	void swap(head_type& r) noexcept {
		std::swap(stqh_last,r.stqh_last);
		std::swap(stqh_first,r.stqh_first);
		if(r.empty()) r = head_type();
		if(empty()) *this = head_type(); // make sure the pointers are fixed right if empty
	}
	head_type& operator+=(head_type&& r) {
		if(!r.empty()) {
			*stqh_last = r.stqh_first;
			stqh_last = r.stqh_last;
			r = head_type();
		}
		return *this;
	}
	iterator begin() { return iterator(stqh_first); }
	iterator end() { return iterator(nullptr); }
	const_iterator begin() const { return const_iterator(stqh_first); }
	const_iterator end() const{ return const_iterator(nullptr); }
protected:
	friend iterator;
	pointer stqh_first;	/* first element */			\
	pointer* stqh_last;	/* addr of last next element */		\
};
template<typename T, stailq_member<T> FIELD,bool _is_const>
struct stailq_iterator  {
	using traits = stailq_traits<typename std::remove_const<T>::type,FIELD>;
	using difference_type = typename traits::difference_type;
	using value_type = typename std::conditional<_is_const,const  T,T>::type;
	using pointer = value_type*;
	using reference = value_type&;
	using iterator_category = std::forward_iterator_tag;
	using iterator = stailq_iterator<T,FIELD,_is_const>;

	constexpr stailq_iterator(pointer current = nullptr) noexcept : _current(current){}
	inline iterator operator++() noexcept {
		if(_current) _current = traits::next(_current);
		return iterator(*this);
	}
	inline iterator operator++(int) noexcept {
		iterator tmp(*this);
		if(_current) _current = traits::next(_current);
		return tmp;
	}
	  // Must downcast from _List_node_base to _List_node to get to _M_data.
	value_type& operator*()  const noexcept { return *_current; }
	//const_value_type& operator*()  noexcept const { return *_current; }
	value_type& operator&()  const noexcept { return *_current; }
	pointer operator->()  const noexcept { return _current; }

	// do I need to test if the field pointers are equal?
	bool operator==(const iterator& r) const { return _current == r._current; }
	bool operator!=(const iterator& r) const { return _current != r._current; }
protected:
	friend stailq_head<T,FIELD>;
	pointer _current;
};

template<typename T, stailq_member<T> FIELD>
struct stailq_foreach {
	using traits = stailq_traits<T,FIELD>;
	using head_type = typename traits::head_type;
	using pointer = typename traits::pointer;
	using iterator = typename traits::iterator;
	using const_iterator =  typename traits::const_iterator;
	stailq_foreach(head_type& head)  : _from(head.slh_first)   {}
	stailq_foreach(pointer f)  : _from(f)  {}
	iterator begin() { return iterator(_from); }
	iterator end() { return iterator(nullptr); }
	const_iterator begin() const { return const_iterator(_from); }
	const_iterator end() const { return const_iterator(nullptr); }
protected:
	pointer _from;
};



#endif
