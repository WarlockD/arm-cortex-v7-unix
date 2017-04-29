#ifndef _LIST_HPP_
#define _LIST_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace sys {
#if 0
	namespace priv {
		template<typename _Tp, typename _Res, typename _Class>
		auto get_field(_Tp&& obj, _Res _Class::* member) noexcept -> decltype(std::forward<_Tp>(obj).*std::declval<_Res _Class::*&>()) {
			return std::forward<_Tp>(obj).*member;
		}
#if 0
		template<typename T, typename FIELD>
		void slist_insert_head(slist_head<T>&head, T* elm, FIELD &&field) { //FT&& field){
			using entry_type = slist_entry<T>;;
			auto t = std::mem_fn(std::forward<FIELD>(field));
			t(*elm).sle_next = head.slh_first;
			//get_field(elm, std::forward<FIELD>(field)).sle_next = head.slh_first;
			head.slh_first = elm;
		}
#endif
	};
#endif
	template<typename T> struct slist_entry;
	template<typename T> using slist_member = slist_entry<T> T::*;

	template<typename T, slist_member<T> FIELD> struct slist_head;
	template<typename T, slist_member<T> FIELD,bool _is_const> struct slist_iterator;


	template<typename T, slist_member<T> FIELD=nullptr>
	struct slist_traits {
		using difference_type = ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using const_value_type =  const typename std::remove_const<T>::type;
		using pointer = T*;
		using reference = T&;
		using const_pointer = const T*;
		using const_reference = const T&;
		using entry_type = slist_entry<T>;
		using field_type = slist_member<T>;
		using head_type = slist_head<T,FIELD>;

		constexpr static field_type member=FIELD;
		constexpr static inline  pointer& next(pointer p)  {
			static_assert(member!=nullptr,"Don't know what field you want!");
			return (p->*member).sle_next;
		}
	};

	template<typename T>
	struct slist_entry  {
		using traits = slist_traits<T>;
		using type = typename traits::entry_type;
		using difference_type = typename traits::difference_type;
		using iterator_category = typename traits::iterator_category;
		using value_type =  typename traits::value_type;
		using pointer =  typename traits::pointer;
		using reference = typename traits::reference;;
		using const_pointer = typename traits::const_pointer;
		using const_reference = typename traits::const_reference;
		using entry_type = typename traits::entry_type;

		pointer sle_next;
		constexpr slist_entry() : sle_next(nullptr) {}
	};




	//template<typename T>
	template<typename T, slist_member<T> FIELD>
	struct slist_head  {
		using traits = slist_traits<T,FIELD>;
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
		using iterator =  slist_iterator<T,FIELD,false>;
		using const_iterator =  slist_iterator<T,FIELD,true>;

		pointer first() const { return slh_first; }
		//constexpr slist_head(T _Class::* field) noexcept :  _field(field),slh_first(nullptr) {}
		//template<typename _Res, typename _Class>
		//explicit constexpr slist_head(_Res _Class::* field) noexcept :  _field(field),slh_first(nullptr) {}
		constexpr slist_head():slh_first(nullptr) {}

		constexpr inline T* first()  {
			return slh_first;
		}
		constexpr inline bool empty()const noexcept  { return slh_first == nullptr; }
		inline void insert_after(T* slistelm, T* elm) noexcept{
			traits::next(elm) = traits::next(slistelm);
			next(slistelm) = elm;
		}
		inline void insert_head(pointer elm) noexcept{ //FT&& field){
			//(elm->*_field).sle_next = slh_first;;
			traits::next(elm)= slh_first;
			slh_first = elm;
		}
		inline pointer remove_head() noexcept{
			pointer ret = slh_first;
			if(slh_first !=nullptr)
				slh_first = traits::next(slh_first);
			return ret;
		}
		inline void remove_after(T* elm)noexcept{
			traits::next(elm) = traits::next(traits::next(elm));
		}
		inline pointer remove(pointer elm) noexcept{
			//QMD_SAVELINK(oldnext, (elm)->field.sle_next);
			if(slh_first == elm) {
				return remove_head();
			} else {
				pointer curelm = slh_first;
				while(traits::next(curelm) != elm)
					curelm = traits::next(curelm);
				remove_after(curelm);
				return curelm;
			}
			//TRASHIT(*oldnext);
		}
		iterator begin() { return iterator(slh_first); }
		iterator end() { return iterator(nullptr); }
		const_iterator begin() const { return const_iterator(slh_first); }
		const_iterator end() const{ return const_iterator(nullptr); }
	protected:
		pointer slh_first;
	};
	template<typename T, slist_member<T> FIELD,bool _is_const>
	struct slist_iterator  {
		using traits = slist_traits<typename std::remove_const<T>::type,FIELD>;
		using difference_type = typename traits::difference_type;
		using value_type = typename std::conditional<_is_const,const  T,T>::type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::forward_iterator_tag;
		using iterator = slist_iterator<T,FIELD,_is_const>;

		constexpr slist_iterator(pointer current = nullptr) noexcept : _current(current){}
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
		pointer _current;
	};

	template<typename T, slist_member<T> FIELD>
	struct slist_foreach {
		using traits = slist_traits<T,FIELD>;
		using head_type = typename traits::head_type;
		using pointer = typename traits::pointer;
		using iterator = typename traits::iterator;
		using const_iterator =  typename traits::const_iterator;
		slist_foreach(head_type& head)  : _from(head.slh_first)   {}
		slist_foreach(pointer f)  : _from(f)  {}
		iterator begin() { return iterator(_from); }
		iterator end() { return iterator(nullptr); }
		const_iterator begin() const { return const_iterator(_from); }
		const_iterator end() const { return const_iterator(nullptr); }
	protected:
		pointer _from;
	};
#if 0
	template<typename T, slist_member<T> FIELD>
	struct slist_foreach_safe {
		using traits = slist_traits<T,FIELD>;
		using head_type = typename traits::head_type;
		using pointer = typename traits::pointer;
		using iterator = typename traits::iterator;
		using const_iterator =  typename traits::const_iterator;
		slist_foreach(head_type& head)  : _from(head.slh_first)   {}
		slist_foreach(pointer f)  : _from(f)  {}
		iterator begin() { return iterator(_from); }
		iterator end() { return iterator(nullptr); }
		const_iterator begin() const { return const_iterator(_from); }
		const_iterator end() const { return const_iterator(nullptr); }
	protected:
		pointer _from;
	};
#endif

};













#endif
