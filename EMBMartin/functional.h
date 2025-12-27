/**
 ******************************************************************************
 * @file    functional.h
 * @author  孙鸣淼
 * @brief   EMBMartin 内置函数式编程库
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 ******************************************************************************
 */

#ifndef EMBMARTIN_FUNCTIONAL_PROGRAMMING_H
#define EMBMARTIN_FUNCTIONAL_PROGRAMMING_H

#include <functional>
#include <tuple> //add tuple

#include "macro.h"
#include "meta.h"
#include "mmath.h"
#include "stream.h"

EMBMARTIN_NAMESPACE_BEGIN

//------------------------------------------python 风格 slice 和 range 对象 --------------------------------------------------

template <typename T>
struct Slice
{
    static_assert(std::is_integral_v<T>);

private:
    using index_type = std::make_unsigned_t<T>;
    using step_type = std::make_signed_t<T>;

public:
    const index_type start;
    const index_type stop;
    const step_type step;

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
        constexpr inline Iterator operator+(size_t offset) const noexcept
        {
            return Iterator(_slice, _pos + offset);
        }
        constexpr inline Iterator operator-(size_t offset) const noexcept
        {
            return Iterator(_slice, _pos - offset);
        }
        constexpr inline Iterator &operator+=(size_t offset) noexcept
        {
            _pos += offset;
            return *this;
        }
        constexpr inline Iterator &operator-=(size_t offset) noexcept
        {
            _pos -= offset;
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
        if constexpr (std::is_signed_v<step_type>)
        {
            return start + index * step;
        }
        else
        {
            return start + index * static_cast<step_type>(1);
        }
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
        return (stop - start) / step;
    }
};

template struct Slice<size_t>;

template <typename T>
using Range = Slice<T>;

constexpr inline Range<size_t> range(int start, int stop, int step = 1)
{
    return Range<size_t>(static_cast<size_t>(start), static_cast<size_t>(stop), static_cast<std::make_signed_t<size_t>>(step));
}
constexpr inline Range<size_t> range(int stop)
{
    return Range<size_t>(static_cast<size_t>(stop));
}
//------------------------------------------ 广义 tuples 基础操作 --------------------------------------------------

/**
 * @brief 将广义 tuples 对象转发为标准元组
 */
template <typename TUPLE>
constexpr decltype(auto) to_tuple(TUPLE &&_tuple_obj)
{
    using raw_t = std::remove_reference_t<TUPLE>;
    if constexpr (is_std_tuple_v<raw_t>)
    {
        return std::forward<TUPLE>(_tuple_obj);
    }
    else if constexpr (is_tuple_like_v<raw_t>)
    {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return std::make_tuple(std::get<Is>(std::forward<TUPLE>(_tuple_obj))...);
        }(std::make_index_sequence<std::tuple_size_v<raw_t>>{});
    }
    else
    {
        return detail::_to_tuple(std::forward<TUPLE>(_tuple_obj));
    }
}
/**
 * @brief 将广义 tuples 对象转换为标准元组
 */
template <typename TUPLE>
constexpr decltype(auto) make_to_tuple(TUPLE &&_tuple_obj)
{
    return std::apply(
        [](auto &&...xs)
        {
            // 消去引用限定符
            return std::make_tuple(std::forward<decltype(xs)>(xs)...);
        },
        to_tuple(std::forward<TUPLE>(_tuple_obj)));
}

/**
 * @brief 适用于所有广义 tuples 对象的 apply 函数
 */
template <typename FUNC, typename TUPLE>
constexpr decltype(auto) apply(FUNC &&visit, TUPLE &&_tuple_obj)
{
    return std::apply(std::forward<FUNC>(visit), to_tuple(std::forward<TUPLE>(_tuple_obj)));
}

// get 方法
template <size_t N, typename TUPLE>
constexpr decltype(auto) get(TUPLE &&_tuple_obj)
{
    return std::get<N>(to_tuple(std::forward<TUPLE>(_tuple_obj)));
}
//------------------------------------------python 风格广义元组解包 --------------------------------------------------

