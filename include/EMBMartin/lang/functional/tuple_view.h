/**
 ******************************************************************************
 * @file    tuple_view.h
 * @brief   广义 tuple 视图：TupleView / TupleInsertView / TupleUnpackView 及索引元编程
 ******************************************************************************
 */

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "EMBMartin/macro.h"
#include "EMBMartin/lang/meta.h"

EMBMARTIN_LANG_NAMESPACE_BEGIN

//------------------------------------------ 广义 tuples 基础操作 --------------------------------------------------

/**
 * @brief 对原始 tuple 的“视图”包装器。
 *
 * `TupleView` 可包装标准库 tuple-like 类型或可解包聚合类型；对左值保留引用、对右值持有对象，
 * 并通过索引映射访问其中的元素。
 *
 * 它常用于：
 *
 * - 选取一部分 tuple（如 split / swap / erase 的中间结果）；
 *
 * - 组合多个 tuple 视图而不立即复制内存；
 *
 * - 对标准库 tuple-like 类型与可解包聚合类型提供统一访问协议。
 *
 * `Is...` 记录的是实际访问顺序映射到原 tuple 的索引值，允许从任意顺序重排元素。
 */
template <typename Tuple, typename IndexSequence>
class TupleView;

/**
 * @brief 在指定位置插入额外元素的 tuple 视图。
 *
 * 这是 `TupleView` 的扩展版本。
 *
 * 它将原 tuple-like / 可解包聚合对象与一个额外的 `std::tuple<ExtraTs...>` 视为一个逻辑上的 larger tuple：
 *
 * - 前 `InsertPos` 个元素来自原 tuple；
 *
 * - 后续插入元素来自 `extra`；
 *
 * - 其余元素继续回到原 tuple。
 *
 * 这种结构使得 `insert` / `prepend_tuple` / `append_tuple` 等操作能在编译期稳定地
 * 生成新视图，而不需要立即创建大量临时元组。
 */
template <typename Tuple, typename IndexSequence, size_t InsertPos, typename ExtraTuple>
class TupleInsertView;

/**
 * @brief Python 风格广义元组解包的视图实现。
 *
 * 把底层 tuple 分成「`PreN` 个前置元素 + 中间剩余段 + `PostN` 个后置元素」：
 *
 * - `get<0>` … `get<PreN-1>`：前置元素（直接引用底层元素）；
 *
 * - `get<PreN>`：中间剩余段的 `TupleView` 子视图（不是元素值）；
 *
 * - `get<PreN+1>` … `get<PreN+PostN>`：后置元素（直接引用底层元素）。
 *
 * `Tuple` 为左值引用（`T&` / `const T&`）时是非拥有视图，为值类型时视图持有对象。
 *
 * @tparam Tuple 底层广义 tuple 的储存类型。
 * @tparam PreN 前置元素个数。
 * @tparam PostN 后置元素个数。
 */
template <typename Tuple, size_t PreN, size_t PostN>
class TupleUnpackView;

template <typename T>
struct is_tuple_view : std::false_type
{
};

template <typename Tuple, size_t... Is>
struct is_tuple_view<TupleView<Tuple, std::index_sequence<Is...>>> : std::true_type
{
};

template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct is_tuple_view<TupleInsertView<Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>>> : std::true_type
{
};

template <typename Tuple, size_t PreN, size_t PostN>
struct is_tuple_view<TupleUnpackView<Tuple, PreN, PostN>> : std::true_type
{
};

template <typename T>
constexpr bool is_tuple_view_v = is_tuple_view<remove_cvref_t<T>>::value;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t I, size_t... Rest>
struct _tuple_view_index;

template <size_t I, size_t First, size_t... Rest>
struct _tuple_view_index<I, First, Rest...> : _tuple_view_index<I - 1, Rest...>
{
};

template <size_t First, size_t... Rest>
struct _tuple_view_index<0, First, Rest...> : std::integral_constant<size_t, First>
{
};

