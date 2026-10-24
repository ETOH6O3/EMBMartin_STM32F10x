/**
 ******************************************************************************
 * @file    tuple_utility.h
 * @brief   广义元组操作与高级解包：to_tuple / apply / get / unpack / split / insert / emplace / erase 等
 ******************************************************************************
 */

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "EMBMartin/macro.h"
#include "EMBMartin/lang/meta.h"
#include "EMBMartin/lang/functional/tuple_view.h"

EMBMARTIN_NAMESPACE_BEGIN


/**
 * @brief 将任意“tuple-like”对象标准化为 `std::tuple`。
 *
 * 该函数是整套元组扩展的统一入口。
 *
 * 它会根据输入类型分别处理：
 *
 * - 若是左值 `std::tuple`，转发原对象；若是右值 `std::tuple`，转移元素并生成拥有值的 tuple；
 *
 * - 若是 `TupleView` / `TupleInsertView`，则 materialize 成真实 tuple；
 *
 * - 若是具备 `std::get<>` / `std::tuple_size<>` 的对象，左值展开为引用元组，右值展开为拥有型元组；
 *
 * - 若是更广义的聚合对象，则调用 `detail::_to_tuple` 做转换。
 *
 * 这样可以保证后续所有 `std::apply`、`std::get`、`insert`、`erase` 等函数都工作在
 * 一个统一的标准 tuple 语义上，减少模板分支碎片化。
 * 对左值 tuple-like 对象，返回的引用元组不会延长源对象生命周期。
 */
template <typename TUPLE>
constexpr decltype(auto) to_tuple(TUPLE &&_tuple_obj)
{
	using raw_t = std::remove_reference_t<TUPLE>;
	if constexpr (is_std_tuple_v<raw_t>)
	{
		// 右值 std::tuple 会移动元素并生成拥有值的 tuple，避免悬空引用
		if constexpr (std::is_rvalue_reference_v<TUPLE &&>)
		{
			return std::apply(
				[](auto &&...xs)
				{
					return std::make_tuple(std::forward<decltype(xs)>(xs)...);
				},
				std::forward<TUPLE>(_tuple_obj));
		}
		else
		{
			return std::forward<TUPLE>(_tuple_obj);
		}
	}
	else if constexpr (is_tuple_view_v<raw_t>)
	{
		return std::forward<TUPLE>(_tuple_obj).materialize();
	}
	else if constexpr (has_std_get_v<raw_t>)
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>)
		{
			if constexpr (std::is_lvalue_reference_v<TUPLE &&>)
			{
				return std::forward_as_tuple(std::get<Is>(std::forward<TUPLE>(_tuple_obj))...);
			}
			else
			{
				return std::make_tuple(std::get<Is>(std::forward<TUPLE>(_tuple_obj))...);
			}
		}(std::make_index_sequence<std::tuple_size_v<raw_t>>{});
	}
	else
	{
		if constexpr (std::is_rvalue_reference_v<TUPLE &&>)
		{
			return std::apply(
				[](auto &&...xs)
				{
					return std::make_tuple(std::forward<decltype(xs)>(xs)...);
				},
				detail::_to_tuple(std::forward<TUPLE>(_tuple_obj)));
		}
		else
		{
			return detail::_to_tuple(std::forward<TUPLE>(_tuple_obj));
		}
	}
}

/**
 * @brief 将广义 tuples 对象转换为标准元组。
 *
 * 该函数会先把任意 tuple-like 或可转换聚合对象统一送入 `to_tuple()`，
 * 再在外层消除引用限定符并返回一个真正的标准 `std::tuple`。
 *
 * 这样可以让后续调用站在统一的标准元组接口上工作，减少各类类型分支的干扰。
 *
 * @note 返回类型是 `decltype(auto)`：若 `to_tuple()` 的结果已经是拥有值的标准 tuple，
 * 会直接转发；若结果是引用元组，则会先把元素拷贝或移动进新构造的值 tuple。
 * 无论哪种情况，结果都可以按标准 tuple 语义使用。
 */
