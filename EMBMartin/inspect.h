/**
 * @file inspect.h
 * @author 孙鸣淼
 * @brief 基于编译器方言实现，魔↗术↓技↑巧↘般的 C++17 内省库
 * @version 0.1
 * @date 2026-09-25
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <array>
#include <string_view>
#include <typeinfo>
#include <memory>
#include <cstdlib>
#include <numeric>

#if defined(__GNUC__) || defined(__clang__)
#if defined(__has_include)
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define TN_HAS_CXA_DEMANGLE 1
#else
#define TN_HAS_CXA_DEMANGLE 0
#endif
#else
#define TN_HAS_CXA_DEMANGLE 0
#endif
#endif

#include "macro.h"
#include "functional.h"

EMBMARTIN_NAMESPACE_BEGIN

// ------------------------------------------编译时获取类型名--------------------------------------------------

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

// constexpr 计算 C 字符串长度
constexpr std::size_t cstrlen(const char *s)
{
    std::size_t n = 0;
    while (s[n] != '\0')
        ++n;
    return n;
}

// constexpr 查找子串，返回起始索引，找不到返回 std::string_view::npos
constexpr std::size_t find_substr(const char *str, std::size_t str_len,
                                  const char *sub, std::size_t sub_len,
                                  std::size_t start = 0)
{
    if (sub_len == 0)
        return start;
    if (start >= str_len)
        return std::string_view::npos;
    for (std::size_t i = start; i + sub_len <= str_len; ++i)
    {
        bool match = true;
        for (std::size_t j = 0; j < sub_len; ++j)
        {
            if (str[i + j] != sub[j])
            {
                match = false;
                break;
            }
        }
        if (match)
            return i;
    }
    return std::string_view::npos;
}

template <typename F>
struct function_signature_type
{
    using type = F;
};

template <typename R, typename... Args>
struct function_signature_type<R (*)(Args...)>
{
    using type = R(Args...);
};

template <typename R, typename... Args>
struct function_signature_type<R (*)(Args...) noexcept>
{
    using type = R(Args...) noexcept;
};

template <typename C, typename R, typename... Args>
struct function_signature_type<R (C::*)(Args...) const noexcept>
{
    using type = R(Args...) const noexcept;
};

template <typename C, typename R, typename... Args>
struct function_signature_type<R (C::*)(Args...) noexcept>
{
    using type = R(Args...) noexcept;
};

template <typename C, typename R, typename... Args>
struct function_signature_type<R (C::*)(Args...) const>
{
    using type = R(Args...) const;
};

template <typename C, typename R, typename... Args>
struct function_signature_type<R (C::*)(Args...)>
{
    using type = R(Args...);
};

template <typename F>
using function_signature_type_t = typename function_signature_type<F>::type;

template <typename F>
struct function_owner
{
    static constexpr bool is_member = false;
    using type = void;
};

template <typename C, typename R, typename... Args>
struct function_owner<R (C::*)(Args...) const noexcept>
{
    static constexpr bool is_member = true;
    using type = C;
};

template <typename C, typename R, typename... Args>
struct function_owner<R (C::*)(Args...) noexcept>
{
    static constexpr bool is_member = true;
    using type = C;
};

template <typename C, typename R, typename... Args>
struct function_owner<R (C::*)(Args...) const>
{
    static constexpr bool is_member = true;
    using type = C;
};

template <typename C, typename R, typename... Args>
struct function_owner<R (C::*)(Args...)>
{
    static constexpr bool is_member = true;
    using type = C;
};

EMBMARTIN_DETAIL_NAMESPACE_END

// 编译期类型名提取（依赖编译器宏）
template <typename T>
constexpr std::string_view type_name()
{
#if defined(__clang__)
    constexpr const char *func = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix = "[T = ";
    constexpr std::size_t func_len = detail::cstrlen(func);
    constexpr std::size_t prefix_len = prefix.size();
    constexpr std::size_t start = detail::find_substr(func, func_len, prefix.data(), prefix_len);
    if (start == std::string_view::npos)
        return {};
    constexpr std::size_t type_start = start + prefix_len;
    // 从后向前找最后一个 ']'，它标志函数签名结束
    std::size_t end = func_len;
    while (end > type_start && func[end - 1] != ']')
    {
        --end;
    }
    if (end > type_start)
    {
        --end; // 去掉结尾的 ']'
    }
    return std::string_view(func + type_start, end - type_start);

#elif defined(__GNUC__)
    constexpr const char *func = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix = "with T = ";
    constexpr std::size_t func_len = detail::cstrlen(func);
    constexpr std::size_t prefix_len = prefix.size();
    constexpr std::size_t start = detail::find_substr(func, func_len, prefix.data(), prefix_len);
    if (start == std::string_view::npos)
        return {};
    constexpr std::size_t type_start = start + prefix_len;
    // 先找分号，若找到则类型名到分号前
    std::size_t end = type_start;
    while (end < func_len && func[end] != ';')
    {
        ++end;
    }
    if (end == func_len)
    {
        // 没有分号，则找最后一个 ']'，类型名到它之前
        end = func_len;
        while (end > type_start && func[end - 1] != ']')
        {
            --end;
        }
        if (end > type_start)
        {
            --end;
        }
    }
    std::string_view type_name_result(func + type_start, end - type_start);
    constexpr std::string_view tag_struct = "struct ";
    constexpr std::string_view tag_class = "class ";
    constexpr std::string_view tag_enum = "enum ";
    if (type_name_result.size() >= tag_struct.size() &&
        type_name_result.substr(0, tag_struct.size()) == tag_struct)
        return type_name_result.substr(tag_struct.size());
    if (type_name_result.size() >= tag_class.size() &&
        type_name_result.substr(0, tag_class.size()) == tag_class)
        return type_name_result.substr(tag_class.size());
    if (type_name_result.size() >= tag_enum.size() &&
        type_name_result.substr(0, tag_enum.size()) == tag_enum)
        return type_name_result.substr(tag_enum.size());
    return type_name_result;

#elif defined(_MSC_VER)
    constexpr const char *func = __FUNCSIG__;
    constexpr std::string_view prefix = "type_name<";
    constexpr std::string_view suffix = ">(void)";
    constexpr std::size_t func_len = detail::cstrlen(func);
    constexpr std::size_t prefix_len = prefix.size();
    constexpr std::size_t suffix_len = suffix.size();
    constexpr std::size_t start = detail::find_substr(func, func_len, prefix.data(), prefix_len);
    if (start == std::string_view::npos)
        return {};
    constexpr std::size_t type_start = start + prefix_len;
    constexpr std::size_t end = detail::find_substr(func, func_len, suffix.data(), suffix_len, type_start);
    if (end == std::string_view::npos)
        return {};
    std::string_view type_name_result(func + type_start, end - type_start);
    constexpr std::string_view tag_struct = "struct ";
    constexpr std::string_view tag_class = "class ";
    constexpr std::string_view tag_enum = "enum ";
    if (type_name_result.size() >= tag_struct.size() &&
        type_name_result.substr(0, tag_struct.size()) == tag_struct)
        return type_name_result.substr(tag_struct.size());
    if (type_name_result.size() >= tag_class.size() &&
        type_name_result.substr(0, tag_class.size()) == tag_class)
        return type_name_result.substr(tag_class.size());
    if (type_name_result.size() >= tag_enum.size() &&
        type_name_result.substr(0, tag_enum.size()) == tag_enum)
        return type_name_result.substr(tag_enum.size());
    return type_name_result;

#else
#error "Unsupported compiler"
#endif
}

// ------------------------------------------编译时获取函数签名或函数名--------------------------------------------------


/**
 * @brief 编译期获取函数签名
 * 
 * @tparam F 函数指针
 * @return constexpr std::string_view 
 */