template <size_t I>
struct _tuple_view_index<I> : std::integral_constant<size_t, I>
{
	static_assert(I == 0, "TupleView index out of range for empty view");
};

template <typename Tuple, size_t... Is>
constexpr auto _make_full_tuple_view(Tuple &&tuple, std::index_sequence<Is...>)
{
	using storage_type = std::conditional_t<std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	return TupleView<storage_type, std::index_sequence<Is...>>(
		std::forward<Tuple>(tuple));
}

template <size_t Start, std::make_signed_t<size_t> Step, typename Tuple, size_t... Is>
constexpr auto _make_split_view(Tuple &&tuple, std::index_sequence<Is...>)
{
	using storage_type = std::conditional_t<std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	using index_sequence = std::index_sequence<
		(Step > 0 ? Start + Is * static_cast<size_t>(Step)
				  : Start - Is * static_cast<size_t>(-Step))...>;
	return TupleView<storage_type, index_sequence>(std::forward<Tuple>(tuple));
}

template <typename Storage, typename Value>
constexpr decltype(auto) _tuple_forward(Value &&value) noexcept
{
	if constexpr (std::is_lvalue_reference_v<Storage>)
	{
		return (value);
	}
	else
	{
		return std::forward<Value>(value);
	}
}

template <size_t I, typename Tuple>
constexpr decltype(auto) _tuple_get(Tuple &&tuple)
{
	using raw_type = remove_cvref_t<Tuple>;
	if constexpr (is_tuple_view_v<raw_type>)
	{
		return std::forward<Tuple>(tuple).template get<I>();
	}
	else if constexpr (has_std_get_v<raw_type>)
	{
		return std::get<I>(std::forward<Tuple>(tuple));
	}
	else
	{
		return std::get<I>(detail::_to_tuple(std::forward<Tuple>(tuple)));
	}
}

template <size_t I, size_t Removed>
	struct _tuple_erase_index : std::integral_constant < size_t,
	I<Removed ? I : I + 1>
{
};

template <size_t N, typename Tuple, size_t... Is>
constexpr auto _make_erase_view(Tuple &&tuple, std::index_sequence<Is...>)
{
	using storage_type = std::conditional_t<std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	using index_sequence = std::index_sequence<_tuple_erase_index<Is, N>::value...>;
	return TupleView<storage_type, index_sequence>(std::forward<Tuple>(tuple));
}

template <size_t N1, size_t N2, typename Tuple, size_t... Is>
constexpr auto _make_swap_view(Tuple &&tuple, std::index_sequence<Is...>)
{
	using storage_type = std::conditional_t<std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	using index_sequence = std::index_sequence<index_swapper_v<Is, N1, N2>...>;
	return TupleView<storage_type, index_sequence>(std::forward<Tuple>(tuple));
}

template <typename Seq, size_t Offset>
struct offset_index_sequence_impl;

template <size_t... Is, size_t Offset>
struct offset_index_sequence_impl<std::index_sequence<Is...>, Offset>
{
	using type = std::index_sequence<(Is + Offset)...>;
};

template <size_t Offset, size_t N>
using make_offset_index_sequence =
	typename offset_index_sequence_impl<std::make_index_sequence<N>, Offset>::type;

EMBMARTIN_DETAIL_NAMESPACE_END

template <typename Tuple, size_t... Is>
class TupleView<Tuple, std::index_sequence<Is...>>
{
private:
	Tuple __tuple;

	template <size_t I>
	using source_index = detail::_tuple_view_index<I, Is...>;

public:
	/*
	 * 生命周期与值类别（视图语义）：
	 *
	 * 1. 生命周期：`Tuple` 为左值引用（`T&` / `const T&`）时视图不拥有数据，只要底层 tuple 仍存活
	 *    使用即安全；`Tuple` 为值类型时视图自己持有该对象。两种情况都不得让 `get` 返回的引用 /
	 *    子视图比底层对象活得更久。
	 *
	 * 2. 值类别：`get() &` 返回 `T&`；`get() const &` 返回 `const T&`（引用成员的 const 性不会
	 *    自动传播，故内部显式套 `std::as_const`）；`get() const &&` 返回 `const T&`。
	 *    右值 `get() &&` 只在 `Tuple` 为值类型时把元素转成 `T&&`（可被移动）；`Tuple` 为 `T&`
	 *    时经 `detail::_tuple_forward` 退化为 `T&`，**不会**搬空底层 tuple 的元素。
	 *
	 * 3. 因此 `materialize() &&` 对拥有型视图是移动元素，对引用型视图是拷贝元素。
	 */
	static constexpr size_t size = sizeof...(Is);

