/**
 ******************************************************************************
 * @file    functional.h
 * @author  孙鸣淼
 * @brief   EMBMartin 的函数式编程库
 *
 * 本头文件提供了三类核心能力：
 * 1. Python 风格的 Slice / Range 语义，适合对序列区间做轻量级描述；
 * 2. 广义 tuple 视图与操作，统一的广义元组接口
 * 3. 管道式编程接口 `pipe`，允许像 Unix 管道一样串联函数调用，并按需解包返回值。
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17（库中使用了 lambda 的显式模板参数列表，属 C++20 语法；
 *    ARM Compiler 6 在 C++17 模式下以扩展接受，MSVC 需启用 /std:c++20）
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 ******************************************************************************
 */

#ifndef EMBMARTIN_FUNCTIONAL_PROGRAMMING_H
#define EMBMARTIN_FUNCTIONAL_PROGRAMMING_H

#include <tuple>
#include <utility>

#include "macro.h"
#include "meta.h"
#include "stream.h"

EMBMARTIN_NAMESPACE_BEGIN

//------------------------------------------python 风格 slice 和 range 对象 --------------------------------------------------

/**
 * @brief Python 风格半开区间切片描述器。
 *
 * `Slice` 允许以类似 Python 的语法描述一个区间：
 *
 * - `Slice(0, 10, 2)` 表示 [0, 10) 且步长为 2；
 *
 * - 允许正向或反向步进；
 *
 * - 提供带随机访问标签的迭代器接口，可直接被范围 for 使用。
 *
 * 该类型适用于对索引区间进行结构化描述，而不是直接存储元素本身。
 * 其 `start`/`stop`/`step` 语义与 Python 的 slicing 设计相近。索引本身是无符号值，
 * 负步长表示反向遍历，不提供 Python 的负索引归一化。
 *
 * 构造时会用 `EMBMARTIN_NON_BLOCKING_ASSERT` 做非阻塞检查：若断言未终止程序，
 * `step == 0` 或方向非法的区间仍会被接受，此时 `size()` 返回 `0`，
 * 不应再依赖 `operator[]` 或迭代器遍历的结果。
 *
 * @note 内部 `Iterator` 只提供随机访问标签与基本算术（`+`、`-`、`+=`、`-=`、
 * 前后置 `++`/`--` 及比较），可用于范围 for；但不保证完整满足 C++20
 * `std::random_access_iterator` 概念（缺少 `operator[]`、`n + it`、`it - it` 等）。
 */
template <typename T>
struct Slice
{
	static_assert(std::is_integral_v<T>);

private:
	using index_type = std::make_unsigned_t<T>;
	using step_type = std::make_signed_t<T>;

public:
	index_type start;
	index_type stop;
	step_type step;

	class Iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = index_type;
		using difference_type = std::ptrdiff_t;
		using pointer = void;
		using reference = index_type;

	private:
		const Slice &_slice;
		index_type _pos;

	public:
		constexpr inline Iterator(const Slice &slice, const index_type &pos = 0) noexcept : _slice{slice}, _pos{pos} {}
		constexpr inline auto operator*() const noexcept { return _slice[_pos]; }
		constexpr inline Iterator operator+(difference_type offset) const noexcept
		{
			const auto new_pos = offset >= 0
									 ? _pos + static_cast<index_type>(offset)
									 : _pos - static_cast<index_type>(-(offset + 1)) - 1;
			return Iterator(_slice, new_pos);
		}
		constexpr inline Iterator operator-(difference_type offset) const noexcept
		{
			const auto new_pos = offset >= 0
									 ? _pos - static_cast<index_type>(offset)
									 : _pos + static_cast<index_type>(-(offset + 1)) + 1;
			return Iterator(_slice, new_pos);
		}
		constexpr inline Iterator &operator+=(difference_type offset) noexcept
		{
			if (offset >= 0)
			{
				_pos += static_cast<index_type>(offset);
			}
			else
			{
				_pos -= static_cast<index_type>(-(offset + 1)) + 1;
			}
			return *this;
		}
		constexpr inline Iterator &operator-=(difference_type offset) noexcept
		{
			if (offset >= 0)
			{
				_pos -= static_cast<index_type>(offset);
			}
			else
			{
				_pos += static_cast<index_type>(-(offset + 1)) + 1;
			}
			return *this;
		}
		constexpr inline Iterator &operator++() noexcept
		{
			++_pos;
			return *this;
		}
		constexpr inline Iterator &operator--() noexcept
		{
			--_pos;
			return *this;
		}
		constexpr inline Iterator operator++(int) noexcept
		{
			Iterator temp = *this;
			++_pos;
			return temp;
		}
		constexpr inline Iterator operator--(int) noexcept
		{
			Iterator temp = *this;
			--_pos;
			return temp;
		}
		constexpr inline bool operator==(const Iterator &other) const noexcept
		{
			return _pos == other._pos;
		}
		constexpr inline bool operator!=(const Iterator &other) const noexcept
		{
			return _pos != other._pos;
		}
		constexpr inline bool operator<(const Iterator &other) const noexcept
		{
			return _pos < other._pos;
		}
		constexpr inline bool operator<=(const Iterator &other) const noexcept
		{
			return _pos <= other._pos;
		}
		constexpr inline bool operator>(const Iterator &other) const noexcept
		{
			return _pos > other._pos;
		}
		constexpr inline bool operator>=(const Iterator &other) const noexcept
		{
			return _pos >= other._pos;
		}
	};