template <typename F>
constexpr std::string_view signature()
{
    return type_name<detail::function_signature_type_t<F>>();
}

/**
 * @brief 编译期获取函数名称
 * 
 * @tparam F 函数指针
 * @return constexpr std::string_view 
 */
template <auto F>
constexpr std::string_view function_name()
{
    (void)F;
#if defined(_MSC_VER)
    constexpr std::string_view raw = __FUNCSIG__;
#else
    constexpr std::string_view raw = __PRETTY_FUNCTION__;
#endif
    constexpr std::size_t len = raw.size();

#if defined(_MSC_VER)
    // MSVC expands __FUNCSIG__ here to the pointed-to function signature.
    std::size_t end = 0;
    while (end < len && raw[end] != '(')
        ++end;
    if (end == len)
        return {};
    while (end > 0 && raw[end - 1] == ' ')
        --end;
    std::size_t start = end;
    while (start > 0)
    {
        const char c = raw[start - 1];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == ':' || c == '~')
            --start;
        else
            break;
    }
    return raw.substr(start, end - start);
#else
    constexpr std::string_view prefix = "F = ";
    constexpr std::size_t prefix_pos = detail::find_substr(raw.data(), len,
                                                            prefix.data(), prefix.size());
    if (prefix_pos == std::string_view::npos)
        return {};
    std::size_t start = prefix_pos + prefix.size();
    std::size_t end = start;
    while (end < len && raw[end] != ']' && raw[end] != ';')
        ++end;