	template <typename SOURCE, typename = std::enable_if_t<
								   !std::is_same_v<std::decay_t<SOURCE>, TupleView> &&
								   std::is_constructible_v<Tuple, SOURCE &&> &&
								   // 引用型储存必须绑定左值，否则视图会指向已销毁的临时对象
								   !(std::is_lvalue_reference_v<Tuple> && !std::is_lvalue_reference_v<SOURCE &&>)>>
	explicit constexpr TupleView(SOURCE &&tuple) noexcept(std::is_nothrow_constructible_v<Tuple, SOURCE &&>)
		: __tuple(std::forward<SOURCE>(tuple))
	{
	}

	template <size_t I>
	constexpr decltype(auto) get() & noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(__tuple);
	}

	template <size_t I>
	constexpr decltype(auto) get() const & noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(std::as_const(__tuple));
	}

	template <size_t I>
	constexpr decltype(auto) get() && noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(detail::_tuple_forward<Tuple>(std::move(__tuple)));
	}

	template <size_t I>
	constexpr decltype(auto) get() const && noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(detail::_tuple_forward<Tuple>(std::as_const(__tuple)));
	}

	template <size_t... Js>
	constexpr auto materialize(std::index_sequence<Js...>) &
	{
		return std::make_tuple(get<Js>()...);
	}

	template <size_t... Js>
	constexpr auto materialize(std::index_sequence<Js...>) const &
	{
		return std::make_tuple(get<Js>()...);
	}

	template <size_t... Js>
	constexpr auto materialize(std::index_sequence<Js...>) &&
	{
		return std::make_tuple(std::move(*this).template get<Js>()...);
	}

	constexpr auto materialize() &
	{
		return materialize(std::make_index_sequence<size>{});
	}

	constexpr auto materialize() const &
	{
		return materialize(std::make_index_sequence<size>{});
	}

	constexpr auto materialize() &&
	{
		return std::move(*this).materialize(std::make_index_sequence<size>{});
	}
};

template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
class TupleInsertView<Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>>
{
private:
	Tuple __tuple;
	std::tuple<ExtraTs...> _extra;

public:
	/*
	 * 生命周期与值类别（视图语义）：
	 *
	 * 1. 生命周期：`Tuple` 为左值引用（`T&` / `const T&`）时视图不拥有底层数据，只要底层
	 *    tuple 仍存活使用即安全；`Tuple` 为值类型时视图自己持有该对象。`_extra` 始终是值类型
	 *    `std::tuple<ExtraTs...>`，由视图自己持有。两种情况都不得让 `get` 返回的引用 /
	 *    子视图比底层对象活得更久。
	 *
	 * 2. 值类别：`get() &` 返回 `T&`；`get() const &` 返回 `const T&`（引用成员的 const 性
	 *    不会自动传播，故内部显式套 `std::as_const`）；`get() const &&` 返回 `const T&`。
	 *    插入元素来自值类型 `_extra`：`get() &&` 返回可移动的 `ExtraTs&&`，`get() const &&`
	 *    返回 `const ExtraTs&`。右值 `get() &&` 只在 `Tuple` 为值类型时把原 tuple 元素转成
	 *    `T&&`（可被移动）；`Tuple` 为 `T&` 时经 `detail::_tuple_forward` 退化为 `T&`，
	 *    **不会**搬空底层 tuple 的元素。
	 *
	 * 3. 因此 `materialize() &&` 对拥有型视图是移动元素，对引用型视图是拷贝元素。
	 */
	static constexpr size_t size = sizeof...(Is) + sizeof...(ExtraTs);

