#ifndef _HASH_HPP_
#define _HASH_HPP_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <cassert>

#include <diag\Trace.h>



namespace hash {
		enum class status {
			ok=0,		// insert or remove went fine
			exists=1,		// already exists on insert but pointer dosn't equal
			dup=2,		// already inside by another pointer, dosn't mean its equal though
			dup_exists=3 // both the pointer and key equals
		};
		template<typename T>
		struct int_hasher {
			constexpr size_t operator()(uintptr_t x) const {
			    x = ((x >> 16) ^ x) * 0x45d9f3b;
			    x = ((x >> 16) ^ x) * 0x45d9f3b;
			    x = (x >> 16) ^ x;
			    return x;
			}
			constexpr size_t operator()(const void* x) const {
				return operator()(reinterpret_cast<uintptr_t>(x));
			}
			constexpr size_t operator()(const T x) const {
				return operator()(static_cast<uintptr_t>(x));
			}
			constexpr size_t operator()(const T* x) const {
				return operator()(reinterpret_cast<uintptr_t>(x));
			}
		};
		template<typename T>
		struct pointer_hasher  {
			constexpr size_t operator()(const T& x) const { return int_hasher<T>()(*x); }
			constexpr size_t operator()(const T* x) const { return int_hasher<T>()(x); }
			constexpr size_t operator()(uintptr_t x) const { return int_hasher<T>()(x); }
		};

		template<typename T>
		struct pointer_equals  {
			constexpr bool operator()(const T& a, const T& b) const { return &a == &b; }
			constexpr bool operator()(const T& a, const T* b) const { return &a == b; }
			constexpr bool operator()(const T& a, uintptr_t b) const { return reinterpret_cast<uintptr_t>(&a) == b; }
		};
		template<typename T, typename _CONTAINER_TYPE, size_t _BUCKET_COUNT, typename HASHER = pointer_hasher<T>, typename EQUALS = pointer_equals<T>>
		class table {
		public:
			constexpr static size_t BUCKET_COUNT = _BUCKET_COUNT;
			using hasher = HASHER;
			using equals = EQUALS;
			using container_type = _CONTAINER_TYPE;
			using container_iterator = typename container_type::iterator;
	 		using traits = typename _CONTAINER_TYPE::traits;
	 		using type = table<T,_CONTAINER_TYPE,_BUCKET_COUNT, HASHER, EQUALS>;
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
	 		using bucket_array_type = std::array<container_type,BUCKET_COUNT>;


			class head_iterator {
			public:
				using difference_type = typename traits::difference_type;
				using iterator_category = std::forward_iterator_tag; // only forward for right now
				using value_type =  typename container_type::value_type;
				using pointer =  typename container_type::pointer;
				using reference = typename container_type::reference;
				using container_iterator = typename container_type::iterator;
				using iterator =  head_iterator;
			private:
				bucket_array_type& _hlist;
				size_t _pos;
				container_iterator cit;

				void _get_next() {
					while(_pos < _hlist.size()) {
						if(_hlist[_pos].end() != cit) ++cit;
						while(_hlist[_pos].end() == cit) {
							if(++_pos >= _hlist.size()) return;
							cit = _hlist[_pos].begin();
						}
						break;
					}
				}
			//	size_t load_count;
			public:
				//size_t load_avg() const { return  load_count/
				head_iterator(type& hlist, size_t bindex, container_iterator it) : _hlist(hlist), _pos(bindex), cit(it) {  }
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
				value_type& operator*()  const noexcept { return &(*cit); }
				//const_value_type& operator*()  noexcept const { return *_current; }
				value_type& operator&()  const noexcept { return *cit; }
				pointer operator->()  const noexcept { return &(*cit); }

				// do I need to test if the field pointers are equal?
				bool operator==(const iterator& r) const { return cit == r.cit && _pos == r._pos; }
				bool operator!=(const iterator& r) const { return !(*this == r); }
			};
			using iterator =  head_iterator;

			iterator begin() { return iterator(_buckets,0,_buckets[0].begin()); }
			iterator end() { return iterator(_buckets,BUCKET_COUNT, _buckets[BUCKET_COUNT].end()); }
			table() : _hasher{}, _equals{} {}
			// this is for debugging

			//const HASHER _hasher = HASHER{};
			//const EQUALS _equals= EQUALS{};
			inline bool obj_equal(const_reference a, const_reference b) const { return _equals(a,b); }
			inline bool obj_equal(const_reference a, const_pointer b) const { return obj_equal(a,*b); }
			inline bool obj_equal(const_reference a, pointer b) const { return obj_equal(a,*b); }

			template<typename ... Args>
			inline bool obj_equal(const_reference a, Args... args) const {
				return _equals(a,std::forward<Args>(args)...);
			}

			template<typename ... Args>
			pointer search(Args... args) {
				size_t hash = _hasher(std::forward<Args>(args)...);
				auto&  bucket = _buckets[hash% BUCKET_COUNT];
				typename head_type::iterator it= bucket.begin();
				while(it != bucket.end()) {
					pointer p = &(*it);
					if(obj_equal(std::forward<T>(*p), std::forward<Args>(args)...))
						return p;
					it++;
				}
				return nullptr;
			}

			status insert(pointer obj,bool insert_dup=false) {
				const_reference obj_ref = *obj;
				size_t hash = _hasher(obj_ref);
				auto& bucket = _buckets[hash% BUCKET_COUNT];
				auto it= bucket.begin();
				while(it != bucket.end()) {
					pointer p = &(*it);
					if(p == obj) return status::exists;
					else if(obj_equal(*p, *obj)){
						if(insert_dup) break;
						else  return (p == obj) ? status::dup_exists : status::dup;
					}
					it++;
				}
				bucket.insert(it,obj);

				return status::ok;
			}
			status insert(reference obj) { return insert(&obj); }

			template<typename ... Args>
			pointer remove(Args... args) noexcept {
				size_t hash = _hasher(std::forward<Args>(args)...) % BUCKET_COUNT;
				auto& bucket = _buckets[hash];
				typename head_type::iterator it= bucket.begin();
				while(it != bucket.end()) {
					pointer p = &(*it);
					if(obj_equal(std::forward<T>(*p), std::forward<Args>(args)...)) {
						bucket.erase(it);
						return p;
					}
					it++;
				}
				return nullptr;
			}

			pointer remove(pointer obj) {
				// in theroy we don't have to check to make sure its in here
				// we can just delete it and it iwlil remove it from the approprate bucket
				const_reference obj_ref = *obj;
				size_t hash = _hasher(obj_ref) % BUCKET_COUNT;
				auto& bucket = _buckets[hash];
				typename head_type::iterator it= bucket.begin();
				while(it != bucket.end()) {
					pointer p = &(*it);
					if(p == obj && obj_equal(*p, *obj)) {
						bucket.erase(it);
						return p;
					}
					it++;
				}
				return nullptr;
			}
			// this is for debuging
			head_type* dbg_bucket(size_t i) { return  i < BUCKET_COUNT ? &_buckets[i] : nullptr; }
		protected:
			bucket_array_type _buckets;
			const HASHER _hasher;
			const EQUALS _equals;
		};



}

#endif