/**
 * @brief 解包出分立的后 N 个元素和前面剩余元素的元组
 *
 * 用法：
 * ```cpp
 * auto [a, rest, b] = unpack<1, 1>(tuple_obj);
 * auto [a, b, rest] = unpack<2, 0>(tuple_obj);
 * auto [rest, a, b] = unpack<0, 2>(tuple_obj);
 * auto [all] = unpack<0, 0>(tuple_obj);
 * auto [a, empty, b] = unpack<1, 1>(two_member_tuple_obj);
 * ```
 * 相当于：
 * ```python
 * a, *rest, b = sequence_obj
 * a, b, *rest = sequence_obj
 * *rest, a, b = sequence_obj
 * *all = sequence_obj
 * a, *empty, b = two_member_sequence_obj
 * ```
 */
template <size_t PRE, size_t POST, typename Tuple>
constexpr decltype(auto) unpack(Tuple &&t)
{
    constexpr size_t Size = tuple_size_v<std::remove_reference_t<Tuple>>;
    static_assert(PRE + POST <= Size, "Index must not exceed tuple size");

    auto &&full_tuple = to_tuple(std::forward<Tuple>(t));
    return [&]<size_t... PreIs, size_t... MidIs, size_t... PostIs>(
               std::index_sequence<PreIs...>,
               std::index_sequence<MidIs...>,
               std::index_sequence<PostIs...>)
    {
        return std::make_tuple(
            std::get<PreIs>(std::forward<decltype(full_tuple)>(full_tuple))...,
            std::make_tuple(std::get<PRE + MidIs>(std::forward<decltype(full_tuple)>(full_tuple))...),
            std::get<Size - POST + PostIs>(std::forward<decltype(full_tuple)>(full_tuple))...);
    }(std::make_index_sequence<PRE>{},
      std::make_index_sequence<Size - PRE - POST>{},
      std::make_index_sequence<POST>{});
}
/**
 * @brief 解包出分立的前 N 个元素和剩余元素的元组
 *
 * 用法：
 * ```cpp
 * auto [a, b, rest] = unpack_pre<2>(tuple_obj);
 * auto [all] = unpack_pre<0>(tuple_obj);
 * auto [a, b, empty] = unpack_pre<2>(two_member_tuple_obj);
 * ```
 * 相当于：
 * ```python
 * a, b, *rest = sequence_obj
 * *all = sequence_obj
 * a, b, *empty = two_member_sequence_obj
 * ```
 */
template <size_t N, typename Tuple>
constexpr decltype(auto) unpack_pre(Tuple &&t)
{
    return unpack<N, 0>(std::forward<Tuple>(t));
}

/**
 * @brief 解包出第一个元素和剩余元素的元组
 *
 * 用法：
 * ```cpp
 * auto [first, rest] = unpack_first (tuple_obj);
 * ```
 * 相当于：
 * ```python
 * a, *b = sequence_obj
 * ```
 */
template <typename Tuple>
constexpr decltype(auto) unpack_first(Tuple &&t)
{
    return unpack_pre<1>(std::forward<Tuple>(t));
}

/**
 * @brief 解包出分立的后 N 个元素和前面剩余元素的元组
 *
 * 用法：
 * ```cpp
 * auto [rest, a, b] = unpack_post<2>(tuple_obj);
 * auto [all] = unpack_post<0>(tuple_obj);
 * auto [empty, a, b] = unpack_post<2>(two_member_tuple_obj);
 * ```
 * 相当于：
 * ```python
 * *rest, a, b = sequence_obj
 * *all = sequence_obj
 * *empty, a, b = sequence_obj
 * ```
 */
template <size_t N, typename Tuple>
constexpr decltype(auto) unpack_post(Tuple &&t)
{
    return unpack<0, N>(std::forward<Tuple>(t));
}
/**
 * @brief 解包出分立的后一个元素和前面剩余元素的元组
 *
 * 用法：
 * ```cpp
 * auto [rest, a] = unpack_last (tuple_obj);
 * ```
 * 相当于：
 * ```python
 * *rest, a = sequence_obj
 * ```
 */
template <typename Tuple>
constexpr decltype(auto) unpack_last(Tuple &&t)
{
    return unpack_post<1>(std::forward<Tuple>(t));
}

//------------------------------------------ 广义元组操作 --------------------------------------------------
// TODO: 改为用视图实现