	template <typename SOURCE, typename EXTRA,
			  typename = std::enable_if_t<std::is_constructible_v<Tuple, SOURCE &&> &&
										  std::is_constructible_v<std::tuple<ExtraTs...>, EXTRA &&>>>
	explicit constexpr TupleInsertView(SOURCE &&tuple, EXTRA &&extra) noexcept(
		std::is_nothrow_constructible_v<Tuple, SOURCE &&> &&
		std::is_nothrow_constructible_v<std::tuple<ExtraTs...>, EXTRA &&>)
		: __tuple(std::forward<SOURCE>(tuple)), _extra(std::forward<EXTRA>(extra))
	{
	}

	template <typename SOURCE, typename... VALUES,
			  typename = std::enable_if_t<std::is_constructible_v<Tuple, SOURCE &&> &&
										  std::is_constructible_v<std::tuple<ExtraTs...>, VALUES &&...>>>
	explicit constexpr TupleInsertView(std::in_place_t, SOURCE &&tuple, VALUES &&...values) noexcept(
		std::is_nothrow_constructible_v<Tuple, SOURCE &&> &&
		std::is_nothrow_constructible_v<std::tuple<ExtraTs...>, VALUES &&...>)
		: __tuple(std::forward<SOURCE>(tuple)), _extra(std::forward<VALUES>(values)...)
	{
	}

	template <size_t I>
	constexpr decltype(auto) get() & noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(__tuple);
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(_extra);
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(__tuple);
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const & noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(std::as_const(__tuple));
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(_extra);
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(std::as_const(__tuple));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() && noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(detail::_tuple_forward<Tuple>(std::move(__tuple)));
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(std::move(_extra));
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(detail::_tuple_forward<Tuple>(std::move(__tuple)));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const && noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(detail::_tuple_forward<Tuple>(std::as_const(__tuple)));
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(std::move(_extra));
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(detail::_tuple_forward<Tuple>(std::as_const(__tuple)));
		}
	}

	template <size_t... Js>
	constexpr auto materialize(std::index_sequence<Js...>) &
	{
		return std::make_tuple(get<Js>()...);
	}

	template <size_t... Js>
	constexpr auto materialize(std::index_sequence<Js...>) const &
	{
		return std::make_tuple(get<Js>()...);
	}

	template <size_t... Js>
	constexpr auto materialize(std::index_sequence<Js...>) &&
	{
		return std::make_tuple(std::move(*this).template get<Js>()...);
	}

	constexpr auto materialize() &
	{
		return materialize(std::make_index_sequence<size>{});
	}

	constexpr auto materialize() const &
	{
		return materialize(std::make_index_sequence<size>{});
	}

	constexpr auto materialize() &&
	{
		return std::move(*this).materialize(std::make_index_sequence<size>{});
	}
};

template <typename Tuple, size_t PreN, size_t PostN>
class TupleUnpackView
{
private:
	static constexpr size_t total_size =
		EMBMartin::lang::generalized_tuple_size_v<remove_cvref_t<Tuple>>;

	// 越界时先断言，并把 mid_size 退化为 0：否则 size_t 下溢出的天文数字会继续被
	// make_offset_index_sequence 拿去实例化，产生一串与真正原因无关的级联报错。
	static constexpr bool size_ok = (PreN + PostN <= total_size);

	static_assert(size_ok, "PreN + PostN must not exceed tuple size");

	static constexpr size_t mid_size = size_ok ? (total_size - PreN - PostN) : 0;

	// 中间段在原 tuple 中的索引：PreN, PreN+1, ..., total_size - PostN - 1
	using mid_index_seq = detail::make_offset_index_sequence<PreN, mid_size>;

	// 中间段视图类型
	using mid_view_type = TupleView<Tuple &, mid_index_seq>;
	using const_mid_view_type = TupleView<
		std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<Tuple>>>,
		mid_index_seq>;

	Tuple __tuple;

