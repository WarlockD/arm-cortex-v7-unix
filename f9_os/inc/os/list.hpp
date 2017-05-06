#ifndef _LIST_HPP_
#define _LIST_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <cassert>

#include <os\printk.h>
#define LIST_DEBUG(...) do { printk(__VA_ARGS__); assert(0); } while(0);

template<typename T> struct list_entry;
template<typename T> using list_member = list_entry<T> T::*;

//template<typename T, list_member<T> FIELD> struct list_head;
template<typename T, list_member<T> FIELD,bool _is_const> struct list_iterator;

template<typename T, list_member<T> FIELD>  struct list_pointer;
template<typename T, list_member<T> FIELD>  struct list_head;;
template<typename T, list_member<T> FIELD=nullptr>
struct list_traits {
	friend T;
	using difference_type = ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using const_value_type =  const typename std::remove_const<T>::type;
	using pointer = T*;
	using reference = T&;
	using const_pointer = const T*;
	using const_reference = const T&;
	using entry_type = list_entry<T>;
	using field_type = list_member<T>;
	//using list_head = list_head<T,FIELD>;
	constexpr static field_type member=FIELD;
	constexpr static field_type member_offset=member == nullptr ? 0 : (size_t) &( reinterpret_cast<pointer>(0)->*member);
	constexpr static inline  pointer& next(pointer p)  {
		static_assert(member!=nullptr,"Don't know what field you want!");
		return (p->*member).le_next;
	}
	constexpr static inline  pointer*& prev(pointer p)  {
		static_assert(member!=nullptr,"Don't know what field you want!");
		return (p->*member).le_prev;
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
template<typename T, list_member<T> FIELD=nullptr>
struct list_pointer {
	friend T;
	using traits = list_traits<T,FIELD>;
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
	using iterator =  list_iterator<T,FIELD,false>;
	using const_iterator =  list_iterator<T,FIELD,true>;
	constexpr list_pointer(pointer current = nullptr) noexcept : _current(current){
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
 struct list_entry  {
	friend T;
	using traits = list_traits<T>;
	using type = typename traits::entry_type;
	using difference_type = typename traits::difference_type;
	using iterator_category = typename traits::iterator_category;
	using value_type =  typename traits::value_type;
	using pointer =  typename traits::pointer;
	using reference = typename traits::reference;;
	using const_pointer = typename traits::const_pointer;
	using const_reference = typename traits::const_reference;
	using entry_type = typename traits::entry_type;


	constexpr list_entry() : le_next(nullptr),le_prev(&le_next)  {}
	pointer le_next;	/* next element */
	pointer *le_prev;	/* address of previous next element */

};


//template<typename T>
template<typename T, list_member<T> FIELD>
 struct list_head  {
	friend T;
	using traits = list_traits<T,FIELD>;

	using type = list_head<T,FIELD>;
	using difference_type = typename traits::difference_type;
	using iterator_category = typename traits::iterator_category;
	using value_type =  typename traits::value_type;
	using pointer =  typename traits::pointer;
	using reference = typename traits::reference;;
	using const_pointer = typename traits::const_pointer;
	using const_reference = typename traits::const_reference;
	using entry_type = typename traits::entry_type;
	using head_type =  list_head<T,FIELD>;
	using field_type = typename traits::field_type;
	using iterator =  list_iterator<T,FIELD,false>;
	using const_iterator =  list_iterator<T,FIELD,true>;
	using container = list_pointer<T,FIELD>;


	constexpr list_head() noexcept: lh_first(nullptr) {}

	constexpr inline reference front() noexcept {return *lh_first;}
	constexpr inline const_reference front() const noexcept {return *lh_first;}
	constexpr inline bool empty()const noexcept  { return lh_first == nullptr; }


	inline void insert_after(iterator listelm, pointer elm) noexcept {
		insert_after(listelm._current,elm);
	}
	inline void insert_after(pointer listelm, pointer elm) noexcept {
		check_next(listelm);
		if ((traits::next(elm) = traits::next(listelm)) == nullptr)
			traits::prev(traits::next(listelm)) = &traits::next(elm);
		traits::next(listelm) = elm;
		traits::prev(elm) = &traits::next(listelm);
	}
	inline void insert_before(iterator listelm, pointer elm) noexcept {
		insert_after(listelm._current,elm);
	}
	inline void insert_before(pointer listelm, pointer elm) noexcept {
		check_prev(listelm);
		traits::prev(elm) = traits::prev(listelm);
		traits::next(elm) = listelm;
		*traits::prev(listelm) = elm;
		traits::prev(listelm) = &traits::next(elm);
	}

	inline void insert_head(pointer elm) noexcept{ //FT&& field){
		check_head(*this);
		if ((traits::next(elm) = lh_first) == nullptr)
			traits::prev(lh_first)  = &traits::next(elm);
		lh_first = elm;
		traits::prev(elm) = &lh_first;
	}

	inline void remove(iterator it) noexcept {
		remove(it._current);
	}
	inline void remove(pointer elm) noexcept{
		check_next(elm);
		check_prev(elm);
		if(traits::next(elm) != nullptr)
			traits::prev(traits::next(elm)) = traits::prev(elm);
		*traits::prev(elm) = traits::next(elm);
	}
	void swap(head_type& r) noexcept {
		std::swap(lh_first,r.lh_first);
		if(lh_first != nullptr) traits::prev(lh_first) = &lh_first;
		if(r.lh_first != nullptr) traits::prev(r.lh_first) = &r.lh_first;
	}
	head_type& operator+=(head_type&& r) {
		pointer curelm = lh_first;
		if(curelm == nullptr) {
			if((lh_first = r.lh_first) != nullptr)
				traits::prev(r.lh_first) = &lh_first;
		}else if(r.lh_first == nullptr){
			while(traits::next(curelm) !=nullptr)
				curelm = traits::next(curelm);
			traits::next(curelm) = r.lh_first;
			traits::prev(r.lh_first)  = &traits::next(curelm);
		}
		r.lh_first = nullptr;
		return *this;
	}
	iterator begin() { return iterator(lh_first); }
	iterator end() { return iterator(nullptr); }
	const_iterator begin() const { return const_iterator(lh_first); }
	const_iterator end() const{ return const_iterator(nullptr); }
protected:
	constexpr static inline void check_head(list_head& head);
	constexpr static inline void check_next(pointer elm);
	constexpr static inline void check_prev(pointer elm);
	friend iterator;
	pointer lh_first;	/* first element */
};
template<typename T,  list_member<T> FIELD,bool _is_const>
struct list_iterator  {
	using traits =  list_traits<typename std::remove_const<T>::type,FIELD>;
	using difference_type = typename traits::difference_type;
	using value_type = typename std::conditional<_is_const,const  T,T>::type;
	using pointer = value_type*;
	using reference = value_type&;
	using iterator_category = std::forward_iterator_tag;
	using iterator =  list_iterator<T,FIELD,_is_const>;

	constexpr list_iterator(pointer current = nullptr) noexcept : _current(current){}
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
	bool operator!=(const iterator& r) const { return !(*this == r); }
protected:
	friend list_head<T,FIELD>;
	pointer _current;
};

template<typename T, list_member<T> FIELD>
struct list_foreach {
	using traits = list_traits<T,FIELD>;
	using head_type = typename traits::head_type;
	using pointer = typename traits::pointer;
	using iterator = typename traits::iterator;
	using const_iterator =  typename traits::const_iterator;
	list_foreach(head_type& head)  : _from(head.slh_first)   {}
	list_foreach(pointer f)  : _from(f)  {}
	iterator begin() { return iterator(_from); }
	iterator end() { return iterator(nullptr); }
	const_iterator begin() const { return const_iterator(_from); }
	const_iterator end() const { return const_iterator(nullptr); }
protected:
	pointer _from;
};

// debug interface
#ifdef LIST_DEBUG
template<typename T, list_member<T> FIELD>
constexpr inline void list_head<T,FIELD>::check_head(list_head& head){
	if(head.lh_first != nullptr && traits::prev(head.lh_first) != &head.lh_first)
		LIST_DEBUG("Bad list head %p first->prev != head", head);
}
template<typename T, list_member<T> FIELD>
constexpr inline void list_head<T,FIELD>::check_next(pointer elm){
	if(traits::next(elm)!=nullptr && traits::prev(traits::next(elm)) != &traits::next(elm))
		LIST_DEBUG("Bad link elm %p next->prev != elm", elm);
}
template<typename T, list_member<T> FIELD>
constexpr inline void list_head<T,FIELD>::check_prev(pointer elm){
	if (*traits::prev(elm) != elm)
			LIST_DEBUG("Bad link elm %p prev->next != elm", elm);
}

#else
template<typename T, list_member<T> FIELD>
constexpr inline void list_head<T,FIELD>::check_head(list_head& head){}
template<typename T, list_member<T> FIELD>
constexpr inline void list_head<T,FIELD>::check_next(pointer elm){}
template<typename T, list_member<T> FIELD>
constexpr inline void list_head<T,FIELD>::check_prev(pointer elm){}
#endif

#endif
