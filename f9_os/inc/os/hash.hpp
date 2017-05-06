#ifndef _HASH_HPP_
#define _HASH_HPP_


// fixed hash lust based off list.  Seen it iused in stuff in lite bsd

#include "list.hpp"

// its basicly a wraper for list with a fixed bucket list
// the catch is the KEY is a var arg function. that is you use lambas
// for the hash function and equal function
/*
 * for HASHER its size_t HASHER(args...);
 * AND HASHER its size_t HASHER(const_refrence object);
 * for EQUALS its bool EQUALS(const_refrence object, args...);
 * AND bool EQUALS(const_refrence object, const_refrence object)
 */

template<typename T, list_member<T> FIELD, size_t _BUCKET_COUNT, typename HASHER, typename EQUALS>
class hash_list {
public:
	using traits = list_traits<T,FIELD>;
	using type = hash_list<T,FIELD,_BUCKET_COUNT,HASHER,EQUALS>;
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
	constexpr static size_t BUCKET_COUNT = _BUCKET_COUNT;
	class hash_list_iterator {
		using difference_type = typename traits::difference_type;
		using iterator_category = std::forward_iterator_tag; // only forward for right now
		using value_type =  typename traits::value_type;
		using pointer =  typename traits::pointer;
		using reference = typename traits::reference;
		using iterator =  hash_list_iterator;
		type& _hlist;
		head_type* _cbucket;
		pointer _current;
		void _get_next() {
			do {
				if(_cbucket>= &_hlist._buckets[BUCKET_COUNT]) break;
				if(_current == nullptr && (_current = &_cbucket->front()) != nullptr) break;
				if(_current != nullptr && (_current = traits::next(_current)) != nullptr) break;
				++_cbucket;
			} while(1);
		}
	public:
		hash_list_iterator(type& hlist, size_t bindex) : _hlist(hlist), _cbucket(&hlist._buckets[bindex]),_current(nullptr) { _get_next(); }
		inline iterator operator++() noexcept {
			_get_next();
			return iterator(*this);
		}
		inline iterator operator++(int) noexcept {
			iterator tmp(*this);
			_get_next();
			return tmp;
		}
		  // Must downcast from _List_node_base to _List_node to get to _M_data.
		value_type& operator*()  const noexcept { return *_current; }
		//const_value_type& operator*()  noexcept const { return *_current; }
		value_type& operator&()  const noexcept { return *_current; }
		pointer operator->()  const noexcept { return _current; }

		// do I need to test if the field pointers are equal?
		bool operator==(const iterator& r) const { return _current == r._current && _cbucket == r._cbucket; }
		bool operator!=(const iterator& r) const { return !(*this == r); }


	};
	using iterator =  hash_list_iterator;

	iterator begin() { return iterator(*this,0); }
	iterator end() { return iterator(*this,BUCKET_COUNT); }
	hash_list() : _hasher{}, _equals{} {}
	// this is for debugging

	//const HASHER _hasher = HASHER{};
	//const EQUALS _equals= EQUALS{};
	template<typename ... Args>
	pointer search(Args... args) {
		size_t hash = _hasher(std::forward<Args>(args)...);
		head_type& bucket = _buckets[hash% BUCKET_COUNT];
		if(!bucket.empty()){
			for(reference o : bucket){
				if(_equals(std::forward<const_reference>(o), std::forward<Args>(args)...)){
					return &o;
				}
			}
		}
		return nullptr;
	}
	pointer insert(pointer obj) {
		const_reference obj_ref = *obj;
		size_t hash = _hasher(std::forward<const_reference>(obj_ref)) ;
		head_type& bucket = _buckets[hash% BUCKET_COUNT];
		if(!bucket.empty()) {
			for(reference o : bucket){
				if(_equals(std::forward<const_reference>(o), std::forward<const_reference>(obj_ref))){
					return &o == obj ? obj : &o; // return the pointer its equal to
				}
			}
		}
		// insert it then
		bucket.insert_head(obj);
		return obj;
	}
	template<typename ... Args>
	pointer remove(Args... args) {
		size_t hash = _hasher(std::forward<Args>(args)...) % BUCKET_COUNT;
		head_type& bucket = _buckets[hash];
		if(!bucket.empty()){
			for(reference o : bucket){
				if(_equals(std::forward<const_reference>(o), std::forward<Args>(args)...)){
					bucket.remove(&o);
					return &o;
				}
			}
		}
		return nullptr;
	}
	// this is for debuging
	head_type* dbg_bucket(size_t i) { return  i < BUCKET_COUNT ? &_buckets[i] : nullptr; }
protected:
	head_type _buckets[BUCKET_COUNT];
	const HASHER _hasher;
	const EQUALS _equals;
};






#endif