#endif

    while (start < end && (raw[start] == '&' || raw[start] == ' '))
        ++start;
    while (end > start && raw[end - 1] == ' ')
        --end;

    // Keep only the unqualified identifier (e.g. main from &main).
    std::size_t name_start = start;
    for (std::size_t i = start; i + 1 < end; ++i)
    {
        if (raw[i] == ':' && raw[i + 1] == ':')
            name_start = i + 2;
    }
    return raw.substr(name_start, end - name_start);
}

template <auto F>
struct function_signature_storage
{
    using function_type = detail::function_signature_type_t<decltype(F)>;
    static constexpr std::string_view type = type_name<function_type>();
    static constexpr std::string_view name = function_name<F>();
    static constexpr bool is_member = detail::function_owner<decltype(F)>::is_member;
    static constexpr std::size_t open = detail::find_substr(
        type.data(), type.size(), "(", 1);
    static constexpr std::size_t owner_size = is_member
        ? type_name<typename detail::function_owner<decltype(F)>::type>().size() + 2
        : 0;
    static constexpr std::size_t size = type.size() + name.size() + 1 + owner_size;

    static constexpr std::array<char, size> make()
    {
        std::array<char, size> result{};
        std::size_t index = 0;
        for (std::size_t i = 0; i < open; ++i)
            result[index++] = type[i];
        result[index++] = ' ';
        if constexpr (is_member)
        {
            constexpr auto owner = type_name<typename detail::function_owner<decltype(F)>::type>();
            for (char c : owner)
                result[index++] = c;
            result[index++] = ':';
            result[index++] = ':';
        }
        for (char c : name)
            result[index++] = c;
        for (std::size_t i = open; i < type.size(); ++i)
            result[index++] = type[i];
        return result;
    }

    inline static constexpr auto value = make();
};

/**
 * @brief 编译期获取带函数名的完整函数签名
 *
 * @tparam F 函数指针或成员函数指针值
 * @return constexpr std::string_view
 */
template <auto F>
constexpr std::string_view signature()
{
    constexpr auto &value = function_signature_storage<F>::value;
    return std::string_view(value.data(), value.size());
}

// ------------------------------------------获取枚举名--------------------------------------------------

