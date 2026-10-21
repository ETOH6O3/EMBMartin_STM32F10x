/**
 ******************************************************************************
 * @file    pipe.h
 * @brief   管道式编程接口 pipe：串联函数调用并按需解包返回值
 ******************************************************************************
 */

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "macro.h"
#include "meta.h"
#include "tuple_view.h"
#include "tuple_utility.h"

EMBMARTIN_NAMESPACE_BEGIN

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
	TUPLE __args;

	constexpr _PipeWrapper() = default;

	template <typename RESULT>
	static constexpr auto __wrap_result(RESULT &&result)
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
		: __args(std::forward<ARGS>(args)...)
	{
	}

	/** @brief 从同类型标准 tuple 构造管道对象。 */
	template <_PipeUnpackMode M = MODE, typename = std::enable_if_t<M == _PipeUnpackMode::OnlyStdTuple>>
	explicit constexpr inline _PipeWrapper(const TUPLE &tup) noexcept
		: __args(tup)
	{
	}

	/** @brief 访问管道参数包中的第 N 项。 */
	template <size_t N>
	constexpr decltype(auto) get() & noexcept
	{
		return std::get<N>(__args);
	}
	template <size_t N>
	constexpr decltype(auto) get() const & noexcept
	{
		return std::get<N>(__args);
	}
	template <size_t N>
	constexpr decltype(auto) get() && noexcept
	{
		return std::get<N>(std::move(__args));
	}
	template <size_t N>
	constexpr decltype(auto) get() const && noexcept
	{
		return std::get<N>(std::move(__args));
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
				std::move(__args));

			return _PipeWrapper<MODE, std::tuple<>>();
		}
		else
		{
			auto &&rslt = std::apply(
				std::forward<decltype(func)>(func),
				std::move(__args));
			return __wrap_result(std::forward<decltype(rslt)>(rslt));
		}
	}

	/** @brief 合并两个位置参数管道，结果按左侧参数后接右侧参数排列。 */
	template <_PipeUnpackMode OTHER_MODE, typename OTHER_TUPLE>
	constexpr auto operator|(_PipeWrapper<OTHER_MODE, OTHER_TUPLE> other) &&
	{
		using merged_type = decltype(std::tuple_cat(std::move(__args), std::move(other.__args)));
		return _PipeWrapper<MODE, merged_type>(
			std::tuple_cat(std::move(__args), std::move(other.__args)));
	}

	/** @brief 切换返回值解包模式。 */
	template <_PipeUnpackMode NEW_MODE>
	constexpr auto operator|(_PipeUnpackModeManipulator<NEW_MODE>) &&
	{
		return _PipeWrapper<NEW_MODE, TUPLE>(std::move(__args));
	}

	/** @brief 对当前参数包做编译期切片。 */
	template <size_t Start, size_t End, std::make_signed_t<size_t> Step>
	constexpr auto operator|(_PipeSlicer<Start, End, Step>) &&
	{
		auto new_args = split<Start, End, Step>(std::move(__args));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}

	/** @brief 在指定位置插入一个或多个参数。 */
	template <size_t N, typename... T>
	constexpr auto operator|(_PipeInserter<N, T...> inserter) &&
	{
		auto new_args = insert_tuple<N>(std::move(__args), std::move(inserter.val));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}

	/** @brief 删除指定位置的一个参数。 */
	template <size_t N>
	constexpr auto operator|(_PipeEraser<N>) &&
	{
		auto new_args = erase<N>(std::move(__args));
		auto materialized = to_tuple(std::move(new_args));
		using NEW_TUPLE = remove_cvref_t<decltype(materialized)>;
		return _PipeWrapper<MODE, NEW_TUPLE>(std::move(materialized));
	}

	/** @brief 交换两个编译期索引对应的参数。 */
	template <size_t N1, size_t N2>
	constexpr auto operator|(_PipeSwapper<N1, N2>) &&
	{
		auto new_args = swap<N1, N2>(std::move(__args));
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


EMBMARTIN_NAMESPACE_END

//------------------------------------------ std 特化：支持 structured binding ----------
namespace std
{
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
} // namespace std