template <size_t Start, size_t End, ::std::make_signed_t<size_t> Step, typename Tuple,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) split(Tuple &&t)
{
    constexpr size_t Size = tuple_size_v<std::remove_reference_t<Tuple>>;
    static_assert(Step != 0);
    constexpr size_t count = (Step > 0) ? ceil_div(End - Start, size_t(Step)) : ceil_div(Start - End, size_t(-Step));

    if constexpr ((End <= Start && Step < 0) || (End >= Start && Step > 0))
    {
        return [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            return std::make_tuple(
                EMBMartin::get<Is * Step + Start>(std::forward<Tuple>(t))...);
        }(std::make_index_sequence<count>());
    }
    else
    {
        return std::make_tuple();
    }
}
template <size_t Start, size_t End, typename Tuple,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) split(Tuple &&t)
{
    return split<Start, End, 1>(std::forward<Tuple>(t));
}

template <size_t End, typename Tuple,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) split(Tuple &&t)
{
    return split<0, End>(::std::forward<Tuple>(t));
}

template <size_t N, typename Tuple, typename... Ty,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) insert(Tuple &&t, Ty &&...val)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<Tuple>>;

    return ::std::tuple_cat(
        split<N>(::std::forward<Tuple>(t)),
        ::std::make_tuple(::std::forward<Ty>(val)...),
        split<N, Size>(::std::forward<Tuple>(t)));
}

template <typename Tuple, typename... Ty,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) push_back(Tuple &&t, Ty &&...val)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<Tuple>>;

    return insert<Size>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

template <typename Tuple, typename... Ty,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) push_front(Tuple &&t, Ty &&...val)
{
    return insert<0>(::std::forward<Tuple>(t), ::std::forward<Ty>(val)...);
}

template <size_t N, typename TupleA, typename TupleB,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<TupleA>>>>
constexpr decltype(auto) insert_tuple(TupleA &&t, TupleB &&val)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<TupleA>>;

    return ::std::tuple_cat(
        split<N>(::std::forward<TupleA>(t)),
        to_tuple(::std::forward<TupleB>(val)),
        split<N, Size>(::std::forward<TupleA>(t)));
}
template <typename TupleA, typename TupleB,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<TupleA>>>>
constexpr decltype(auto) append_tuple(TupleA &&t, TupleB &&val)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<TupleA>>;

    return insert_tuple<Size>(::std::forward<TupleA>(t), ::std::forward<TupleB>(val));
}
template <typename TupleA, typename TupleB,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<TupleA>>>>
constexpr decltype(auto) prepend_tuple(TupleA &&t, TupleB &&val)
{
    return insert_tuple<0>(::std::forward<TupleA>(t), ::std::forward<TupleB>(val));
}

template <size_t N, typename T, typename Tuple, typename... Ty,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) emplace(Tuple &&t, Ty &&...val)
{
    return insert<N>(::std::forward<Tuple>(t), T(::std::forward<Ty>(val)...));
}

template <typename T, typename Tuple, typename... Ty,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) emplace_back(Tuple &&t, Ty &&...val)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<Tuple>>;

    return emplace<Size, T>(::std::forward<Tuple>(t), T(::std::forward<Ty>(val)...));
}
template <typename T, typename Tuple, typename... Ty,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) emplace_front(Tuple &&t, Ty &&...val)
{
    return emplace<0, T>(::std::forward<Tuple>(t), T(::std::forward<Ty>(val)...));
}

template <size_t N, typename Tuple,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) erase(Tuple &&t)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<Tuple>>;

    return ::std::tuple_cat(
        split<N>(::std::forward<Tuple>(t)),
        split<N + 1, Size>(::std::forward<Tuple>(t)));
}

template <typename Tuple,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) pop_back(Tuple &&t)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<Tuple>>;

    return erase<Size - 1>(::std::forward<Tuple>(t));
}

template <typename Tuple,
          typename = ::std::enable_if_t<is_tuple_v<::std::remove_reference_t<Tuple>>>>
constexpr decltype(auto) pop_front(Tuple &&t)
{
    return erase<0>(::std::forward<Tuple>(t));
}

template <size_t N1, size_t N2, typename Tuple>
constexpr decltype(auto) swap(Tuple &&t)
{
    constexpr size_t Size = tuple_size_v<::std::remove_reference_t<Tuple>>;
    static_assert(N1 < Size && N2 < Size, "N must be less than tuple size");

    if constexpr (N1 == N2)
    {
        return ::std::forward<Tuple>(t);
    }
    else
    {
        constexpr size_t MinN = (N1 <= N2) ? N1 : N2;
        constexpr size_t MaxN = (N1 > N2) ? N1 : N2;
        return [&]<size_t... Is>(::std::index_sequence<Is...>)
        {
            return ::std::make_tuple(
                EMBMartin::get<index_swapper_v<Is, MinN, MaxN>>(::std::forward<Tuple>(t))...);
        }(::std::make_index_sequence<Size>());
    }
}