// 默认搜索范围，可被调用方在包含 inspect.h 之前用宏覆盖
#ifndef EMBMARTIN_ENUM_DEFAULT_MIN
#define EMBMARTIN_ENUM_DEFAULT_MIN (0)
#endif
#ifndef EMBMARTIN_ENUM_DEFAULT_MAX
#define EMBMARTIN_ENUM_DEFAULT_MAX (32)
#endif

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

// 默认搜索范围对象。放在命名空间作用域，才能作为非类型模板实参。
// 用 inline constexpr 保证多 TU 单一定义（C++17 起支持 inline 变量）。
inline constexpr Slice<size_t> default_enum_range{
    static_cast<size_t>(EMBMARTIN_ENUM_DEFAULT_MIN),
    static_cast<size_t>(EMBMARTIN_ENUM_DEFAULT_MAX)};

// 从 __PRETTY_FUNCTION__ / __FUNCSIG__ 中提取枚举值名
// - GCC :  "... [with E = LogLevel; V = LogLevel::INFO]"
// - Clang / ARMCLANG: "... [E = LogLevel, V = LogLevel::INFO]"
// - MSVC : "... enum_name_impl<enum LogLevel,LogLevel::INFO>(void)"
// 未命名值（如 (LogLevel)5）返回空 string_view
template <typename E, auto V>
constexpr std::string_view enum_name_impl()
{
    static_assert(std::is_same_v<std::decay_t<decltype(V)>, E>,
                  "enum_name_impl: V must be of enum type E");
#if defined(_MSC_VER)
    constexpr std::string_view raw = __FUNCSIG__;
#else
    constexpr std::string_view raw = __PRETTY_FUNCTION__;
#endif
    constexpr std::size_t len = raw.size();

#if defined(_MSC_VER)
    constexpr const char *prefix = "enum_name_impl<";
    constexpr std::size_t prefix_len = 16;
    constexpr std::size_t p = find_substr(raw.data(), len, prefix, prefix_len);
    if (p == std::string_view::npos)
        return {};
    constexpr std::size_t value_start = p + prefix_len;
    constexpr std::size_t comma =
        find_substr(raw.data() + value_start, len - value_start, ",", 1);
    if (comma == std::string_view::npos)
        return {};
    constexpr std::size_t name_start = value_start + comma + 1;
    constexpr const char *suffix = ">(void)";
    constexpr std::size_t suffix_len = 7;
    constexpr std::size_t e =
        find_substr(raw.data() + name_start, len - name_start, suffix, suffix_len);
    if (e == std::string_view::npos)
        return {};
    const std::string_view value_str = raw.substr(name_start, e);
#else
    constexpr const char *prefix = "V = ";
    constexpr std::size_t prefix_len = 4;
    constexpr std::size_t p = find_substr(raw.data(), len, prefix, prefix_len);
    if (p == std::string_view::npos)
        return {};
    constexpr std::size_t value_start = p + prefix_len;
    std::size_t end = value_start;
    while (end < len && raw[end] != ']' && raw[end] != ';' && raw[end] != '>')
        ++end;
    const std::string_view value_str = raw.substr(value_start, end - value_start);
#endif

    // 未命名值判定：形如 "(LogLevel)5" 的整型常量
    for (std::size_t i = 0; i < value_str.size(); ++i)
        if (value_str[i] == '(')
            return {};

    // 取最后一个 "::" 之后的标识符
    std::size_t pos = std::string_view::npos;
    for (std::size_t i = 0; i + 2 <= value_str.size(); ++i)
        if (value_str[i] == ':' && value_str[i + 1] == ':')
            pos = i;
    return (pos == std::string_view::npos) ? value_str : value_str.substr(pos + 2);
}

// 单个 Slice 范围对应的名字表
template <typename E, auto Range>
struct enum_range_table
{
    static constexpr long long lo = static_cast<long long>(Range.start);
    static constexpr long long hi = static_cast<long long>(Range.stop);
    static constexpr long long st = static_cast<long long>(Range.step);