template <typename TUPLE>
constexpr decltype(auto) make_to_tuple(TUPLE &&_tuple_obj)
{
	using tuple_result = decltype(to_tuple(std::declval<TUPLE &&>()));
	using tuple_type = remove_cvref_t<tuple_result>;
	constexpr bool has_only_value_elements = []<size_t... Is>(std::index_sequence<Is...>)
	{
		return (... && !std::is_reference_v<std::tuple_element_t<Is, tuple_type>>);
	}(std::make_index_sequence<std::tuple_size_v<tuple_type>>{});

	if constexpr (!std::is_reference_v<tuple_result> && has_only_value_elements)
	{
		return to_tuple(std::forward<TUPLE>(_tuple_obj));
	}
	else
	{
		return std::apply(
			[](auto &&...xs)
			{
				return std::make_tuple(std::forward<decltype(xs)>(xs)...);
			},
			to_tuple(std::forward<TUPLE>(_tuple_obj)));
	}
}

/**
 * @brief 适用于所有广义 tuples 对象的 apply 函数。
 *
 * 该封装本质上是对 `std::apply` 的统一入口：它首先将输入对象规范化为标准 tuple，
 * 然后再把函数对象应用到该参数包上。
 *
 * 这样可以让 `TupleView`、`TupleInsertView`、聚合 tuple-like 类型以及普通 `std::tuple`
 * 共享同一套高层调用接口。
 */
template <typename FUNC, typename TUPLE>
constexpr decltype(auto) apply(FUNC &&visit, TUPLE &&_tuple_obj)
{
	return std::apply(std::forward<FUNC>(visit), to_tuple(std::forward<TUPLE>(_tuple_obj)));
}

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N, typename TUPLE>
constexpr auto _tuple_get_value(TUPLE &&_tuple_obj)
{
	using raw_type = remove_cvref_t<TUPLE>;
	if constexpr (is_tuple_view_v<raw_type>)
	{
		return std::forward<TUPLE>(_tuple_obj).template get<N>();
	}
	else if constexpr (has_std_get_v<raw_type>)
	{
		return std::get<N>(std::forward<TUPLE>(_tuple_obj));
	}
	else
	{
		return std::get<N>(detail::_to_tuple(std::forward<TUPLE>(_tuple_obj)));
	}
}

EMBMARTIN_DETAIL_NAMESPACE_END

// get 方法
template <size_t N, typename TUPLE>
constexpr decltype(auto) get(TUPLE &&_tuple_obj)
{
	using raw_type = remove_cvref_t<TUPLE>;
	if constexpr (!std::is_lvalue_reference_v<TUPLE &&>)
	{
		return detail::_tuple_get_value<N>(std::forward<TUPLE>(_tuple_obj));
	}
	else if constexpr (is_tuple_view_v<raw_type>)
	{
		return std::forward<TUPLE>(_tuple_obj).template get<N>();
	}
	else if constexpr (has_std_get_v<raw_type>)
	{
		return std::get<N>(std::forward<TUPLE>(_tuple_obj));
	}
	else
	{
		return std::get<N>(detail::_to_tuple(std::forward<TUPLE>(_tuple_obj)));
	}
}

template <size_t N, typename Tuple, size_t... Is>
constexpr decltype(auto) get(TupleView<Tuple, std::index_sequence<Is...>> &view) noexcept
{
	return view.template get<N>();
}

template <size_t N, typename Tuple, size_t... Is>
constexpr decltype(auto) get(const TupleView<Tuple, std::index_sequence<Is...>> &view) noexcept
{
	return view.template get<N>();
}

template <size_t N, typename Tuple, size_t... Is>
constexpr auto get(TupleView<Tuple, std::index_sequence<Is...>> &&view) noexcept
{
	return std::move(view).template get<N>();
}

template <size_t N, typename Tuple, size_t... Is>
constexpr auto get(const TupleView<Tuple, std::index_sequence<Is...>> &&view) noexcept
{
	return std::move(view).template get<N>();
}

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
constexpr decltype(auto) get(TupleInsertView<Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>> &view) noexcept
{
	return view.template get<N>();
}

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
constexpr decltype(auto) get(const TupleInsertView<Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>> &view) noexcept
{
	return view.template get<N>();
}

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
constexpr auto get(TupleInsertView<Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>> &&view) noexcept
{
	return std::move(view).template get<N>();
}

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
constexpr auto get(const TupleInsertView<Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>> &&view) noexcept
{
	return std::move(view).template get<N>();
}