public:
	constexpr inline Slice(const index_type &start, const index_type &stop, const step_type &step = 1)
		: start(start), stop(stop), step(step)
	{
		EMBMARTIN_NON_BLOCKING_ASSERT(step != 0, "Slice step must not be zero", &console);
		EMBMARTIN_NON_BLOCKING_ASSERT((step > 0 && start <= stop) || (step < 0 && start >= stop), "Slice range invalid", &console);
	}
	explicit constexpr inline Slice(const index_type &stop)
		: Slice(0, stop, 1)
	{
	}

	constexpr inline index_type operator[](const index_type &index) const
	{
		const auto magnitude = static_cast<index_type>(step > 0 ? step : -(step + 1)) + (step < 0 ? 1 : 0);
		const auto offset = index * magnitude;
		return step > 0 ? start + offset : start - offset;
	}

	constexpr inline Iterator begin() const noexcept
	{
		return Iterator(*this, 0);
	}
	constexpr inline Iterator end() const noexcept
	{
		return Iterator(*this, size());
	}
	constexpr inline Iterator cbegin() const noexcept
	{
		return Iterator(*this, 0);
	}
	constexpr inline Iterator cend() const noexcept
	{
		return Iterator(*this, size());
	}
	constexpr inline bool empty() const noexcept
	{
		return size() == 0;
	}
	constexpr inline index_type size() const noexcept
	{
		if (step == 0 || (step > 0 && start > stop) || (step < 0 && start < stop))
		{
			return 0;
		}
		const index_type distance = step > 0 ? stop - start : start - stop;
		const index_type magnitude = static_cast<index_type>(step > 0 ? step : -(step + 1)) + (step < 0 ? 1 : 0);
		return distance / magnitude + (distance % magnitude != 0);
	}
};

template <typename T>
using Range = Slice<T>;

/**
 * @brief 便捷构造 `Range<size_t>`。
 *
 * @param start 起始索引（含）。
 * @param stop 结束索引（不含）。
 * @param step 步长，默认为 `1`；负值表示反向步进。
 * @return 返回对应的 `Range<size_t>`。
 *
 * @note 形参类型是 `int`，内部会 `static_cast` 到 `size_t`；请只传非负索引。
 * 负值会转换成很大的无符号数，可能触发 `Slice` 的方向断言，或得到不符合直觉的区间。
 */
constexpr inline Range<size_t> range(int start, int stop, int step = 1)
{
	return Range<size_t>(static_cast<size_t>(start), static_cast<size_t>(stop), static_cast<std::make_signed_t<size_t>>(step));
}