//------------------------------------------ 高级管道运算 --------------------------------------------------

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

// 对多返回值的解包处理模式
enum class _PipeUnpackMode
{
    NoUnpack = 0,
    OnlyStdTuple = 1,
    OnlyTupleLike = 2,
    AllTuplesConcept = 3
};

template <_PipeUnpackMode MODE>
struct _PipeUnpackModeManiper
{
};

template <size_t Start, size_t End, size_t Step>
struct _PipeSlicer
{
    static_assert(Step != 0);
    static_assert((End <= Start && Step > 0) || (End >= Start && Step < 0));
};

template <size_t N, typename T>
struct _PipeInserter
{
    T val;
};

template <size_t N>
struct _PipeEraser
{
};

template <size_t N1, size_t N2>
struct _PipeSwapper
{
};

// 管道运算封装类
template <_PipeUnpackMode MODE, typename TUPLE>
class _PipeWrapper
{
private:
    TUPLE _args;

    constexpr _PipeWrapper() = default;

public:
    template <_PipeUnpackMode _MODE, typename _TUPLE>
    friend class _PipeWrapper;

    template <typename... ARGS,
              typename = std::enable_if_t<std::is_constructible_v<TUPLE, ARGS &&...>>>
    explicit constexpr inline _PipeWrapper(ARGS &&...args) noexcept
        : _args(std::forward<ARGS>(args)...) {}

    template <_PipeUnpackMode M = MODE, typename = std::enable_if_t<M == _PipeUnpackMode::OnlyStdTuple>>
    explicit constexpr inline _PipeWrapper(const TUPLE &tup) noexcept
        : _args(tup) {}

    template <size_t N>
    constexpr decltype(auto) get() const & noexcept
    {
        return std::get<N>(this->_args);
    }
    template <size_t N>
    constexpr decltype(auto) get() & noexcept
    {
        return std::get<N>(this->_args);
    }
    template <size_t N>
    constexpr decltype(auto) get() && noexcept
    {
        return std::get<N>(std::move(this->_args));
    }
    template <size_t N>
    constexpr decltype(auto) get() const && noexcept
    {
        return std::get<N>(std::move(this->_args));
    }

    template <typename FUNC, typename = std::enable_if_t<is_invocable_with_std_tuple_v<FUNC, TUPLE>>>
    constexpr auto operator|(FUNC &&func) const
    {
        if constexpr (std::is_same_v<decltype(std::apply(func, this->_args)), void>)
        {
            std::apply(
                std::forward<decltype(func)>(func),
                std::move(this->_args));

            return _PipeWrapper<MODE, std::tuple<>>();
        }
        else
        {
            auto &&rslt = std::apply(
                std::forward<decltype(func)>(func),
                std::move(this->_args));

            using rslt_type = std::remove_reference_t<decltype(rslt)>;

            if constexpr (
                MODE == _PipeUnpackMode::OnlyStdTuple &&
                is_std_tuple_v<rslt_type>)
            {
                return _PipeWrapper<MODE, rslt_type>(std::move(rslt));
            }
            else if constexpr (
                MODE == _PipeUnpackMode::OnlyTupleLike &&
                is_tuple_like_v<rslt_type>)
            {
                auto tuple_val = make_to_tuple(std::move(rslt));

                using rslt_tup_type = std::remove_reference_t<decltype(tuple_val)>;

                auto temp = _PipeWrapper<MODE, rslt_tup_type>();
                temp._args = std::move(tuple_val);
                return temp;
            }
            else if constexpr (
                MODE == _PipeUnpackMode::AllTuplesConcept &&
                is_tuple_v<rslt_type>)
            {
                auto tuple_val = make_to_tuple(std::move(rslt));

                using rslt_tup_type = std::remove_reference_t<decltype(tuple_val)>;

                auto temp = _PipeWrapper<MODE, rslt_tup_type>();
                temp._args = std::move(tuple_val);
                return temp;
            }
            else
            {
                return _PipeWrapper<MODE, std::tuple<rslt_type>>(std::forward<decltype(rslt)>(rslt));
            }
        }
    }