template <size_t N, typename Tuple, size_t PreN, size_t PostN>
constexpr decltype(auto) get(TupleUnpackView<Tuple, PreN, PostN> &view) noexcept
{
	return view.template get<N>();
}

template <size_t N, typename Tuple, size_t PreN, size_t PostN>
constexpr decltype(auto) get(const TupleUnpackView<Tuple, PreN, PostN> &view) noexcept
{
	return view.template get<N>();
}

template <size_t N, typename Tuple, size_t PreN, size_t PostN>
constexpr auto get(TupleUnpackView<Tuple, PreN, PostN> &&view) noexcept
{
	return std::move(view).template get<N>();
}

template <size_t N, typename Tuple, size_t PreN, size_t PostN>
constexpr auto get(const TupleUnpackView<Tuple, PreN, PostN> &&view) noexcept
{
	return std::move(view).template get<N>();
}
//------------------------------------------python 风格广义元组解包 --------------------------------------------------

/**
 * @brief 将 tuple 拆分为“前置部分 + 中间剩余 + 后置部分”，返回三段式视图。
 *
 * 返回值是一个 `TupleUnpackView<storage_type, PRE, POST>`，其 `get<I>()` 布局为：
 *
 * - `get<0>` … `get<PRE-1>`：前置元素（引用底层元素）；
 *
 * - `get<PRE>`：中间剩余段的 `TupleView` 子视图；
 *
 * - `get<PRE+1>` … `get<PRE+POST>`：后置元素（引用底层元素）。
 *
 * 视图不拥有数据：左值输入时视图引用底层 tuple，右值输入时视图拥有 tuple。
 * 需要真正的值 tuple 时，请对视图调用 `materialize()` 或 `to_tuple()`。
 *
 * 该接口非常适合：
 *
 * - 处理可变参数元组；
 *
 * - 抽取首尾元素并保留中间内容；
 *
 * - 让泛化 tuple 语法更接近 Python 的 unpacking 习惯。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 2.0, 'x', 4);
 * auto v = unpack<1, 1>(t);              // 视图
 * auto [a, mid, z] = v;                  // a:int, mid:TupleView, z:int
 * auto values = v.materialize();         // std::tuple<int, std::tuple<double,char>, int>
 * ```
 */
template <size_t PRE, size_t POST, typename Tuple>
constexpr decltype(auto) unpack(Tuple &&t)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;
	static_assert(PRE + POST <= Size, "PRE + POST must not exceed tuple size");

	using storage_type = std::conditional_t<
		std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	return TupleUnpackView<storage_type, PRE, POST>(std::forward<Tuple>(t));
}

/**
 * @brief 解包出分立的前 N 个元素和剩余元素的视图。
 *
 * 用法：
 *
 * ```cpp
 * auto [a, b, rest] = unpack_pre<2>(tuple_obj);   // rest 是 TupleView
 * auto [all] = unpack_pre<0>(tuple_obj);          // all 是 TupleView（整段）
 * auto [a, b, empty] = unpack_pre<2>(two_member_tuple_obj);
 * ```
 *
 * 相当于：
 *
 * ```python
 * a, b, *rest = sequence_obj
 * *all = sequence_obj
 * a, b, *empty = two_member_sequence_obj
 * ```
 *
 * @note 返回的是视图；`rest` / `all` 是引用底层 tuple 的 `TupleView`，需要值 tuple
 * 时请对视图调用 `materialize()` 或 `to_tuple()`。
 */
template <size_t N, typename Tuple>
constexpr decltype(auto) unpack_pre(Tuple &&t)
{
	return unpack<N, 0>(std::forward<Tuple>(t));
}

/**
 * @brief 解包出第一个元素和剩余元素的视图。
 *
 * 用法：
 *
 * ```cpp
 * auto [first, rest] = unpack_first(tuple_obj);   // rest 是 TupleView
 * ```
 *
 * 相当于：
 *
 * ```python
 * a, *b = sequence_obj
 * ```
 *
 * @note 同 `unpack_pre`：返回视图；`rest` 引用底层 tuple 的中间段。
 */
template <typename Tuple>
constexpr decltype(auto) unpack_first(Tuple &&t)
{
	return unpack_pre<1>(std::forward<Tuple>(t));
}