	// 把前段 / 中间段 / 后段按 Python 解包结构物化成真实 tuple：
	// 前段与后段逐元素展开，中间段收进一个内层 std::tuple。
	template <typename SourceTuple, size_t... PreIs, size_t... MidIs, size_t... PostIs>
	static constexpr auto __materialize_impl(SourceTuple &&source,
											std::index_sequence<PreIs...>,
											std::index_sequence<MidIs...>,
											std::index_sequence<PostIs...>)
	{
		return std::make_tuple(
			detail::_tuple_get<PreIs>(std::forward<SourceTuple>(source))...,
			std::make_tuple(detail::_tuple_get<PreN + MidIs>(std::forward<SourceTuple>(source))...),
			detail::_tuple_get<total_size - PostN + PostIs>(std::forward<SourceTuple>(source))...);
	}

public:
	/*
	 * 生命周期与值类别（视图语义）：
	 *
	 * 1. 生命周期：`Tuple` 为左值引用（`T&` / `const T&`）时视图不拥有数据；`Tuple` 为值类型时
	 *    视图自己持有该对象。前段 / 后段 `get` 返回的是底层元素的引用，不得比底层 tuple 活得更久。
	 *
	 * 2. 中间段：`get<PreN>()` 返回的是指向底层 tuple 的 `TupleView` 子视图（不是元素值）；
	 *    对临时对象（或即将销毁的 `TupleUnpackView`）调用会立刻悬空。
	 *
	 * 3. 值类别（`get`）：`get() &` 返回 `T&`；`get() const &` 返回 `const T&`（引用成员的 const 性
	 *    不会自动传播，故内部显式套 `std::as_const`）；`get() &&` 在 `Tuple` 为值类型时移动前 / 后段
	 *    元素，在 `Tuple` 为 `T&` 时移动的是**原对象**的元素；中间段在 `&&` 下不移动元素（只返回
	 *    子视图），与前后段语义不同。
	 *
	 * 4. 值类别（`materialize`）：把前 / 中 / 后三段物化成真实 `std::tuple`——前段与后段逐元素展开，
	 *    中间段收进一个内层 `std::tuple`。这里与 `get()` 不同：`materialize() &&` 只在 `Tuple` 为值
	 *    类型时移动元素；`Tuple` 为 `T&` 时经 `detail::_tuple_forward` 退化为左值，**只拷贝、不搬空
	 *    原对象**；`&` / `const &` / `const &&` 一律拷贝元素。中间段在这两种情况下都由内层 `std::tuple`
	 *    收拢成值。
	 *
	 */
	static constexpr size_t size = PreN + PostN + 1;

	template <typename SOURCE,
			  typename = std::enable_if_t<
				  !std::is_same_v<std::decay_t<SOURCE>, TupleUnpackView> &&
				  std::is_constructible_v<Tuple, SOURCE &&> &&
				  // 引用型储存必须绑定左值，否则视图会指向已销毁的临时对象
				  !(std::is_lvalue_reference_v<Tuple> &&
					!std::is_lvalue_reference_v<SOURCE &&>)>>
	explicit constexpr TupleUnpackView(SOURCE &&tuple) noexcept(std::is_nothrow_constructible_v<Tuple, SOURCE &&>)
		: __tuple(std::forward<SOURCE>(tuple))
	{
	}