    static constexpr std::size_t N =
        (st > 0 && hi > lo) ? static_cast<std::size_t>((hi - lo + st - 1) / st) :
        (st < 0 && lo > hi) ? static_cast<std::size_t>((lo - hi + (-st) - 1) / (-st)) :
                              std::size_t{0};

    template <std::size_t... Is>
    static constexpr std::array<std::string_view, N>
    make(std::index_sequence<Is...>)
    {
        return {
            enum_name_impl<E,
                static_cast<E>(lo + static_cast<long long>(Is) * st)>()...
        };
    }

    static constexpr auto value = make(std::make_index_sequence<N>{});
};

// 在单个 Slice 范围内按值查找
template <typename E, auto Range>
constexpr std::string_view search_by_value(std::underlying_type_t<E> v)
{
    constexpr auto &tbl = enum_range_table<E, Range>::value;
    constexpr long long lo = static_cast<long long>(Range.start);
    constexpr long long st = static_cast<long long>(Range.step);
    const long long target = static_cast<long long>(v);

    for (std::size_t i = 0; i < tbl.size(); ++i)
    {
        if (lo + static_cast<long long>(i) * st == target && !tbl[i].empty())
            return tbl[i];
    }
    return {};
}

// 在单个 Slice 范围内按名字查找
template <typename E, auto Range>
constexpr std::pair<bool, E> search_by_name(std::string_view name)
{
    constexpr auto &tbl = enum_range_table<E, Range>::value;
    constexpr long long lo = static_cast<long long>(Range.start);
    constexpr long long st = static_cast<long long>(Range.step);

    for (std::size_t i = 0; i < tbl.size(); ++i)
    {
        if (!tbl[i].empty() && tbl[i] == name)
            return { true, static_cast<E>(lo + static_cast<long long>(i) * st) };
    }
    return { false, static_cast<E>(0) };
}

// 折叠辅助：找到就停
template <typename E, auto Range>
constexpr bool try_name(E value, std::string_view &out)
{
    auto n = search_by_value<E, Range>(
        static_cast<std::underlying_type_t<E>>(value));
    if (!n.empty())
    {
        out = n;
        return true;
    }
    return false;
}

template <typename E, std::size_t... Is>
constexpr bool try_default_name_impl(E value, std::string_view &out,
                                     std::index_sequence<Is...>)
{
    bool found = false;
    const long long target = static_cast<long long>(
        static_cast<std::underlying_type_t<E>>(value));
    (( !found && target == static_cast<long long>(default_enum_range.start) +
                    static_cast<long long>(Is) * default_enum_range.step
        ? (out = enum_name_impl<E, static_cast<E>(
                    default_enum_range.start + Is * default_enum_range.step)>(), found = true)
        : false), ...);
    return found;
}

template <typename E>
constexpr bool try_default_name(E value, std::string_view &out)
{
    return try_default_name_impl<E>(
        value, out, std::make_index_sequence<
            (default_enum_range.stop - default_enum_range.start) /
            default_enum_range.step>{});
}

template <typename E, auto Range>
constexpr bool try_cast(std::string_view name, E &out)
{
    auto r = search_by_name<E, Range>(name);
    if (r.first)
    {
        out = r.second;
        return true;
    }
    return false;
}

template <typename E, std::size_t... Is>
constexpr bool try_default_cast_impl(std::string_view name, E &out,
                                     std::index_sequence<Is...>)
{
    bool found = false;
    (( !found && enum_name_impl<E, static_cast<E>(
                    default_enum_range.start + Is * default_enum_range.step)>() == name
        ? (out = static_cast<E>(default_enum_range.start +
                                Is * default_enum_range.step), found = true)
        : false), ...);
    return found;
}

template <typename E>
constexpr bool try_default_cast(std::string_view name, E &out)
{
    return try_default_cast_impl<E>(
        name, out, std::make_index_sequence<
            (default_enum_range.stop - default_enum_range.start) /
            default_enum_range.step>{});
}