/**
 * @brief 解包出分立的后 N 个元素和前面剩余元素的视图。
 *
 * 用法：
 *
 * ```cpp
 * auto [rest, a, b] = unpack_post<2>(tuple_obj);  // rest 是 TupleView
 * auto [all] = unpack_post<0>(tuple_obj);         // all 是 TupleView（整段）
 * auto [empty, a, b] = unpack_post<2>(two_member_tuple_obj);
 * ```
 *
 * 相当于：
 *
 * ```python
 * *rest, a, b = sequence_obj
 * *all = sequence_obj
 * *empty, a, b = sequence_obj
 * ```
 *
 * @note 同 `unpack_pre`：返回视图；`rest` / `all` 引用底层 tuple 的中间段。
 */
template <size_t N, typename Tuple>
constexpr decltype(auto) unpack_post(Tuple &&t)
{
	return unpack<0, N>(std::forward<Tuple>(t));
}

/**
 * @brief 解包出后一个元素和前面剩余元素的视图。
 *
 * 用法：
 *
 * ```cpp
 * auto [rest, a] = unpack_last(tuple_obj);        // rest 是 TupleView
 * ```
 *
 * 相当于：
 *
 * ```python
 * *rest, a = sequence_obj
 * ```
 *
 * @note 同 `unpack_pre`：返回视图；`rest` 引用底层 tuple 的中间段。
 */
template <typename Tuple>
constexpr decltype(auto) unpack_last(Tuple &&t)
{
	return unpack_post<1>(std::forward<Tuple>(t));
}

//------------------------------------------ 广义元组操作 --------------------------------------------------

/**
 * @brief 对 tuple 进行编译期切片，返回新的 tuple view。
 *
 * 语义与 Python 的 list slicing 相近，但它工作在编译期索引和 tuple 视图层面，
 * 具有较低运行时成本。
 *
 * 支持：
 *
 * - `split<Start, End>`：普通正向切片；
 *
 * - `split<Start, End, Step>`：带步长；
 *
 * - 支持负步长；若步长方向与 Start / End 不匹配，则返回空视图。
 *
 * 该函数主要用于：
 *
 * - 抽取 tuple 的某一段；
 *
 * - 在管道中裁剪参数包；
 *
 * - 作为组合其他 tuple 操作的底层基础。
 *
 * @details
 * 这里的切片并不立即复制元素，而是生成一个新的 `TupleView`，因此可以在编译期保留
 * 原 tuple 的访问映射关系，并在后续操作中继续组合、插入、删去或交换元素。
 *
 * @tparam Start 起始索引，包含在结果中。
 * @tparam End 结束索引，通常为半开区间的右端点。
 * @tparam Step 步长，允许正向或负向切片。
 * @tparam Tuple 满足广义 tuple 概念的类型，包括标准库 tuple-like 类型与可解包聚合类型。
 *
 * @param t 要切片的 tuple 对象。
 * @return 返回一个新的 tuple 视图，按要求保留选定元素顺序。
 *
 * @note 该接口是后续 `insert` / `erase` / `swap` 等操作的底层基础，适合在元组处理链中
 * 维持低成本、无临时拷贝的计算风格。
 *
 * ```cpp
 * auto t = std::make_tuple(10, 20, 30, 40, 50);
 * auto v = split<1, 4>(t);                  // {20, 30, 40}
 * auto w = split<4, 1, -1>(t);              // {50, 40, 30}
 * auto x = split<0, 5, 2>(t);               // {10, 30, 50}
 * ```
 */
template <size_t Start, size_t End, ::std::make_signed_t<size_t> Step, typename Tuple,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) split(Tuple &&t)
{
	static_assert(Step != 0);

	if constexpr ((End <= Start && Step < 0) || (End >= Start && Step > 0))
	{
		constexpr size_t distance = Step > 0 ? End - Start : Start - End;
		constexpr size_t step = Step > 0 ? size_t(Step) : size_t(-Step);
		constexpr size_t count = (distance + step - 1) / step;
		return detail::_make_split_view<Start, Step>(
			std::forward<Tuple>(t), std::make_index_sequence<count>{});
	}
	else
	{
		return detail::_make_split_view<0, 1>(
			std::forward<Tuple>(t), std::index_sequence<>{});
	}
}