/** @brief 便捷构造从 `0` 开始、步长为 `1` 的 `Range<size_t>`；`stop` 的取值要求同上。 */
constexpr inline Range<size_t> range(int stop)
{
	return Range<size_t>(static_cast<size_t>(stop));
}
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
	Tuple _tuple;

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
		: _tuple(std::forward<SOURCE>(tuple))
	{
	}

	template <size_t I>
	constexpr decltype(auto) get() & noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(_tuple);
	}

	template <size_t I>
	constexpr decltype(auto) get() const & noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(std::as_const(_tuple));
	}

	template <size_t I>
	constexpr decltype(auto) get() && noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(detail::_tuple_forward<Tuple>(std::move(_tuple)));
	}

	template <size_t I>
	constexpr decltype(auto) get() const && noexcept
	{
		static_assert(I < size, "TupleView index out of range");
		return detail::_tuple_get<source_index<I>::value>(detail::_tuple_forward<Tuple>(std::as_const(_tuple)));
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
	Tuple _tuple;
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
		: _tuple(std::forward<SOURCE>(tuple)), _extra(std::forward<EXTRA>(extra))
	{
	}

	template <typename SOURCE, typename... VALUES,
			  typename = std::enable_if_t<std::is_constructible_v<Tuple, SOURCE &&> &&
										  std::is_constructible_v<std::tuple<ExtraTs...>, VALUES &&...>>>
	explicit constexpr TupleInsertView(std::in_place_t, SOURCE &&tuple, VALUES &&...values) noexcept(
		std::is_nothrow_constructible_v<Tuple, SOURCE &&> &&
		std::is_nothrow_constructible_v<std::tuple<ExtraTs...>, VALUES &&...>)
		: _tuple(std::forward<SOURCE>(tuple)), _extra(std::forward<VALUES>(values)...)
	{
	}

	template <size_t I>
	constexpr decltype(auto) get() & noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(_tuple);
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(_extra);
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(_tuple);
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const & noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(std::as_const(_tuple));
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(_extra);
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(std::as_const(_tuple));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() && noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(detail::_tuple_forward<Tuple>(std::move(_tuple)));
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(std::move(_extra));
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(detail::_tuple_forward<Tuple>(std::move(_tuple)));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const && noexcept
	{
		static_assert(I < size, "TupleInsertView index out of range");
		if constexpr (I < InsertPos)
		{
			return detail::_tuple_get<detail::_tuple_view_index<I, Is...>::value>(detail::_tuple_forward<Tuple>(std::as_const(_tuple)));
		}
		else if constexpr (I < InsertPos + sizeof...(ExtraTs))
		{
			return detail::_tuple_get<I - InsertPos>(std::move(_extra));
		}
		else
		{
			return detail::_tuple_get<detail::_tuple_view_index<I - sizeof...(ExtraTs), Is...>::value>(detail::_tuple_forward<Tuple>(std::as_const(_tuple)));
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
		EMBMartin::generalized_tuple_size_v<remove_cvref_t<Tuple>>;

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

	Tuple _tuple;

	// 把前段 / 中间段 / 后段按 Python 解包结构物化成真实 tuple：
	// 前段与后段逐元素展开，中间段收进一个内层 std::tuple。
	template <typename SourceTuple, size_t... PreIs, size_t... MidIs, size_t... PostIs>
	static constexpr auto _materialize_impl(SourceTuple &&source,
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
		: _tuple(std::forward<SOURCE>(tuple))
	{
	}

	template <size_t I>
	constexpr decltype(auto) get() & noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(_tuple);
		}
		else if constexpr (I == PreN)
		{
			return mid_view_type{_tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(_tuple);
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const & noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(std::as_const(_tuple));
		}
		else if constexpr (I == PreN)
		{
			return const_mid_view_type{_tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(
				std::as_const(_tuple));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() && noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(std::move(_tuple));
		}
		else if constexpr (I == PreN)
		{
			return mid_view_type{_tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(
				std::move(_tuple));
		}
	}

	template <size_t I>
	constexpr decltype(auto) get() const && noexcept
	{
		static_assert(I < size, "TupleUnpackView index out of range");
		if constexpr (I < PreN)
		{
			return detail::_tuple_get<I>(std::as_const(_tuple));
		}
		else if constexpr (I == PreN)
		{
			return const_mid_view_type{_tuple};
		}
		else
		{
			return detail::_tuple_get<total_size - PostN + (I - PreN - 1)>(
				std::as_const(_tuple));
		}
	}

	constexpr auto materialize() &
	{
		return _materialize_impl(_tuple,
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}

	constexpr auto materialize() const &
	{
		return _materialize_impl(std::as_const(_tuple),
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}

	constexpr auto materialize() &&
	{
		return _materialize_impl(detail::_tuple_forward<Tuple>(std::move(_tuple)),
								 std::make_index_sequence<PreN>{},
								 std::make_index_sequence<mid_size>{},
								 std::make_index_sequence<PostN>{});
	}

	constexpr auto materialize() const &&
	{
		return _materialize_impl(detail::_tuple_forward<Tuple>(std::as_const(_tuple)),
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

//------------------------------------------ 高级管道运算 --------------------------------------------------

/**
 * @brief 管道式编程框架。
 *
 * 该部分提供了一个轻量化的函数组合机制。
 *
 * 其核心思想是：
 *
 * - `pipe(a, b, c)` 创建参数包；
 *
 * - `|` 运算符将当前参数包交给下一个函数；
 *
 * - 若函数返回值符合当前模式，可自动解包标准库 tuple-like 对象或广义 tuple 并继续传递；
 *
 * - 支持 `pipe_split`、`pipe_insert`、`pipe_erase`、`pipe_swap`、`pipe_prepend`
 *   等参数操作适配器。
 *
 * 这种风格在工程中很适合对链式处理过程建模，例如：
 * `pipe(x, y) | add | normalize | print`。
 *
 * 与传统函数栈相比，它更强调“数据流”而非嵌套调用，能够让参数转换和函数组合
 * 的逻辑更清晰地呈现出来。
 */

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

/**
 * @brief 管道函数返回值的处理策略。
 *
 * 这些模式只影响启用该模式后调用的函数，其返回值如何进入下一步；当前参数包
 * 传给函数时始终按 tuple 元素展开。模式标记不会改变当前参数包的展开方式。
 */
enum class _PipeUnpackMode
{
	NoUnpack = 0,		 ///< 返回的 tuple 作为一个参数。
	OnlyStdTuple = 1,	 ///< 只展开 std::tuple。
	OnlyTupleLike = 2,	 ///< 展开具有 tuple_size/get 协议的类型。
	AllTuplesConcept = 3 ///< 展开广义 tuple，包括可转换的聚合体。
};

/** @brief 解包模式标记类型，供 operator| 接收。 */
template <_PipeUnpackMode MODE>
struct _PipeUnpackModeManipulator
{
};

/** @brief 管道中的编译期切片操作描述。 */
template <size_t Start, size_t End, std::make_signed_t<size_t> Step>
struct _PipeSlicer
{
	static_assert(Step != 0);
	static_assert((End <= Start && Step < 0) || (End >= Start && Step > 0));
};

/** @brief 管道中的编译期插入操作描述。 */
template <size_t N, typename... ARGS>
struct _PipeInserter
{
	std::tuple<ARGS...> val;

	template <typename... VALUES, typename = std::enable_if_t<sizeof...(VALUES) == sizeof...(ARGS)>>
	explicit constexpr _PipeInserter(VALUES &&...values)
		: val(std::forward<VALUES>(values)...)
	{
	}
};

/** @brief 管道中的编译期删除操作描述。 */
template <size_t N>
struct _PipeEraser
{
};

/** @brief 管道中的编译期交换操作描述。 */
template <size_t N1, size_t N2>
struct _PipeSwapper
{
};

/**
 * @brief 管道参数包的拥有者与链式调用入口。
 *
 * TUPLE 始终是保存参数的实际类型，通常为 std::tuple。管道每次调用
 * 都消费当前参数包并返回新的 wrapper，因此不会把局部 tuple view 的
 * 引用泄露到下一阶段。MODE 控制函数返回的 tuple-like 或广义 tuple 是否解包。
 *
 * 提供 tuple_size / tuple_element 特化和成员 get<N>()，可用于 to_tuple() / make_to_tuple() 转换；
 *
 * 结构化绑定支持取决于编译器实现。
 */
template <_PipeUnpackMode MODE, typename TUPLE>
class _PipeWrapper
{
private:
	TUPLE _args;

	constexpr _PipeWrapper() = default;

	template <typename RESULT>
	static constexpr auto _wrap_result(RESULT &&result)
	{
		using result_type = remove_cvref_t<RESULT>;
		if constexpr (
			(MODE == _PipeUnpackMode::OnlyStdTuple && is_std_tuple_v<result_type>) ||
			(MODE == _PipeUnpackMode::OnlyTupleLike && is_tuple_like_v<result_type>) ||
			(MODE == _PipeUnpackMode::AllTuplesConcept && is_generalized_tuple_v<result_type>))
		{
			auto tuple_value = make_to_tuple(std::forward<RESULT>(result));
			using tuple_type = remove_cvref_t<decltype(tuple_value)>;
			return _PipeWrapper<MODE, tuple_type>(std::move(tuple_value));
		}
		else
		{
			return _PipeWrapper<MODE, std::tuple<result_type>>(
				std::forward<RESULT>(result));
		}
	}

public:
	template <_PipeUnpackMode _MODE, typename _TUPLE>
	friend class _PipeWrapper;

	/** @brief 用参数构造一个拥有参数包的管道对象。 */
	template <typename... ARGS,
			  typename = std::enable_if_t<std::is_constructible_v<TUPLE, ARGS &&...>>>
	explicit constexpr inline _PipeWrapper(ARGS &&...args) noexcept
		: _args(std::forward<ARGS>(args)...)
	{
	}

	/** @brief 从同类型标准 tuple 构造管道对象。 */
	template <_PipeUnpackMode M = MODE, typename = std::enable_if_t<M == _PipeUnpackMode::OnlyStdTuple>>
	explicit constexpr inline _PipeWrapper(const TUPLE &tup) noexcept
		: _args(tup)
	{
	}

	/** @brief 访问管道参数包中的第 N 项。 */
	template <size_t N>
	constexpr decltype(auto) get() & noexcept
	{
		return std::get<N>(_args);
	}
	template <size_t N>
	constexpr decltype(auto) get() const & noexcept
	{
		return std::get<N>(_args);
	}
	template <size_t N>
	constexpr decltype(auto) get() && noexcept
	{
		return std::get<N>(std::move(_args));
	}
	template <size_t N>
	constexpr decltype(auto) get() const && noexcept
	{
		return std::get<N>(std::move(_args));
	}

	/**
	 * @brief 将当前参数包解包传给可调用对象。
	 *
	 * void 返回值产生空参数包；普通返回值作为一个参数保存；符合当前
	 * MODE 的 tuple-like 或广义 tuple 返回值会被转为标准 tuple 后解包保存。
	 */
	template <typename FUNC, typename = std::enable_if_t<is_invocable_with_std_tuple_v<FUNC, TUPLE>>>
	constexpr auto operator|(FUNC &&func) &&
	{
		using result_type = decltype(std::apply(std::declval<FUNC>(), std::declval<TUPLE>()));
		if constexpr (std::is_void_v<result_type>)
		{
			std::apply(
				std::forward<decltype(func)>(func),
				std::move(_args));

			return _PipeWrapper<MODE, std::tuple<>>();
		}
		else
		{
			auto &&rslt = std::apply(
				std::forward<decltype(func)>(func),
				std::move(_args));
			return _wrap_result(std::forward<decltype(rslt)>(rslt));
		}
	}

	/** @brief 合并两个位置参数管道，结果按左侧参数后接右侧参数排列。 */
	template <_PipeUnpackMode OTHER_MODE, typename OTHER_TUPLE>
	constexpr auto operator|(_PipeWrapper<OTHER_MODE, OTHER_TUPLE> other) &&
	{
		using merged_type = decltype(std::tuple_cat(std::move(_args), std::move(other._args)));
		return _PipeWrapper<MODE, merged_type>(
			std::tuple_cat(std::move(_args), std::move(other._args)));
	}

	/** @brief 切换返回值解包模式。 */
	template <_PipeUnpackMode NEW_MODE>
	constexpr auto operator|(_PipeUnpackModeManipulator<NEW_MODE>) &&
	{
		return _PipeWrapper<NEW_MODE, TUPLE>(std::move(_args));
	}

	/** @brief 对当前参数包做编译期切片。 */
	template <size_t Start, size_t End, std::make_signed_t<size_t> Step>
	constexpr auto operator|(_PipeSlicer<Start, End, Step>) &&
	{
		auto new_args = split<Start, End, Step>(std::move(_args));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}

	/** @brief 在指定位置插入一个或多个参数。 */
	template <size_t N, typename... T>
	constexpr auto operator|(_PipeInserter<N, T...> inserter) &&
	{
		auto new_args = insert_tuple<N>(std::move(_args), std::move(inserter.val));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}

	/** @brief 删除指定位置的一个参数。 */
	template <size_t N>
	constexpr auto operator|(_PipeEraser<N>) &&
	{
		auto new_args = erase<N>(std::move(_args));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}

	/** @brief 交换两个编译期索引对应的参数。 */
	template <size_t N1, size_t N2>
	constexpr auto operator|(_PipeSwapper<N1, N2>) &&
	{
		auto new_args = swap<N1, N2>(std::move(_args));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}
};

template <typename T>
struct is_pipe : std::false_type
{
};
template <_PipeUnpackMode MODE, typename TUPLE>
struct is_pipe<_PipeWrapper<MODE, TUPLE>> : std::true_type
{
};
template <typename T>
constexpr bool is_pipe_v = is_pipe<T>::value;

EMBMARTIN_DETAIL_NAMESPACE_END

/**
 * @brief 创建一个位置参数管道入口。
 *
 * `pipe(...)` 会将输入参数打包为一个内部参数元组，并返回一个 `_PipeWrapper`。
 * 随后可以通过 `|` 运算符依次把当前参数包传递给下一个函数，形成类似 Unix 管道的处理链。
 *
 * @details
 * 每次调用 `|` 都会先读取当前参数包，再按 tuple 元素方式展开并传递给下一个可调用对象。
 * 若返回值符合当前解包模式，则会继续被包装成新的参数包，形成链式传递。模式标记应放在
 * 要处理其返回值的函数之前；标记不会改变当前参数包的展开方式。
 *
 * 默认模式为 unpack_std_tuple；
 * 若需要解包 tuple-like 或广义 tuple，可切换到 unpack_tuple_like / unpack_tuples_concept；
 * 若希望返回值整体作为单个参数，可用 no_unpack。
 *
 * @tparam ARGS... 入口参数的类型列表。
 * @param args... 首次进入管道的参数值。
 * @return 返回一个持有当前参数包的 `_PipeWrapper`，可继续使用 `|` 连接更多处理步骤。
 *
 * @note 这是“位置参数型入口”；如果你希望以 `std::tuple` 作为起点并立即按元素展开，
 * 请使用 `pipe_tuple()`。
 *
 * @note `|` 的结果始终是 `_PipeWrapper`，而不是链尾函数的返回值；需要最终值时用
 * `get<N>()` 取出，或用 `to_tuple()` / `make_to_tuple()` 转成标准 tuple，也可用结构化绑定接收。
 *
 * ```cpp
 * auto result = pipe(1, 2)
 *     | [](int a, int b) { return std::make_tuple(a + b, a * b); }
 *     | [](int sum, int product) { return product + sum; };
 * int value = result.get<0>();  // 5
 * ```
 */
template <typename... ARGS>
constexpr decltype(auto) pipe(ARGS &&...args) noexcept
{
	using tuple_type = std::tuple<std::decay_t<ARGS>...>;
	return detail::_PipeWrapper<detail::_PipeUnpackMode::OnlyStdTuple, tuple_type>(
		std::forward<ARGS>(args)...);
}

/**
 * @brief 从 `std::tuple` 创建立即可解包的管道入口。
 *
 * `pipe_tuple()` 以标准元组作为起点，并将其视为已展开的参数包。
 * 因此后续的 lambda 可以直接按参数列表形式接收元素值，而不必再手动展开该 tuple。
 *
 * @details
 * 它和 `pipe()` 的主要区别在于：`pipe()` 先收集位置参数，再在后续阶段展开；
 * 而 `pipe_tuple()` 直接从一个已经存在的 tuple 开始，因此更适合把已有参数容器
 * 封装成函数链。
 *
 * @tparam ARGS... 元组中各元素的类型列表。
 * @param args 入口参数元组。
 * @return 返回一个持有当前参数包的 `_PipeWrapper`，后续可继续用 `|` 串联函数。
 *
 * @note 适合在已存在多个参数值时直接进入管道，避免重复手动展开。
 *
 * @note 管道表达式的结果是 `_PipeWrapper`，不是最终值；需要时用 `get<0>()` 取出，
 * 或用 `to_tuple()` / `make_to_tuple()` 转成标准 tuple，也可用结构化绑定接收。
 *
 * ```cpp
 * std::tuple<int, int> args{3, 4};
 * auto result = pipe_tuple(args) | [](int a, int b) { return a + b; };
 * int sum = result.get<0>();  // 7
 * ```
 */
template <typename... ARGS>
constexpr decltype(auto) pipe_tuple(const std::tuple<ARGS...> &args) noexcept
{
	return detail::_PipeWrapper<detail::_PipeUnpackMode::OnlyStdTuple, std::tuple<ARGS...>>(args);
}

template <typename... ARGS>
constexpr decltype(auto) pipe_tuple(std::tuple<ARGS...> &&args) noexcept
{
	return detail::_PipeWrapper<detail::_PipeUnpackMode::OnlyStdTuple, std::tuple<ARGS...>>(
		std::move(args));
}

// ====== 管道适配器 ======

/**
 * @brief 关闭返回值解包。
 *
 * 将此标记放在函数调用之前时，该函数返回的 tuple 会作为下一阶段的单个参数，
 * 不会展开为多个参数。它不会把当前管道参数包重新包装成一个参数。
 */
constexpr detail::_PipeUnpackModeManipulator<detail::_PipeUnpackMode::NoUnpack> no_unpack{};

/**
 * @brief 仅展开 `std::tuple` 返回值。
 *
 * 这是默认行为，兼顾了可读性和稳定性，适合多数普通函数链。
 */
constexpr detail::_PipeUnpackModeManipulator<detail::_PipeUnpackMode::OnlyStdTuple> unpack_std_tuple{};

/**
 * @brief 展开所有满足 tuple 协议的对象。
 *
 * 包括标准 tuple 与带有 `tuple_size` / `get` 的类型，适合偏泛型元组处理。
 */
constexpr detail::_PipeUnpackModeManipulator<detail::_PipeUnpackMode::OnlyTupleLike> unpack_tuple_like{};

/**
 * @brief 展开广义元组概念中的所有可解包对象。
 *
 * 这会放宽到“所有可转换为 tuple 的对象”，适合在高层业务代码中使用，但需要时刻
 * 注意返回值是否真的应当被解包成参数列表。
 */
constexpr detail::_PipeUnpackModeManipulator<detail::_PipeUnpackMode::AllTuplesConcept> unpack_tuples_concept{};

/**
 * @brief 在管道参数包中创建一个切片描述对象。
 *
 * 该辅助函数只生成一个编译期描述值，用于在 `|` 链中对当前参数包执行
 * `[Start, End)` 区间裁切。它本身不改变参数值，只把切片意图交给
 * `_PipeWrapper::operator|` 继续执行。
 *
 * @details
 * 这是 `split<Start, End, Step>` 在管道风格中的对应版本。与普通 tuple 切片相比，
 * 它把索引信息保存在一个轻量描述对象中，随后在管道中被消费、展开为实际的新参数包。
 *
 * 这种写法适合把参数裁剪、重组和函数组合放进同一条链式表达式中，例如：
 * `pipe(a, b, c, d) | pipe_split<1, 4>() | func`。
 *
 * @tparam Start 起始索引，包含在结果中。
 * @tparam End 结束索引，前闭后开。
 * @tparam Step 步长，默认为 `1`。
 * @return 返回一个描述当前切片需求的 `_PipeSlicer` 对象。
 *
 * ```cpp
 * auto x = pipe(1, 2, 3, 4, 5) | pipe_split<1, 4>();       // {2, 3, 4}
 * auto y = pipe(10, 20, 30, 40) | pipe_split<3, 0, -1>();  // {40, 30, 20}
 * ```
 */
template <size_t Start, size_t End, std::make_signed_t<size_t> Step = 1>
constexpr auto pipe_split()
{
	return detail::_PipeSlicer<Start, End, Step>{};
}

/**
 * @brief 在管道参数包中创建一个从 0 开始的前缀切片描述对象。
 *
 * 这是 `pipe_split<0, End, 1>()` 的语义化重载，常用于对前缀区间直接裁剪。
 *
 * @details
 * 对于很多场景来说，参数包的前缀处理比一般的区间切片更常见，因此该重载
 * 提供了更直接的表达方式，减少模板参数写法带来的噪音。
 *
 * @tparam End 前缀长度，即切片右端点。
 * @return 返回一个描述前缀切片的 `_PipeSlicer` 对象。
 *
 * ```cpp
 * auto x = pipe(1, 2, 3, 4, 5) | pipe_split<3>();  // {1, 2, 3}
 * // 等价于: pipe_split<0, 3, 1>()
 * ```
 */
template <size_t End>
constexpr auto pipe_split()
{
	return detail::_PipeSlicer<0, End, 1>{};
}

/**
 * @brief 在管道参数包的索引 N 处插入一个或多个参数。
 *
 * 该辅助函数返回一个描述对象，随后由 `_PipeWrapper::operator|` 执行实际插入。
 * 与 `insert<N>(...)` 的语义一致，但以管道式描述的方式嵌入链式调用。
 *
 * @details
 * 该描述对象并不立即执行插入，而是把插入位置和待加入值保存为编译期信息，
 * 等到管道消费时再应用到当前参数包上。这样可以让 `pipe(...) | pipe_insert<1>(x, y)`
 * 这种链式表达式保持清晰，同时保留模板级索引信息。
 *
 * @tparam N 插入的位置索引。
 * @tparam Ty... 要插入的值类型列表。
 * @param val... 要插入的参数值。
 * @return 返回一个 `_PipeInserter` 描述对象。
 *
 * ```cpp
 * auto x = pipe(1, 3) | pipe_insert<1>(2);      // {1, 2, 3}
 * auto y = pipe(1, 4) | pipe_insert<1>(2, 3);  // {1, 2, 3, 4}
 * ```
 */
template <size_t N, typename... Ty>
constexpr auto pipe_insert(Ty &&...val)
{
	return detail::_PipeInserter<N, std::decay_t<Ty>...>(std::forward<Ty>(val)...);
}

/**
 * @brief 在管道参数包中删除索引 N 处的一个参数。
 *
 * 它会在 `|` 链中对当前参数包执行 `erase<N>()` 的语义，并返回新的参数包。
 *
 * @details
 * 这是一种“描述性操作”：调用 `pipe_erase<N>()` 时，仅仅构造删除描述对象，
 * 真正的删除动作发生在后续 `_PipeWrapper::operator|` 中。
 *
 * @tparam N 要删除的参数索引。
 * @return 返回一个 `_PipeEraser` 描述对象。
 *
 * ```cpp
 * auto x = pipe(10, 20, 30) | pipe_erase<1>();  // {10, 30}
 * ```
 */
template <size_t N>
constexpr auto pipe_erase()
{
	return detail::_PipeEraser<N>{};
}

/**
 * @brief 在管道参数包中交换两个索引对应的参数。
 *
 * 它等价于 `swap<N1, N2>()`，但使用管道描述对象的方式嵌入链式处理。
 *
 * @details
 * 若需要在参数包中调整顺序而不破坏链式表达式，可直接用该适配器，
 * 好处是语义与高层算法保持一致，并且无需显式构造中间 tuple。
 *
 * @tparam N1 第一个交换位置。
 * @tparam N2 第二个交换位置。
 * @return 返回一个 `_PipeSwapper` 描述对象。
 *
 * ```cpp
 * auto x = pipe(1, 2, 3, 4) | pipe_swap<1, 3>();  // {1, 4, 3, 2}
 * ```
 */
template <size_t N1, size_t N2>
constexpr auto pipe_swap()
{
	return detail::_PipeSwapper<N1, N2>{};
}

/**
 * @brief 在管道参数包前端插入一个或多个参数。
 *
 * 这是 `pipe_insert<0>(...)` 的语义化封装，适合把前导参数快速加入当前管道参数包。
 *
 * @details
 * 该适配器常用于把过滤、校验或上下文参数插到已有参数序列前面，
 * 例如在函数链中加入中间状态变量而不打破已有调用顺序。
 *
 * @tparam Ty... 要插入参数的类型列表。
 * @param val... 要插入的参数值。
 * @return 返回一个 `_PipeInserter<0, ...>` 描述对象。
 *
 * ```cpp
 * auto x = pipe(2, 3) | pipe_prepend(1);  // {1, 2, 3}
 * auto y = pipe(3, 4) | pipe_prepend(1, 2);  // {1, 2, 3, 4}
 * ```
 */
template <typename... Ty>
constexpr auto pipe_prepend(Ty &&...val)
{
	return pipe_insert<0>(std::forward<Ty>(val)...);
}

template <typename T, typename FUNC,
		  typename TRaw = std::remove_reference_t<T>,
		  typename = std::enable_if_t<
			  !detail::is_pipe_v<TRaw> &&
			  std::is_invocable_v<FUNC &&, T &&>>>
constexpr decltype(auto) operator|(T &&val, FUNC &&func)
{
	return func(std::forward<T>(val));
}

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

template <size_t N, typename Tuple, typename IndexSequence, size_t InsertPos, typename ExtraTuple,
		  bool Before = (N < InsertPos),
		  bool InExtra = (N >= InsertPos && N < InsertPos + std::tuple_size_v<ExtraTuple>)>
struct _tuple_insert_element;

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct _tuple_insert_element<N, Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>, true, false>
{
	using type = EMBMartin::generalized_tuple_element_t<_tuple_view_index<N, Is...>::value, EMBMartin::remove_cvref_t<Tuple>>;
};

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct _tuple_insert_element<N, Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>, false, true>
{
	using type = std::tuple_element_t<N - InsertPos, std::tuple<ExtraTs...>>;
};

template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
struct _tuple_insert_element<N, Tuple, std::index_sequence<Is...>, InsertPos, std::tuple<ExtraTs...>, false, false>
{
	using type = EMBMartin::generalized_tuple_element_t<
		_tuple_view_index<N - sizeof...(ExtraTs), Is...>::value, EMBMartin::remove_cvref_t<Tuple>>;
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
	using element = EMBMartin::generalized_tuple_element_t<N, raw_ref>;
	using type = std::conditional_t<Const, std::add_const_t<element>, element>;
};

// 中间：N == PreN —— 返回子视图类型
template <size_t N, typename Tuple, size_t PreN, size_t PostN, bool Const>
struct _tuple_unpack_view_element<N, Tuple, PreN, PostN, Const, false, true>
{
	static constexpr size_t total_size =
		EMBMartin::generalized_tuple_size_v<EMBMartin::remove_cvref_t<Tuple>>;
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

	using type = EMBMartin::TupleView<storage_ref, mid_seq>;
};

// 后置：N > PreN
template <size_t N, typename Tuple, size_t PreN, size_t PostN, bool Const>
struct _tuple_unpack_view_element<N, Tuple, PreN, PostN, Const, false, false>
{
	using raw_ref = std::remove_reference_t<Tuple>;
	using element = EMBMartin::generalized_tuple_element_t<
		EMBMartin::generalized_tuple_size_v<EMBMartin::remove_cvref_t<Tuple>> - PostN + (N - PreN - 1),
		raw_ref>;
	using type = std::conditional_t<Const, std::add_const_t<element>, element>;
};
EMBMARTIN_DETAIL_NAMESPACE_END

EMBMARTIN_NAMESPACE_END

namespace std
{
	template <typename Tuple, size_t... Is>
	struct tuple_size<EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
		: integral_constant<size_t, sizeof...(Is)>
	{
	};
	template <typename Tuple, size_t... Is>
	struct tuple_size<const EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
		: tuple_size<EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
	};
	template <typename Tuple, size_t... Is>
	struct tuple_size<volatile EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
		: tuple_size<EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
	};
	template <typename Tuple, size_t... Is>
	struct tuple_size<const volatile EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
		: tuple_size<EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
	};

	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: integral_constant<size_t, sizeof...(Is) + sizeof...(ExtraTs)>
	{
	};
	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<const EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: tuple_size<EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
	};
	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<volatile EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: tuple_size<EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
	};
	template <typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_size<const volatile EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
		: tuple_size<EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
	};

	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = EMBMartin::generalized_tuple_element_t<
			EMBMartin::detail::_tuple_view_index<N, Is...>::value, EMBMartin::remove_cvref_t<Tuple>>;
	};
	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, const EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = add_const_t<typename tuple_element<N, EMBMartin::TupleView<Tuple, index_sequence<Is...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, volatile EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = add_volatile_t<typename tuple_element<N, EMBMartin::TupleView<Tuple, index_sequence<Is...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is>
	struct tuple_element<N, const volatile EMBMartin::TupleView<Tuple, index_sequence<Is...>>>
	{
		using type = add_cv_t<typename tuple_element<N, EMBMartin::TupleView<Tuple, index_sequence<Is...>>>::type>;
	};

	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = typename EMBMartin::detail::_tuple_insert_element<
			N, Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>::type;
	};
	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, const EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = add_const_t<typename tuple_element<N, EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, volatile EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = add_volatile_t<typename tuple_element<N, EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>::type>;
	};
	template <size_t N, typename Tuple, size_t... Is, size_t InsertPos, typename... ExtraTs>
	struct tuple_element<N, const volatile EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>
	{
		using type = add_cv_t<typename tuple_element<N, EMBMartin::TupleInsertView<Tuple, index_sequence<Is...>, InsertPos, tuple<ExtraTs...>>>::type>;
	};

	template <size_t N, EMBMartin::detail::_PipeUnpackMode MODE, typename TUPLE>
	struct tuple_element<N, EMBMartin::detail::_PipeWrapper<MODE, TUPLE>>
		: tuple_element<N, TUPLE>
	{
	};
	template <EMBMartin::detail::_PipeUnpackMode MODE, typename TUPLE>
	struct tuple_size<EMBMartin::detail::_PipeWrapper<MODE, TUPLE>>
		: tuple_size<TUPLE>
	{
	};


	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
		: integral_constant<size_t, PreN + PostN + 1>
	{
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<const EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_size<EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<volatile EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_size<EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <typename Tuple, size_t PreN, size_t PostN>
	struct tuple_size<const volatile EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_size<EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
		using type = typename EMBMartin::detail::_tuple_unpack_view_element<
			N, Tuple, PreN, PostN, false>::type;
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, const EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
		using type = typename EMBMartin::detail::_tuple_unpack_view_element<
			N, Tuple, PreN, PostN, true>::type;
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, volatile EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_element<N, EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

	template <size_t N, typename Tuple, size_t PreN, size_t PostN>
	struct tuple_element<N, const volatile EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
		: tuple_element<N, EMBMartin::TupleUnpackView<Tuple, PreN, PostN>>
	{
	};

} // namespace std

#endif // EMBMARTIN_FUNCTIONAL_PROGRAMMING_H