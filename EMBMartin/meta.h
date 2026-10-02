/**
 ******************************************************************************
 * @file    meta.h
 * @author  孙鸣淼
 * @brief   EMBMartin 内置元编程库
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 ******************************************************************************
 */
#ifndef EMBMARTIN_METAPROGRAMING
#define EMBMARTIN_METAPROGRAMING

#include <type_traits>
#include <tuple>
#include <string_view>
#include <etl/string_view.h>

#include "macro.h"

EMBMARTIN_NAMESPACE_BEGIN

// C++17 兼容的 remove_cvref / remove_cvref_t
#if defined(__cpp_lib_remove_cvref)
using std::remove_cvref;
using std::remove_cvref_t;
#else
template <class T>
struct remove_cvref
{
	using type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;
#endif

//------------------------------------------ 是否应当移动 --------------------------------------------------
// 判断类型 T 是否应该通过移动语义传递，如果 T 的大小大于指针大小则返回 true，否则返回 false
template <typename T>
struct should_move : std::conditional_t<(sizeof(T) > sizeof(void*)), std::true_type, std::false_type>
{
};

template <typename T>
constexpr inline bool should_move_v = should_move<T>::value;

//------------------------------------------ 是否有迭代器 --------------------------------------------------
template <typename T, typename = void>
struct has_iterator : std::false_type
{
};

template <typename T>
struct has_iterator<T, std::void_t<
	typename T::iterator,
	typename T::const_iterator,
	decltype(std::declval<T>().begin()),
	decltype(std::declval<T>().end())>> : std::true_type
{
};

template <typename T>
constexpr inline bool has_iterator_v = has_iterator<T>::value;

//------------------------------------------ 判断迭代器指向元素类型 --------------------------------------------------
template <typename Iter, typename T, typename = void>
struct is_iterator_of : std::false_type
{
};

template <typename Iter, typename T>
struct is_iterator_of<Iter, T,
	typename std::enable_if<
	std::is_same<
	typename std::iterator_traits<Iter>::value_type,
	T>::value>::type> : std::true_type
{
};

template <typename Iter, typename T>
constexpr bool is_iterator_of_v = is_iterator_of<Iter, T>::value;

//------------------------------------------ 判断是否为字符类型 --------------------------------------------------
template <typename T>
struct is_character
{
	static constexpr bool value =
		std::is_same_v<T, char> ||
		std::is_same_v<T, signed char> ||
		std::is_same_v<T, unsigned char> ||
		std::is_same_v<T, wchar_t> ||
		std::is_same_v<T, char16_t> ||
		std::is_same_v<T, char32_t>;
};

template <typename T>
constexpr bool is_character_v = is_character<T>::value;

//------------------------------------------ 判断是否为字符数组 --------------------------------------------------
template <typename T>
struct is_character_array : std::false_type
{
};
template <std::size_t N>
struct is_character_array<char[N]> : std::true_type
{
};
template <std::size_t N>
struct is_character_array<const char[N]> : std::true_type
{
};
template <std::size_t N>
struct is_character_array<wchar_t[N]> : std::true_type
{
};

template <std::size_t N>
struct is_character_array<const wchar_t[N]> : std::true_type
{
};
template <std::size_t N>
struct is_character_array<char16_t[N]> : std::true_type
{
};

template <std::size_t N>
struct is_character_array<char32_t[N]> : std::true_type
{
};

template <typename T>
constexpr bool is_character_array_v = is_character_array<T>::value;

//------------------------------------------ 判断是否为字符串类类型 --------------------------------------------------
/**
 * @brief 判断 T 是否为“字符串类”类型
 *
 * 判据是 std::string_view 能否由 T 构造得到，因此 const char* / char* / char[N] /
 * std::string / std::string_view 等类型均为真。
 * 注意方向：不能写成 std::is_constructible_v<T, std::string_view>，
 * 因为 std::string_view 不能隐式转换为 const char*，那样 const char* 会被判为假。
 */
template <typename T>
struct is_string_like
{
	static constexpr bool value = std::is_constructible_v<std::string_view, T> ||
		std::is_constructible_v<etl::string_view, T>;
};

template <typename T>
constexpr bool is_string_like_v = is_string_like<T>::value;

//------------------------------------------ 编译时 Any --------------------------------------------------

/**
 * @brief 安全的编译时 Any, 模板元编程的重要工具
 *
 * 该类具有可以转换为具有任意限定符的任意类型的、仅有函数声明而没有函数定义的转换运算符
 * 可配合 decltype, declval 判断参数数目而不关心参数类型
 *
 * 在运行时该类不可构造，也没有任何可供调用的方法
 */
struct CompileTimeAny
{
	// declval 不需要类型可构造
	constexpr inline CompileTimeAny() noexcept = delete;
	// decltype 不需要函数完整定义
	template <typename T>
	constexpr inline operator T& () noexcept; // T& 可以赋值给 T, const T, T&, const T&
	template <typename T>
	constexpr inline operator T && () noexcept; // T&& 可以赋值给 T&&, const T&&
};

//------------------------------------------ pairs 概念 --------------------------------------------------
// 定义：
// 所有恰好可解包出两个元素 (即 auto [a, b] = pair_obj 合法) 的类型属于广义 pairs 概念
// 所有 2 元素 tuple-like 类型属于狭义 pairs 概念（ pair-like ）

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <typename T, typename... Args>
constexpr auto _test_constructible(int) -> decltype(T{ std::declval<Args>()... }, std::true_type{});

template <typename T, typename... Args>
constexpr std::false_type _test_constructible(...);
template <size_t N, typename T>
struct _is_N_list_constructible_tester
{
private:
	template <size_t... Is>
	static constexpr auto test_construct(std::index_sequence<Is...>)
		-> decltype(detail::_test_constructible<T, decltype((void(Is), std::declval<CompileTimeAny>()))...>(0));

public:
	static constexpr bool value = decltype(test_construct(std::make_index_sequence<N>{}))::value;
};

EMBMARTIN_DETAIL_NAMESPACE_END

// 在 MSVC 中等同于 std::is_constructable<T, CompileTimeAny, CompileTimeAny>
// 但在很多编译器（例如 GCC ）中，聚合类型只能用花括号初始化
template <size_t N, typename T>
struct is_N_list_constructible
	: std::conditional_t<
	detail::_is_N_list_constructible_tester<N, T>::value,
	std::true_type, std::false_type>
{
};

template <size_t N, typename T>
constexpr bool is_N_list_constructible_v = is_N_list_constructible<N, T>::value;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <typename T>
struct _is_two_member_aggregate
	: std::conditional_t<
	is_N_list_constructible_v<2, T> &&
	!is_N_list_constructible_v<3, T> /* 聚合类型的成员可以有默认值 */&&
	std::is_aggregate_v<T> &&
	!std::is_array_v<T> /*C 数组可列表初始化且属于聚合类型 */,
	std::true_type, std::false_type>
{
};

template <typename T>
constexpr bool _is_two_member_aggregate_v = _is_two_member_aggregate<T>::value;

EMBMARTIN_DETAIL_NAMESPACE_END

// 狭义 pair 概念
template <typename T, typename = void>
struct is_pair_like : std::false_type
{
};

template <typename T>
struct is_pair_like<T, std::void_t<decltype(std::tuple_size<T>::value)>>
	: std::integral_constant<
	bool,
	!std::is_same_v<std::tuple_size<T>, std::tuple_size<void>>&&
	std::tuple_size<T>::value == 2>
{
};

template <typename T>
constexpr bool is_pair_like_v = is_pair_like<T>::value;

// 广义 pair 概念
template <typename T>
struct is_generalized_pair
	: std::conditional_t<
	is_pair_like_v<T> || detail::_is_two_member_aggregate_v<T>,
	std::true_type,
	std::false_type>
{
};

template <typename T>
constexpr bool is_generalized_pair_v = is_generalized_pair<T>::value;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N, typename _Pair>
constexpr inline auto _pair_get(const _Pair& pair) noexcept
{
	const auto& [a, b] = pair;
	if constexpr (N == 0)
	{
		return a;
	}
	else if constexpr (N == 1)
	{
		return b;
	}
	else
	{
		static_assert(N <= 1);
	}
}

EMBMARTIN_DETAIL_NAMESPACE_END

template <size_t N, typename _Pair>
struct generalized_pair_element
{
	using type = std::remove_reference_t<decltype(detail::_pair_get<N, _Pair>(std::declval<_Pair>()))>;
};

template <size_t N, typename _Pair>
using generalized_pair_element_t = typename generalized_pair_element<N, _Pair>::type;

// 别名：符合广义 pair 协议的类型
template <typename T>
using is_pair_protocol = is_generalized_pair<T>;
template <typename T>
constexpr bool is_pair_protocol_v = is_generalized_pair_v<T>;
template <size_t N, typename _Pair>
using pair_protocol_element = generalized_pair_element_t<N, _Pair>;
template <size_t N, typename _Pair>
using pair_protocol_element_t = generalized_pair_element_t<N, _Pair>;

// ------------------------------------------ tuples 概念--------------------------------------------------
// 定义：
// 所有可解包的类型属于广义 tuples 概念
// 所有 tuple-like 类型属于狭义 tuples 概念

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N, typename T, typename = void>
struct _is_preN_tuple_element_implemented : std::false_type
{
};

template <typename T>
struct _is_preN_tuple_element_implemented<
	0,
	T,
	std::void_t<decltype(std::declval<std::tuple_element_t<0, T>>())>>
	: std::true_type
{
};
template <size_t N, typename T>
struct _is_preN_tuple_element_implemented<
	N,
	T,
	std::void_t<decltype(std::declval<std::tuple_element_t<N, T>>())>>
	: _is_preN_tuple_element_implemented<N - 1, T>
{
};
template <size_t N, typename T>
constexpr bool _is_preN_tuple_element_implemented_v = _is_preN_tuple_element_implemented<N, T>::value;

template <typename T, typename = void>
struct _is_all_tuple_element_implemented : std::false_type
{
};
template <typename T>
struct _is_all_tuple_element_implemented<T, std::void_t<decltype(std::tuple_size<T>::value)>>
	: _is_preN_tuple_element_implemented<std::tuple_size<T>::value - 1, T>
{
};
template <typename T>
constexpr bool _is_all_tuple_element_implemented_v = _is_all_tuple_element_implemented<T>::value;

template <typename T, size_t... Is>
constexpr bool _has_std_get(std::index_sequence<Is...>)
{
	using U = std::remove_reference_t<T>;
	// 注意：这里用 std::get<Is>(declval<T>())，要求 ADL/重载可见
	return (std::is_same_v<decltype(std::get<Is>(std::declval<T>())), decltype(std::get<Is>(std::declval<T>()))> && ...);
}

EMBMARTIN_DETAIL_NAMESPACE_END

// std tuple
template <typename T>
struct is_std_tuple : std::false_type
{
};

template <typename... Args>
struct is_std_tuple<std::tuple<Args...>> : std::true_type
{
};

template <typename T>
constexpr bool is_std_tuple_v = is_std_tuple<T>::value;

// 狭义 tuples 概念
template <typename T, typename = void>
struct is_tuple_like : std::false_type
{
};

template <typename T>
struct is_tuple_like<
	T,
	std::void_t<
	decltype(std::tuple_size<T>::value)>>
	: detail::_is_all_tuple_element_implemented<T>
{
};

template <typename T>
constexpr bool is_tuple_like_v = is_tuple_like<T>::value;


EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <typename T, size_t... Is>
using _has_std_get_expr_t = std::void_t<decltype(std::get<Is>(std::declval<T>()))...>;

template <typename T, typename Seq, typename = void>
struct _has_std_get_seq : std::false_type
{
};

template <typename T, size_t... Is>
struct _has_std_get_seq<T, std::index_sequence<Is...>, _has_std_get_expr_t<T, Is...>> : std::true_type
{
};

EMBMARTIN_DETAIL_NAMESPACE_END

template <typename T, typename = void>
struct has_std_get : std::false_type
{
};

template <typename T>
struct has_std_get<T, std::void_t<decltype(std::tuple_size<std::remove_reference_t<T>>::value)>>
	: detail::_has_std_get_seq<T, std::make_index_sequence<std::tuple_size<std::remove_reference_t<T>>::value>>
{
};

template <typename T>
constexpr bool has_std_get_v = has_std_get<T>::value;

// 可解包聚合类(广义元组和狭义元组的差集)
template <typename T>
struct is_unpackable_aggregate
	: std::conditional_t<
	std::is_aggregate_v<T> && !std::is_array_v<T> && !std::is_empty_v<T>,
	std::true_type, std::false_type>
{
};
template <typename T>
constexpr bool is_unpackable_aggregate_v = is_unpackable_aggregate<T>::value;

// 广义 tuples 概念
template <typename T>
struct is_generalized_tuple
	: std::conditional_t<
	(std::is_aggregate_v<T> && !std::is_array_v<T>)/* 可解包聚合类型 */ || is_tuple_like_v<T>,
	std::true_type, std::false_type>
{
};
template <typename T>
constexpr bool is_generalized_tuple_v = is_generalized_tuple<T>::value;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

// 测试可解包聚合类型成员数目
template <size_t N, typename T>
struct _aggr_member_num_test_from
	: std::conditional_t<
	is_N_list_constructible_v<N, T> && !is_N_list_constructible_v<N + 1, T>,
	std::integral_constant<size_t, N>,
	_aggr_member_num_test_from<N + 1, T>>
{
};

EMBMARTIN_DETAIL_NAMESPACE_END

template <typename T>
struct unpackable_aggregate_size : detail::_aggr_member_num_test_from<0, T>
{
	static_assert(is_unpackable_aggregate_v<T>, "must be unpackable aggregate type");
};
template <typename T>
constexpr auto unpackable_aggregate_size_v = unpackable_aggregate_size<T>::value;

// 广义 tuple size ; 注意与 std::tuple_size 区分
template <typename T>
struct generalized_tuple_size
	: std::conditional_t<
	is_unpackable_aggregate_v<T>,
	unpackable_aggregate_size<T>,
	std::tuple_size<T>>
{
};
template <typename T>
constexpr auto generalized_tuple_size_v = generalized_tuple_size<T>::value;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <typename TUPLE, typename MEMBER>
constexpr decltype(auto) _aggregate_forward(MEMBER &member) noexcept
{
	if constexpr (std::is_rvalue_reference_v<TUPLE &&>)
	{
		return std::move(member);
	}
	else
	{
		return (member);
	}
}

template <typename TUPLE>
constexpr decltype(auto) _to_tuple(TUPLE &&_tuple_obj)
{
	static_assert(is_generalized_tuple_v<remove_cvref_t<TUPLE>>);
	if constexpr (false)
	{
	}

	// else if constexpr(generalized_tuple_size_v<remove_cvref_t<TUPLE>> == 1 )
	// {
	// 	const auto &[_0] = _tuple_obj;
	// 	return std::forward_as_tuple(_0);
	// }
	// ... 一路展开到 256 元素

#include "_to_tuple_part.txt" // 内联 python 生成的代码

	else
	{
		static_assert(generalized_tuple_size_v<std::remove_reference_t<TUPLE>> <= 256, "too much members");
	}
}
EMBMARTIN_DETAIL_NAMESPACE_END

template<typename TUPLE>
struct corresponding_std_tuple
{
	using type = decltype(detail::_to_tuple(std::declval<const std::remove_reference_t<TUPLE> &>()));
};
template<typename TUPLE>
using corresponding_std_tuple_t = typename corresponding_std_tuple<TUPLE>::type;


template <size_t N, typename T>
struct generalized_tuple_element
{
private:
	using tuple_type = decltype(detail::_to_tuple(std::declval<const std::remove_cv_t<std::remove_reference_t<T>> &>()));

public:
	using type = std::tuple_element_t<N, tuple_type>;
};
template <size_t N, typename T>
using generalized_tuple_element_t = typename generalized_tuple_element<N, T>::type;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN
template <typename Func, typename Tuple, size_t... Is>
constexpr auto _is_invocable_with_std_tuple(std::index_sequence<Is...>)
-> std::bool_constant<
	std::is_invocable_v<Func,
	decltype(std::get<Is>(std::declval<Tuple &&>()))...>>;
EMBMARTIN_DETAIL_NAMESPACE_END

template <typename Func, typename Tuple>
using is_invocable_with_std_tuple = decltype(detail::_is_invocable_with_std_tuple<Func, Tuple>(
	std::make_index_sequence<generalized_tuple_size_v<Tuple>>{}));

template <typename Func, typename Tuple>
constexpr bool is_invocable_with_std_tuple_v =
is_invocable_with_std_tuple<Func, Tuple>::value;

// ------------------------------------------别名：可解包类型/统一在元组协议下的类型--------------------------------------------------
template <typename T>
using is_unpackable = is_generalized_tuple<T>;
template <typename T>
constexpr bool is_unpackable_v = is_unpackable<T>::value;
template <typename T>
using unpack_size = generalized_tuple_size<T>;
template <typename T>
constexpr size_t unpack_size_v = unpack_size<T>::value;
template <size_t N, typename T>
using unpack_element = generalized_tuple_element<N, T>;
template <size_t N, typename T>
using unpack_element_t = typename unpack_element<N, T>::type;

template <typename T>
using is_tuple_protocol = is_generalized_tuple<T>;
template <typename T>
constexpr bool is_tuple_protocol_v = is_tuple_protocol<T>::value;
template <typename T>
using tuple_protocol_size = generalized_tuple_size<T>;
template <typename T>
constexpr size_t tuple_protocol_size_v = tuple_protocol_size<T>::value;
template <size_t N, typename T>
using tuple_protocol_element = generalized_tuple_element<N, T>;
template <size_t N, typename T>
using tuple_protocol_element_t = typename tuple_protocol_element<N, T>::type;

// ------------------------------------------索引序列--------------------------------------------------

template <size_t I, size_t N1, size_t N2>
struct index_swapper
{
	static constexpr size_t value = I == N1 ? N2 : (I == N2 ? N1 : I);
};
template <size_t I, size_t N1, size_t N2>
constexpr size_t index_swapper_v = index_swapper<I, N1, N2>::value;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N1, size_t N2, size_t... Is>
auto _make_swapped_index_sequence(std::index_sequence<Is...>)
{
	return std::index_sequence<index_swapper_v<Is, N1, N2>...>{};
}

EMBMARTIN_DETAIL_NAMESPACE_END

template <size_t I, size_t N1, size_t N2>
auto make_swapped_index_sequence()
{
	return detail::_make_swapped_index_sequence<N1, N2>(std::make_index_sequence<I>{});
}

EMBMARTIN_NAMESPACE_END

#endif