/**
 * @brief 对 tuple 做普通正向切片。
 *
 * 这是 `split<Start, End, Step>` 的简化重载，默认步长为 1。
 *
 * @tparam Start 起始索引。
 * @tparam End 结束索引，前闭后开。
 * @tparam Tuple 满足广义 tuple 概念的类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @param t 要切片的 tuple 对象。
 * @return 返回满足 `[Start, End)` 范围的切片视图。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 2, 3, 4, 5);
 * auto a = split<1, 4>(t);  // {2, 3, 4}
 * ```
 */
template <size_t Start, size_t End, typename Tuple,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) split(Tuple &&t)
{
	return split<Start, End, 1>(std::forward<Tuple>(t));
}

/**
 * @brief 对 tuple 做从 0 到 End 的前缀切片。
 *
 * 它等价于 `split<0, End>`，常用于提取开头若干元素而不再手动写起点。
 *
 * @tparam End 结束索引。
 * @tparam Tuple 满足广义 tuple 概念的类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @param t 要切片的 tuple 对象。
 * @return 返回前缀切片视图。
 *
 * ```cpp
 * auto t = std::make_tuple('a', 'b', 'c', 'd');
 * auto prefix = split<2>(t);  // {'a', 'b'}
 * ```
 */
template <size_t End, typename Tuple,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) split(Tuple &&t)
{
	return split<0, End>(::std::forward<Tuple>(t));
}

/**
 * @brief 在 tuple 的指定位置插入一个或多个元素。
 *
 * 作用类似 `std::tuple` 的插入，但该接口更偏“视图级重排”。
 *
 * 结果保留原 tuple 的布局，并将新增元素按索引插入。
 *
 * 适用于：
 *
 * - 在参数包前后加入节点；
 *
 * - 以编译期索引重构 tuple；
 *
 * - 让管道式参数处理更加低成本。
 *
 * @tparam N 插入位置，范围为 `[0, tuple_size]`。
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @tparam Ty... 插入元素的类型列表。
 * @param t 原 tuple 对象。
 * @param val... 要插入的新元素。
 * @return 返回一个新的 tuple 视图，其中插入元素位于 `N` 位置。
 *
 * @note 该函数并不强制立即创建一份完整复制；它通过 tuple view 形式保留源数据布局，
 * 便于后续继续调用 `split` / `erase` / `swap` 等操作。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 2, 4);
 * auto x = insert<2>(t, 3);  // {1, 2, 3, 4}
 * ```
 */
template <size_t N, typename Tuple, typename... Ty,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) insert(Tuple &&t, Ty &&...val)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;
	static_assert(N <= Size, "Tuple insert index out of range");

	return detail::_make_insert_view<N>(
		::std::forward<Tuple>(t),
		::std::make_tuple(::std::forward<Ty>(val)...));
}

/**
 * @brief 在 tuple 尾部追加一个或多个元素。
 *
 * 这是 `insert<Size>` 的语义化封装，适合在后端追加参数或输出值。
 *
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @tparam Ty... 要追加的元素类型列表。
 * @param t 原 tuple 对象。
 * @param val... 要追加的新值。
 * @return 返回追加后的新 tuple 视图。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 2);
 * auto x = push_back(t, 3, 4);  // {1, 2, 3, 4}
 * ```
 */
template <typename Tuple, typename... Ty,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) push_back(Tuple &&t, Ty &&...val)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;

	return insert<Size>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

/**
 * @brief 在 tuple 头部插入一个或多个元素。
 *
 * 这是 `insert<0>` 的语义化封装，适合用于前置扩展参数包。
 *
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @tparam Ty... 要插入元素的类型列表。
 * @param t 原 tuple 对象。
 * @param val... 要插入的新值。
 * @return 返回头部插入后的新 tuple 视图。
 *
 * ```cpp
 * auto t = std::make_tuple(2, 3);
 * auto x = push_front(t, 1);  // {1, 2, 3}
 * ```
 */