	template <size_t I>
	constexpr decltype(auto) get() & noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(__tuple);
		}
		else if constexpr (I == PreN)
		{
			return mid_view_type{__tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(__tuple);
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const & noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(std::as_const(__tuple));
		}
		else if constexpr (I == PreN)
		{
			return const_mid_view_type{__tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(
				std::as_const(__tuple));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() && noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(std::move(__tuple));
		}
		else if constexpr (I == PreN)
		{
			return mid_view_type{__tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(
				std::move(__tuple));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const && noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(std::as_const(__tuple));
		}
		else if constexpr (I == PreN)
		{
			return const_mid_view_type{__tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(
				std::as_const(__tuple));
		}
	}

	constexpr auto materialize() &
	{
		return __materialize_impl(__tuple,
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}

	constexpr auto materialize() const &
	{
		return __materialize_impl(std::as_const(__tuple),
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}

	constexpr auto materialize() &&
	{
		return __materialize_impl(detail::_tuple_forward<Tuple>(std::move(__tuple)),
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}

	constexpr auto materialize() const &&
	{
		return __materialize_impl(detail::_tuple_forward<Tuple>(std::as_const(__tuple)),
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}
};

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N, typename Tuple, typename ExtraTuple>
constexpr auto _make_insert_view(Tuple &&tuple, ExtraTuple &&extra)
{
	using storage_type = std::conditional_t<std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	using extra_type = remove_cvref_t<ExtraTuple>;
	return TupleInsertView<storage_type, std::make_index_sequence<generalized_tuple_size_v<remove_cvref_t<storage_type>>>, N, extra_type>(
		std::forward<Tuple>(tuple), std::forward<ExtraTuple>(extra));
}

template <size_t N, typename T, typename Tuple, typename... VALUES>
constexpr auto _make_emplace_view(Tuple &&tuple, VALUES &&...values)
{
	using storage_type = std::conditional_t<std::is_lvalue_reference_v<Tuple &&>, Tuple &&, remove_cvref_t<Tuple>>;
	using index_sequence = std::make_index_sequence<generalized_tuple_size_v<remove_cvref_t<storage_type>>>;
	return TupleInsertView<storage_type, index_sequence, N, std::tuple<T>>(
		std::in_place, std::forward<Tuple>(tuple), std::forward<VALUES>(values)...);
}

EMBMARTIN_DETAIL_NAMESPACE_END

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N, typename Tuple, typename IndexSequence, size_t InsertPos, typename ExtraTuple,
		  bool Before = (N < InsertPos),
		  bool InExtra = (N >= InsertPos && N < InsertPos + std::tuple_size_v<ExtraTuple>)>
struct _tuple_insert_element;

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct _tuple_insert_element<N, Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>, true, false>
{
	using type = EMBMartin::lang::generalized_tuple_element_t<_tuple_view_index<N, Is...>::value, EMBMartin::lang::remove_cvref_t<Tuple>>;
};

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct _tuple_insert_element<N, Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>, false, true>
{
	using type = std::tuple_element_t<N - InsertPos, std::tuple<ExtraTs...>>;
};

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct _tuple_insert_element<N, Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>, false, false>
{
	using type = EMBMartin::lang::generalized_tuple_element_t<
		_tuple_view_index<N - sizeof...(ExtraTs), Is...>::value, EMBMartin::lang::remove_cvref_t<Tuple>>;
};

// 推导 TupleUnpackView 的第 N 项类型；三个位置分别对应前置 / 中间子视图 / 后置。
// - Const 参数区分「视图本身是否 const」；False 用于非 const 视图，True 用于 const 视图。
// - 元素类型用 remove_reference_t<Tuple> 求，以保留底层存储自身的 const（Tuple = const T& 时）。
// - 中间段返回 TupleView 子视图，与 get<PreN>() 的返回类型逐字对应。
template <size_t N, typename Tuple, size_t PreN, size_t PostN, bool Const,
		  bool IsPre = (N < PreN), bool IsMiddle = (N == PreN)>
struct _tuple_unpack_view_element
{
	// 三个有效组合 (true,false) / (false,true) / (false,false) 由下面的偏特化覆盖；
	// (true,true) 在语义上不可能出现（N < PreN 与 N == PreN 互斥）。
};

// 前置：N < PreN
template <size_t N, typename Tuple, size_t PreN, size_t PostN, bool Const>
struct _tuple_unpack_view_element<N, Tuple, PreN, PostN, Const, true, false>
{
	using raw_ref = std::remove_reference_t<Tuple>;
	using element = EMBMartin::lang::generalized_tuple_element_t<N, raw_ref>;
	using type = std::conditional_t<Const, std::add_const_t<element>, element>;
};

// 中间：N == PreN —— 返回子视图类型
template <size_t N, typename Tuple, size_t PreN, size_t PostN, bool Const>
struct _tuple_unpack_view_element<N, Tuple, PreN, PostN, Const, false, true>
{
	static constexpr size_t total_size =
		EMBMartin::lang::generalized_tuple_size_v<EMBMartin::lang::remove_cvref_t<Tuple>>;
	static constexpr size_t mid_size =
		(total_size >= PreN + PostN) ? (total_size - PreN - PostN) : 0;
	using mid_seq = make_offset_index_sequence<PreN, mid_size>;

	// 非 const：Tuple&（引用折叠后与类内 mid_view_type 一致）
	// const：    add_lvalue_reference_t<add_const_t<remove_reference_t<Tuple>>>
	//            （与类内 const_mid_view_type 完全一致）
	using storage_ref = std::conditional_t<
		Const,
		std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<Tuple>>>,
		Tuple &>;

	using type = EMBMartin::lang::TupleView<storage_ref, mid_seq>;
};

// 后置：N > PreN
template <size_t N, typename Tuple, size_t PreN, size_t PostN, bool Const>
struct _tuple_unpack_view_element<N, Tuple, PreN, PostN, Const, false, false>
{
	using raw_ref = std::remove_reference_t<Tuple>;
	using element = EMBMartin::lang::generalized_tuple_element_t<
		EMBMartin::lang::generalized_tuple_size_v<EMBMartin::lang::remove_cvref_t<Tuple>> - PostN + (N - PreN - 1),
		raw_ref>;
	using type = std::conditional_t<Const, std::add_const_t<element>, element>;
};
EMBMARTIN_DETAIL_NAMESPACE_END

EMBMARTIN_LANG_NAMESPACE_END

//------------------------------------------ std 特化：支持 structured binding ----------
namespace std
{
	template <typename Tuple, size_t... Is>
	struct tuple_size<EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
		: integral_constant<size_t, sizeof...(Is)>
	{
	};
	template <typename Tuple, size_t... Is>
	struct tuple_size<const EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
		: tuple_size<EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
	};
	template <typename Tuple, size_t... Is>
	struct tuple_size<volatile EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
		: tuple_size<EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
	};
	template <typename Tuple, size_t... Is>
	struct tuple_size<const volatile EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
		: tuple_size<EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
	};

	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: integral_constant<size_t, sizeof...(Is) + sizeof...(ExtraTs)>
	{
	};
	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<const EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: tuple_size<EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
	};
	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<volatile EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: tuple_size<EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
	};
	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<const volatile EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: tuple_size<EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
	};

	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = EMBMartin::lang::generalized_tuple_element_t<
			EMBMartin::lang::detail::_tuple_view_index<N, Is...>::value, EMBMartin::lang::remove_cvref_t<Tuple>>;
	};
	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, const EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = add_const_t<typename tuple_element<N, EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, volatile EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = add_volatile_t<typename tuple_element<N, EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, const volatile EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = add_cv_t<typename tuple_element<N, EMBMartin::lang::TupleView<Tuple, index_sequence<Is...>>>::type>;
	};

	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = typename EMBMartin::lang::detail::_tuple_insert_element<
			N, Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>::type;
	};
	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, const EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = add_const_t<typename tuple_element<N, EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, volatile EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = add_volatile_t<typename tuple_element<N, EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, const volatile EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = add_cv_t<typename tuple_element<N, EMBMartin::lang::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>::type>;
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
		: integral_constant<size_t, PreN + PostN + 1>
	{
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<const EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_size<EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<volatile EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_size<EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<const volatile EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_size<EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
		using type = typename EMBMartin::lang::detail::_tuple_unpack_view_element<
			N, Tuple, PreN, PostN, false>::type;
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, const EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
		using type = typename EMBMartin::lang::detail::_tuple_unpack_view_element<
			N, Tuple, PreN, PostN, true>::type;
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, volatile EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_element<N, EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, const volatile EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_element<N, EMBMartin::lang::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

} // namespace std
