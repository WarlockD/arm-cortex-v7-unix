/*
 * casting.hpp
 *
 *  Created on: Jun 2, 2017
 *      Author: Paul
 */

#ifndef OS_CASTING_HPP_
#define OS_CASTING_HPP_

#include <type_traits>


namespace priv {
// case c++17 isn't out yet:(
	template <size_t ...I>
	struct index_sequence {};

	template <size_t N, size_t ...I>
	struct make_index_sequence : public make_index_sequence<N - 1, N - 1, I...> {};

	template <size_t ...I>
	struct make_index_sequence<0, I...> : public index_sequence<I...> {};
// https://stackoverflow.com/questions/16893992/check-if-type-can-be-explicitly-converted
// better idea for a cast
template<typename From, typename To>
struct _is_explicitly_convertible
{
    template<typename T>
    static void f(T);

    template<typename F, typename T>
    static constexpr auto test(int) ->
    decltype(f(static_cast<T>(std::declval<F>())),true) { return true; }

    template<typename F, typename T>
    static constexpr auto test(...) -> bool {  return false; }

    static bool constexpr value = test<From,To>(0);
};
template<typename From, typename To>
struct is_explicitly_convertible : std::conditional<_is_explicitly_convertible<From,To>::value,std::true_type, std::false_type>::type {
    static bool constexpr value = _is_explicitly_convertible<From,To>::value;
    using type = typename std::conditional<_is_explicitly_convertible<From,To>::value,std::true_type, std::false_type>::type;

};
// silly cast
	template<typename T>
	struct cast_info {
		using in_type = typename std::remove_reference<T>::type;
		constexpr static bool is_pointer = std::is_pointer<in_type>::value || std::is_array<in_type>::value;
		using type = typename std::conditional<std::is_pointer<in_type>::value,typename  std::remove_pointer<in_type>::type,in_type>::type;
		constexpr static bool is_intergral = std::is_integral<in_type>::value;
	};
	template<typename FROM>
	constexpr static inline uintptr_t __ptr_to_uint(FROM v,std::true_type) { return reinterpret_cast<uintptr_t>(v); }
	template<typename FROM>
	constexpr static inline uintptr_t __ptr_to_uint(FROM v,std::false_type) { return static_cast<uintptr_t>(v); }
	template<typename FROM>
	constexpr static inline uintptr_t _ptr_to_uint(FROM v) { return __ptr_to_uint(v,std::is_pointer<FROM>()); }
	template<typename FROM, typename TO>
	constexpr static inline TO _ptr_to_int(FROM v,std::true_type) { return reinterpret_cast<TO>(v); }
	template<typename FROM, typename TO>
	constexpr static inline TO _ptr_to_int(FROM v,std::false_type) { return static_cast<TO>(v); }

	template<typename FROM, typename TO>
	constexpr static inline TO __ptr_to_int(FROM&& v,std::false_type) { return reinterpret_cast<TO>(v); }
	template<typename FROM, typename TO>
	constexpr static inline TO __ptr_to_int(FROM&& v,std::true_type) { return static_cast<TO>(v); }

	//template<typename T>
	//constexpr static inline uintptr_t to_uintptr_t(T v) { return _ptr_to_int<T,uintptr_t>(v,std::is_pointer<T>()); }

	template<typename T>
	constexpr static inline uintptr_t to_uintptr_t(T&& v) { return __ptr_to_int<T,uintptr_t>(v,is_explicitly_convertible<T,uintptr_t>()); }
	template<typename T,typename P>
	constexpr static inline P to_pointer(T&& v) { return __ptr_to_int<T,uintptr_t>(v,
			typename std::conditional<std::is_pointer<P>::value,
			std::false_type,
			typename std::conditional<is_explicitly_convertible<T,uintptr_t>::value,
				std::true_type,
				std::false_type>::type>::type


			{}); }
#if 0
	template <typename... T>
	auto remove_ref_from_tuple_members(std::tuple<T...> const& t) {
	    return std::tuple<typename std::remove_reference<T>::type...>{ t };
	}
	template <class Tuple, size_t... Is>
	constexpr auto _to_uintptr(Tuple t, index_sequence<Is...>) {
		return std::make_tuple(_ptr_to_uint(std::get<Is>(t))... );

		//return std::tie(_ptr_to_uint(std::get<Is>(t)) ...);
	}
#endif
	template<typename T>
	constexpr auto _to_uintptr_truple(T&& v) { return std::make_tuple(_ptr_to_uint(v)); }
	template<typename T,typename ... Args>
	constexpr auto _to_uintptr_truple(T&& v, Args ... args) { return std::make_tuple(_ptr_to_uint(v), _to_uintptr_truple(std::forward<Args>(args)...)); }
	template <typename ... Args>
	constexpr auto  to_uintptr_truple(Args ... args) { return _to_uintptr_truple(std::forward<Args>(args)...); }
};
//template<typename T>
//constexpr static inline uintptr_t to_uintptr_t(T v) { return _ptr_to_int<T,uintptr_t>(v,std::is_pointer<T>()); }

template<typename T>
constexpr static inline uintptr_t ptr_to_int(T v) { return priv::_ptr_to_int<T,uintptr_t>(std::forward<T>(v),std::is_pointer<T>()); }


template<typename T>
constexpr static inline T int_to_ptr(uintptr_t v) { return priv::_ptr_to_int<uintptr_t,T>(v,std::is_pointer<T>()); }
//template<typename T>
constexpr static inline uintptr_t ptr_to_int(nullptr_t) { return uintptr_t{}; }
// helper to convert a pointer to a uintptr_t
template<typename T>
constexpr static inline typename std::enable_if<std::is_arithmetic<T>::value, void*>::type
to_voidp(T v) { return static_cast<void*>(v); }
template<typename T>
constexpr static inline void*
to_voidp(T* v) { return reinterpret_cast<void*>(v); }
	typedef uint32_t reg_t;
	typedef int irq_id_t;

#endif /* OS_CASTING_HPP_ */