template <typename Tuple, typename... Ty,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) push_front(Tuple &&t, Ty &&...val)
{
	return insert<0>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

/**
 * @brief 在指定位置插入一个广义 tuple 对象。
 *
 * 它与 `insert` 的区别在于：这里的 `val` 本身就是一个标准库 tuple-like 或可解包聚合对象，
 * 会先标准化为真实 tuple，再按 `N` 深度插入。
 *
 * @tparam N 插入位置。
 * @tparam TupleA 原广义 tuple 类型。
 * @tparam TupleB 要插入的广义 tuple 类型。
 * @param t 原 tuple。
 * @param val 要插入的 tuple 对象。
 * @return 返回插入后的新 tuple 视图。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 4);
 * auto extra = std::make_tuple(2, 3);
 * auto x = insert_tuple<1>(t, extra);  // {1, 2, 3, 4}
 * ```
 */
template <size_t N, typename TupleA, typename TupleB,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<TupleA>>>>
constexpr decltype(auto) insert_tuple(TupleA &&t, TupleB &&val)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<TupleA>>;
	static_assert(N <= Size, "Tuple insert index out of range");

	return detail::_make_insert_view<N>(
		::std::forward<TupleA>(t),
		to_tuple(::std::forward<TupleB>(val)));
}

/**
 * @brief 在 tuple 尾部追加另一个 tuple。
 *
 * 等价于 `insert_tuple<tuple_size>`。
 *
 * @tparam TupleA 原广义 tuple 类型。
 * @tparam TupleB 要追加的广义 tuple 类型。
 * @param t 原 tuple 对象。
 * @param val 要追加的广义 tuple 对象。
 * @return 返回追加后的新 tuple 视图。
 *
 * ```cpp
 * auto left = std::make_tuple(1, 2);
 * auto right = std::make_tuple(3, 4);
 * auto x = append_tuple(left, right);  // {1, 2, 3, 4}
 * ```
 */
template <typename TupleA, typename TupleB,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<TupleA>>>>
constexpr decltype(auto) append_tuple(TupleA &&t, TupleB &&val)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<TupleA>>;

	return insert_tuple<Size>(::std::forward<TupleA>(t), ::std::forward<TupleB>(val));
}

/**
 * @brief 在 tuple 头部前置另一个 tuple。
 *
 * 等价于 `insert_tuple<0>`，适合把前缀参数或结果段拼进当前参数包。
 *
 * @tparam TupleA 原广义 tuple 类型。
 * @tparam TupleB 要前置的广义 tuple 类型。
 * @param t 原 tuple 对象。
 * @param val 要前置的广义 tuple 对象。
 * @return 返回前置后的新 tuple 视图。
 *
 * ```cpp
 * auto s = std::make_tuple(2, 3);
 * auto x = prepend_tuple(s, std::make_tuple(1));  // {1, 2, 3}
 * ```
 */
template <typename TupleA, typename TupleB,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<TupleA>>>>
constexpr decltype(auto) prepend_tuple(TupleA &&t, TupleB &&val)
{
	return insert_tuple<0>(::std::forward<TupleA>(t), ::std::forward<TupleB>(val));
}

/**
 * @brief 在指定位置插入一个由单个值构造/转换得到的 `T`。
 *
 * 参数会被转发给内部 `std::tuple<T>` 的构造，而 `std::tuple<T>` 只接受一个可转换为
 * `T` 的实参，所以这不是标准容器意义上的多参数 `emplace`：
 *
 * - `emplace<1, std::pair<int, int>>(t, 2, 4)` 无法编译；
 *
 * - 需要插入复杂类型时，请先构造好 `T` 再传入，例如
 *   `emplace<1, std::pair<int, int>>(t, std::pair<int, int>{2, 4})`。
 *
 * 若需要一次插入多个元素，请使用 `insert<N>`；若要插入一个已构造好的广义 tuple，
 * 请使用 `insert_tuple<N>`。
 *
 * @tparam N 插入位置。
 * @tparam T 要插入的元素类型。
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @tparam Ty... 构造 `T` 所用的参数类型列表。
 * @param t 原 tuple 对象。
 * @param val... 用于构造新元素的参数。
 * @return 返回带新元素的 tuple 视图。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 3);
 * auto x = emplace<1, std::pair<int, int>>(t, std::pair<int, int>{2, 4});  // {1, {2, 4}, 3}
 * ```
 */
template <size_t N, typename T, typename Tuple, typename... Ty,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) emplace(Tuple &&t, Ty &&...val)
{
	return detail::_make_emplace_view<N, T>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

/**
 * @brief 构造一个元素并追加到尾部。
 *
 * 等价于 `emplace<tuple_size, T>`。
 *
 * @tparam T 要构造的元素类型。
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @tparam Ty... 构造参数类型列表。
 * @param t 原 tuple 对象。
 * @param val... 构造参数。
 * @return 返回追加后的 tuple 视图。
 */
template <typename T, typename Tuple, typename... Ty,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) emplace_back(Tuple &&t, Ty &&...val)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;

	return emplace<Size, T>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