template <typename E, long long Start, long long Stop, std::size_t... Is>
constexpr bool try_integer_range_impl(E value, std::string_view &out,
                                      std::index_sequence<Is...>)
{
    bool found = false;
    const long long target = static_cast<long long>(
        static_cast<std::underlying_type_t<E>>(value));
    (( !found && target == Start + static_cast<long long>(Is)
        ? (out = enum_name_impl<E, static_cast<E>(Start + static_cast<long long>(Is))>(), found = true)
        : false), ...);
    return found;
}

template <typename E, long long Start, long long Stop>
constexpr bool try_integer_range(E value, std::string_view &out)
{
    static_assert(Stop >= Start, "enum_name range stop must not be less than start");
    return try_integer_range_impl<E, Start, Stop>(
        value, out, std::make_index_sequence<static_cast<std::size_t>(Stop - Start)>{});
}

template <typename E, auto... Bounds>
struct integer_range_search;

template <typename E>
struct integer_range_search<E>
{
    static constexpr bool find(E, std::string_view &)
    {
        return false;
    }
};

template <typename E, auto Start, auto Stop, auto... Rest>
struct integer_range_search<E, Start, Stop, Rest...>
{
    static constexpr bool find(E value, std::string_view &out)
    {
        if (try_integer_range<E, static_cast<long long>(Start),
                              static_cast<long long>(Stop)>(value, out))
            return true;
        return integer_range_search<E, Rest...>::find(value, out);
    }
};

EMBMARTIN_DETAIL_NAMESPACE_END

// ---------- 对外接口 ----------

/**
 * @brief 编译期把枚举值转成名字
 *
 * @tparam E       枚举类型
 * @tparam Bounds  零个或多个整数，每两个整数表示一个半开区间 [start, stop)。
 *                 不传时使用 EMBMARTIN_ENUM_DEFAULT_MIN/MAX 指定的默认范围。
 * @param  value   枚举值
 * @return constexpr std::string_view  未找到返回空视图
 */
template <auto... Bounds, typename E>
constexpr std::string_view enum_name(E value)
{
    static_assert(std::is_enum_v<E>, "enum_name: E must be an enum type");
    static_assert(sizeof...(Bounds) % 2 == 0,
                  "enum_name bounds must contain start/stop pairs");

    std::string_view result;
    if constexpr (sizeof...(Bounds) == 0)
        (void)detail::try_default_name<E>(value, result);
    else
        (void)detail::integer_range_search<E, Bounds...>::find(value, result);
    return result;
}

/**
 * @brief 编译期把名字转成枚举值
 *
 * @tparam E             枚举类型
 * @tparam Ranges        零个或多个 constexpr Slice 对象；
 *                       不传时使用默认范围
 * @param  name          枚举名字符串
 * @param  default_value 未找到时返回该值
 */
template <auto... Ranges, typename E>
constexpr E enum_cast(std::string_view name,
                      E default_value = static_cast<E>(0))
{
    static_assert(std::is_enum_v<E>, "enum_cast: E must be an enum type");

    E result = default_value;
    if constexpr (sizeof...(Ranges) == 0)
        (void)detail::try_default_cast<E>(name, result);
    else
        (void)(detail::try_cast<E, Ranges>(name, result) || ...);
    return result;
}

/**
 * @brief 判断名字是否在给定范围内存在；不传范围时使用默认范围
 */
template <auto... Ranges, typename E>
constexpr bool enum_contains(std::string_view name)
{
    static_assert(std::is_enum_v<E>, "enum_contains: E must be an enum type");

    E dummy = static_cast<E>(0);
    if constexpr (sizeof...(Ranges) == 0)
        return detail::try_default_cast<E>(name, dummy);
    else
        return (detail::try_cast<E, Ranges>(name, dummy) || ...);
}

EMBMARTIN_NAMESPACE_END