    template <_PipeUnpackMode NEW_MODE>
    constexpr auto operator|(_PipeUnpackModeManiper<NEW_MODE>) const
    {
        auto temp = _PipeWrapper<NEW_MODE, TUPLE>{};
        temp._args = std::move(this->_args);
        return temp;
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
 * @brief 创建管道入口, 当且仅当本次不解包元组; 若希望入口即解包，请使用 pipe_tuple 或在外部先解包
 *
 * @tparam ARGS 首次传入的参数类型
 * @param args 首次传入的参数
 * @return _PipeWrapper 持有参数元组的管道包装器
 *
 *
 * 使用示例：
 * ```cpp
 * // 基本用法：单值进入管道, 用结构化绑定接收内部值
 * const auto &[x] = pipe (3) | [](int x) { return x + 1; };
 * console.showln (x); // 输出 4
 *
 * // 组合多步：返回 std::tuple 时自动解包传递
 * auto pw = pipe (1, 2)
 *          | [](int a, int b) { return std::make_tuple (a + b, a * b); } // 3 2
 *          | [](auto a, auto b) { return std::make_tuple (a * b, a + b); };  // 6 5
 *          | [&](auto&&... args){ console.showsep (args...); } // 输出 6 5
 *
 * // 可以切换为更激进的解包模式
 *
 * struct add_result // 聚合类
 * {
 *    int sum;
 *    bool overflow;
 * };
 * add_result add (int x, int y)
 * {
 *   int s = x + y;
 *   return {s, (s < x) ? 1 : 0};
 * }
 * auto _ = pw
 *         | unpack_tuples_concept // 解包所有可解包对象（所有广义元组对象）
 *         | add // 自动解包 add_result 传递
 *         | [&](auto&&... args){ console.showsep (args...); } // 输出 11 0
 * ```
 */
template <typename... ARGS>
constexpr decltype(auto) pipe(ARGS &&...args) noexcept
{
    return detail::_PipeWrapper<detail::_PipeUnpackMode::OnlyStdTuple, std::tuple<ARGS...>>(std::forward<ARGS>(args)...);
}

/**
 * @brief 以 std::tuple 作为入口并立即按元素解包传递给后续函数
 *
 * @tparam ARGS 元组内的参数类型
 * @param args 入口元组
 * @return _PipeWrapper 持有已解包视角的管道包装器
 *
 * @note 入口即使用 `OnlyStdTuple` 模式，允许后续直接以形参列表接收。
 *
 * 使用示例：
 * ```cpp
 * std::tuple<int, int> tup {3, 4};
 *
 * // 入口即解包：lambda 直接拿到 3, 4
 * auto rs = pipe_tuple (tup)
 *          | [](int a, int b) { return a + b; }; // => 7
 *
 * ```
 */
template <typename... ARGS>
constexpr decltype(auto) pipe_tuple(const std::tuple<ARGS...> &args) noexcept
{
    return detail::_PipeWrapper<detail::_PipeUnpackMode::OnlyStdTuple, std::tuple<ARGS...>>(args);
}

// ====== 管道适配器 ======

// 不解包
constexpr detail::_PipeUnpackModeManiper<detail::_PipeUnpackMode::NoUnpack> no_unpack{};
// 仅解包 std::tuple
constexpr detail::_PipeUnpackModeManiper<detail::_PipeUnpackMode::OnlyStdTuple> unpack_std_tuple{};
// 解包所有 tuple-like 对象
constexpr detail::_PipeUnpackModeManiper<detail::_PipeUnpackMode::OnlyTupleLike> unpack_tuple_like{};
// 解包所有符合广义元组概念的对象 ( 即所有可解包对象 )
constexpr detail::_PipeUnpackModeManiper<detail::_PipeUnpackMode::AllTuplesConcept> unpack_tuples_concept{};

// Unix 风格管道运算符
template <typename T, typename FUNC,
          typename = std::enable_if_t<!detail::is_pipe_v<std::remove_reference_t<T>>>>
constexpr decltype(auto) operator|(T &&val, FUNC &&func)
{
    return func(std::forward<T>(val));
}
EMBMARTIN_NAMESPACE_END

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

#endif // EMBMARTIN_FUNCTIONAL_PROGRAMMING_H