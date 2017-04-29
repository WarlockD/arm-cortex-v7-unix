#ifndef _LIST_HPP_
#define _LIST_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <list>
namespace sys {
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

	template<typename T> struct slist_entry;
	template<typename T> struct slist_head;
	template<typename T, bool _is_const> struct slist_iterator;

	template<typename T>
	struct slist_traits {
		using difference_type = ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using const_pointer = const T*;
		using const_reference = const T&;
		using entry_type = slist_entry<T>;
		using head_type = slist_head<T>;
		using field_type = slist_entry<T> T::*;
		using iterator =  slist_iterator<T,false>;
		using const_iterator =  slist_iterator<T,true>;
		// helper for getting the entry
		__attribute__((always_inline))
		constexpr static inline slist_entry<T>& field(T* obj, field_type field) noexcept  {
			return  obj->*field;
		}
		__attribute__((always_inline))
		constexpr static inline const slist_entry<T>& field(const T* obj, field_type field) noexcept {
			return obj->*field;
		}
		__attribute__((always_inline))
		constexpr static inline slist_entry<T>& field(T& obj, field_type field) noexcept  {
			return obj.*field;
		}
		__attribute__((always_inline))
		constexpr static inline const slist_entry<T>& field(const T& obj, field_type field) noexcept {
			return obj.*field;
		}
	};

	template<typename T>
	struct slist_entry{
		using difference_type = typename slist_traits<T>::difference_type;
		using iterator_category = typename slist_traits<T>::iterator_category;
		using value_type =  typename slist_traits<T>::value_type;
		using pointer =  typename slist_traits<T>::pointer;
		using reference = typename slist_traits<T>::reference;;
		using const_pointer = typename slist_traits<T>::const_pointer;
		using const_reference = typename slist_traits<T>::const_reference;
		using entry_type = typename slist_traits<T>::entry_type;

		pointer sle_next;
		constexpr slist_entry() : sle_next(nullptr) {}
	};


	template<typename T, bool _is_const>
	struct slist_iterator  {
		using traits = slist_traits<T>;
		using difference_type = typename traits::difference_type;
		using iterator_category = typename traits::iterator_category;
		using value_type =  typename std::conditional<_is_const, const typename traits::value_type,typename traits::value_type>::type;
		using const_value_type =  const typename traits::value_type;
		using pointer = typename std::conditional<_is_const, const T*, T*>::type;
		using reference = typename std::conditional<_is_const, const T&, T&>::type;
		using const_pointer = typename traits::const_pointer;
		using const_reference = typename traits::const_reference;
		using entry_type = typename traits::entry_type;
		using head_type = typename traits::head_type;
		using field_type = typename traits::field_type;
		using iterator =  slist_iterator<T,_is_const>;
		using const_iterator =  slist_iterator<T,true>;

		constexpr slist_iterator(pointer current, field_type field) noexcept :
			_current(current), _field(field) {}
		inline iterator operator++() noexcept {
			if(_current) _current = traits::field(_current, _field).sle_next;
			return iterator(*this);
		}
		inline iterator operator++(int) noexcept {
			iterator tmp(*this);
			if(_current) _current = traits::field(_current, _field).sle_next;
			return tmp;
		}
	      // Must downcast from _List_node_base to _List_node to get to _M_data.
		value_type& operator*()  noexcept { return *_current; }
		//const_value_type& operator*()  noexcept const { return *_current; }
		value_type& operator&()  noexcept { return *_current; }
		const_value_type& operator&()  const noexcept { return *_current; }
	//	const value_type& operator*() const noexcept { return _current; }
	    pointer operator->()  noexcept { return _current; }
	    const pointer operator->() const noexcept { return _current; }
	    // do I need to test if the field pointers are equal?
	    bool operator==(const iterator& r) const { return _current == r._current; }
	    bool operator!=(const iterator& r) const { return _current != r._current; }
	private:
		pointer _current;
		field_type _field;
	};

	template<typename T>
	struct slist_head{
		using difference_type = typename slist_traits<T>::difference_type;
		using iterator_category = typename slist_traits<T>::iterator_category;
		using value_type =  typename slist_traits<T>::value_type;
		using pointer =  typename slist_traits<T>::pointer;
		using reference = typename slist_traits<T>::reference;;
		using const_pointer = typename slist_traits<T>::const_pointer;
		using const_reference = typename slist_traits<T>::const_reference;
		using entry_type = typename slist_traits<T>::entry_type;
		using head_type = typename slist_traits<T>::head_type;
		using field_type = typename slist_traits<T>::field_type;
		using iterator =  typename slist_traits<T>::iterator;
		using const_iterator =  typename slist_traits<T>::const_iterator;