/**
 * @brief 构造一个元素并前置到头部。
 *
 * 等价于 `emplace<0, T>`，适合在参数包前插入新节点。
 *
 * @tparam T 要构造的元素类型。
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @tparam Ty... 构造参数类型列表。
 * @param t 原 tuple 对象。
 * @param val... 构造参数。
 * @return 返回前置后的 tuple 视图。
 */
template <typename T, typename Tuple, typename... Ty,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) emplace_front(Tuple &&t, Ty &&...val)
{
	return emplace<0, T>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

/**
 * @brief 删除位于索引 N 的元素。
 *
 * 它会把前半段 tuple 与后半段 tuple 重新组合，得到一个去掉指定元素的新视图。
 *
 * @tparam N 要删除的位置。
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @param t 原 tuple 对象。
 * @return 返回删除一个元素后的新 tuple 视图。
 *
 * @note 索引在编译期检查（要求 `N < tuple_size`）；空 tuple 上调用 `pop_back` /
 * `pop_front` 会因此触发静态断言。
 *
 * ```cpp
 * auto t = std::make_tuple(10, 20, 30, 40);
 * auto x = erase<1>(t);  // {10, 30, 40}
 * ```
 */
template <size_t N, typename Tuple,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) erase(Tuple &&t)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;
	static_assert(N < Size, "Tuple erase index out of range");
	return detail::_make_erase_view<N>(
		::std::forward<Tuple>(t), std::make_index_sequence<Size - 1>{});
}

/**
 * @brief 删除尾部元素。
 *
 * 等价于 `erase<tuple_size - 1>`。
 *
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @param t 原 tuple 对象。
 * @return 返回尾部元素已删除的新 tuple 视图。
 *
 * @note 编译期要求 tuple 非空；空 tuple 会触发 `erase<tuple_size - 1>` 的静态断言。
 */
template <typename Tuple,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) pop_back(Tuple &&t)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;

	return erase<Size - 1>(::std::forward<Tuple>(t));
}

/**
 * @brief 删除头部元素。
 *
 * 等价于 `erase<0>`。
 *
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @param t 原 tuple 对象。
 * @return 返回头部元素已删除的新 tuple 视图。
 *
 * @note 编译期要求 tuple 非空；空 tuple 会触发 `erase<0>` 的静态断言。
 */
template <typename Tuple,
		  typename = ::std::enable_if_t<is_generalized_tuple_v<remove_cvref_t<Tuple>>>>
constexpr decltype(auto) pop_front(Tuple &&t)
{
	return erase<0>(::std::forward<Tuple>(t));
}

/**
 * @brief 交换 tuple 中两个位置上的元素。
 *
 * 它返回一个新的顺序重排后的 `TupleView`，而不是直接修改原 tuple。
 *
 * @tparam N1 第一个索引。
 * @tparam N2 第二个索引。
 * @tparam Tuple 原广义 tuple 类型，包括标准库 tuple-like 类型与可解包聚合类型。
 * @param t 原 tuple 对象。
 * @note 索引在编译期检查（要求 `N1`、`N2` 均小于 `tuple_size`）。
 *
 * @return 返回交换后元素顺序的新 tuple 视图。
 *
 * ```cpp
 * auto t = std::make_tuple(1, 2, 3, 4);
 * auto x = swap<1, 3>(t);  // {1, 4, 3, 2}
 * ```
 */
template <size_t N1, size_t N2, typename Tuple>
constexpr decltype(auto) swap(Tuple &&t)
{
	constexpr size_t Size = generalized_tuple_size_v<remove_cvref_t<Tuple>>;
	static_assert(N1 < Size && N2 < Size, "N must be less than tuple size");

	constexpr size_t MinN = (N1 <= N2) ? N1 : N2;
	constexpr size_t MaxN = (N1 > N2) ? N1 : N2;
	return detail::_make_swap_view<MinN, MaxN>(
		std::forward<Tuple>(t), std::make_index_sequence<Size>{});
}


EMBMARTIN_NAMESPACE_END