		pointer first() const { return slh_first; }
		//constexpr slist_head(T _Class::* field) noexcept :  _field(field),slh_first(nullptr) {}
		template<typename _Res, typename _Class>
		explicit
		constexpr slist_head(_Res _Class::* field) noexcept :  _field(field),slh_first(nullptr) {}
		constexpr inline slist_entry<T>& field(T* obj) noexcept  {
			return  obj->*_field;
		}
		constexpr inline slist_entry<T>& field(T* obj) const noexcept {
			return  obj->*_field;
		}
		constexpr inline  T*& next(T* p)  {
			return field(p).sle_next;
		}
		 constexpr inline  const T*& next(const T* p) const noexcept  {
			return field(p).sle_next;
		}
		constexpr inline T* first()  {
			return slh_first;
		}
		constexpr inline bool empty()const noexcept  { return slh_first == nullptr; }
		inline void insert_after(T* slistelm, T* elm) noexcept{
			slist_next(elm,field) = next(slistelm);
			slist_next(slistelm,field) = elm;
		}
		inline void insert_head(T* elm) noexcept{ //FT&& field){
			next(elm) = slh_first;
			slh_first = elm;
		}
		inline pointer remove_head() noexcept{
			pointer ret = slh_first;
			if(slh_first !=nullptr)
				slh_first = next(slh_first);
			return ret;
		}
		inline void remove_after(T* elm)noexcept{
			slist_next(elm,field) = slist_next(next(elm));
		}
		inline pointer remove(T* elm) noexcept{ //FT&& field){
			//QMD_SAVELINK(oldnext, (elm)->field.sle_next);
			slist_next(elm,field) = slh_first;
			slh_first = elm;
			if(slh_first == elm)
				return remove_head();
			else {
				T* curelm = slh_first;
				while(next(curelm) != elm)
					curelm = next(curelm);
				remove_after(curelm);
				return curelm;
			}
			//TRASHIT(*oldnext);
		}
		iterator begin() { return iterator(slh_first, _field); }
		iterator end() { return iterator(nullptr, _field); }
		const_iterator begin() const { return const_iterator(slh_first, _field); }
		const_iterator end() const{ return const_iterator(nullptr, _field); }
	protected:
		field_type _field;
		pointer slh_first;
	};

	// helper, like a stupid mem_fn

	template<typename T, typename _Class>
	struct slist_foreach : public slist_traits<T> {
		using difference_type = typename slist_traits<T>::difference_type;
		using iterator_category = typename slist_traits<T>::iterator_category;
		using value_type =  typename slist_traits<T>::value_type;
		using pointer =  typename slist_traits<T>::pointer;
		using reference = typename slist_traits<T>::reference;;
		using const_pointer = typename slist_traits<T>::const_pointer;
		using const_reference = typename slist_traits<T>::const_reference;
		using entry_type = typename slist_traits<T>::entry_type;
		using head_type = typename slist_traits<T>::head_type;
		using field_type = typename slist_traits<T>::field_type;
		using iterator =  typename slist_traits<T>::iterator;
		using const_iterator =  typename slist_traits<T>::const_iterator;


		slist_foreach(head_type& head, field_type field) noexcept : _field(field),_from(head.slh_first) {}
		slist_foreach(pointer from, field_type field) noexcept :  _field(field),_from(from) {}
		iterator begin() { return iterator(_from,_field); }
		iterator end() { return iterator(nullptr,_field); }
		const_iterator begin() const { return const_iterator(_from,_field); }
		const_iterator end() const { return const_iterator(nullptr,_field); }
	protected:
		pointer _from;
		field_type _field;
	};
	template<typename T, typename U> constexpr size_t offsetOf(U T::*member)
	{
	    return (char*)&((T*)nullptr->*member) - (char*)nullptr;
	}
	namespace _oldc11 {
			template <typename T1, typename T2>
			struct offset_of_impl {
				static T2 object;
				static constexpr size_t offset(T1 T2::*member) {
					return size_t(&(offset_of_impl<T1, T2>::object.*member)) -
						   size_t(&offset_of_impl<T1, T2>::object);
				}
			};
			template <typename T1, typename T2>
			T2 offset_of_impl<T1, T2>::object;

			template <typename T1, typename T2>
			inline constexpr size_t offset_of(T1 T2::*member) {
				return offset_of_impl<T1, T2>::offset(member);
			}
		};
		template <typename T1, typename T2>
		inline size_t constexpr offset_of(T1 T2::*member) {
		    constexpr T2 object {};
		    return size_t(&(object.*member)) - size_t(&object);
		}




};













#endif