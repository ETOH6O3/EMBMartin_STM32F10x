#pragma once

// fmt 后端二：etl_fmt —— 直转发第三方库 ETL 的 format。
// 自 fmt.h 拆出；fmt.h 负责按原有顺序汇总包含各后端。

#include "EMBMartin/lang/fmt/fmt_config.h"
#include "EMBMartin/lang/fmt/fmt_helpers.h"

EMBMARTIN_NAMESPACE_BEGIN

// #################################################################################################
// # 后端二：etl_fmt —— 直转发第三方库 ETL 的 format
//
// 本后端不实现任何格式化核心：说明符解析、参数存储、各类型的格式化全部由 ETL 完成。
// 这里只做三件事：
//   1. 把「字面量 / 运行期 string_view」两种格式串入口收敛到 ETL 的 vformat_to；
//   2. 用 ETL 的 args_mask 把本次调用不可能命中的分支裁掉，避免无谓的代码膨胀；
//   3. 把 ETL 的类型别名搬到本命名空间，使两套后端的对外名字一致。
// #################################################################################################

/**
 * @namespace EMBMartin::etl_fmt
 * @brief ETL 直转发后端
 *
 * @details 本命名空间在 @c EMBMARTIN_FMT_USE_ETL 为真时是**内联命名空间**，因此
 * 其中的名字可直接以 @c EMBMartin:: 访问。所有函数都是对 @c etl:: 对应设施的薄包装，
 * 不引入任何本库自有的格式化逻辑。
 */
EMBMARTIN_FMT_INLINE_ETL namespace etl_fmt
{
	// ------------------------------------------ETL 类型别名--------------------------------------------------

	/** @brief ETL 的格式化实参类型（见 @c etl::basic_format_arg ） */
	using etl::basic_format_arg;
	/** @brief ETL 的格式化实参集合类型（见 @c etl::basic_format_args ） */
	using etl::basic_format_args;
	/** @brief ETL 的编译期检查格式串类型（见 @c etl::basic_format_string ） */
	using etl::basic_format_string;
	/** @brief ETL 的单个格式化实参别名 */
	using etl::format_arg;
	/** @brief ETL 的格式化实参集合别名 */
	using etl::format_args;
	/** @brief ETL 的格式串类型（C++20 下带编译期检查） */
	using etl::format_string;
	/** @brief ETL 的格式化器主模板（自定义点） */
	using etl::formatter;
	/** @brief ETL 的实参存储构造器 */
	using etl::make_format_args;

	/** @brief ETL 的解析上下文（@c char 版本） */
	using basic_format_parse_context = etl::basic_format_parse_context<char>;
	/** @brief ETL 的解析上下文别名 */
	using format_parse_context = etl::format_parse_context;

	/**
	 * @brief ETL 的格式上下文
	 * @tparam OutputIt 输出迭代器类型
	 * @tparam CharT    字符类型
	 */
	template <typename OutputIt, typename CharT>
	using basic_format_context = etl::basic_format_context<OutputIt, CharT>;

	/**
	 * @brief ETL 的格式上下文别名
	 * @tparam OutputIt 输出迭代器类型
	 */
	template <typename OutputIt>
	using format_context = etl::format_context<OutputIt>;

	/** @brief 与 ETL 内部同源的「可格式化」探测（要求 @c etl::formatter<T> 同时提供 @c parse 与 @c format ） */
	using etl::private_format::is_formattable;

	/**
	 * @brief @c is_formattable 的变量模板形式
	 * @tparam T 待探测的类型
	 */
	template <typename T>
	constexpr bool is_formattable_v = is_formattable<T>::value;

	// ------------------------------------------ETL 风格入口--------------------------------------------------

	/**
	 * @brief 运行期格式串 → 输出迭代器
	 *
	 * @tparam OutputIt 输出迭代器类型
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 已推进的输出迭代器
	 *
	 * @note 格式串是运行期值，无法走 ETL 的 @c format_string （consteval 构造），
	 *       故直接调用 @c etl::vformat_to 。
	 * @note 这里显式传入 `args_mask<Args...>::value`，配合 @c etl 中面向自定义类型的
	 *       @c arg_type_mask 特化，可显著减小代码体积。
	 */
	template <typename OutputIt, typename... Args>
	OutputIt vformat_to(OutputIt out, etl::string_view fmt_str, const Args &...args)
	{
		auto store = etl::make_format_args<OutputIt>(args...);
		return etl::vformat_to<OutputIt, etl::private_format::args_mask<Args...>::value>(
			out, fmt_str, etl::format_args<OutputIt>(store));
	}

	/**
	 * @brief 运行期格式串 → 输出迭代器
	 *
	 * @tparam OutputIt 输出迭代器类型（不能是 @c etl::istring 的派生类）
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 * @return 已推进的输出迭代器
	 */
	template <typename OutputIt, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to(OutputIt out, etl::string_view fmt_str, const Args &...args)
	{
		return etl_fmt::vformat_to(out, fmt_str, args...);
	}

	/**
	 * @brief 字面量格式串 → 输出迭代器
	 *
	 * @tparam OutputIt 输出迭代器类型（不能是 @c etl::istring 的派生类）
	 * @tparam N        字面量数组长度
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param fmt_str 字符串字面量
	 * @param args    待格式化的实参
	 *
	 * @return 已推进的输出迭代器
	 *
	 * @note C++20 下额外构造 @c etl::format_string 以启用 ETL 的编译期格式串检查；
	 *       C++17 下 @c basic_format_string 不做检查（ETL 的 @c ETL_CONSTEVAL 为空）。
	 */
	template <typename OutputIt, size_t N, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to(OutputIt out, const char (&fmt_str)[N], const Args &...args)
	{
#if ETL_USING_CPP20
		etl::format_string<std::remove_cv_t<Args>...> checked(fmt_str);
		(void)checked;
#endif
		return etl_fmt::vformat_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/**
	 * @brief 最多写 @p n 个字符；返回已推进的输出迭代器（与 ETL 的 @c etl::format_to_n 一致）
	 *
	 * @tparam OutputIt 输出迭代器类型（不能是 @c etl::istring 的派生类）
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param n       最多写入的字符数
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 已推进的输出迭代器
	 *
	 * @note 用 ETL 的 @c limit_iterator 实现截断：写满 @p n 个字符后不再前进。
	 */
	template <typename OutputIt, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to_n(OutputIt out, size_t n, etl::string_view fmt_str, const Args &...args)
	{
		using wrapper_iterator = etl::private_format::limit_iterator<OutputIt>;
		auto store = etl::make_format_args<wrapper_iterator>(args...);
		return etl::vformat_to<wrapper_iterator, etl::private_format::args_mask<Args...>::value>(
				   wrapper_iterator(out, n), fmt_str, etl::format_args<wrapper_iterator>(store))
			.get();
	}

	/**
	 * @brief 字面量格式串的「最多写 @p n 个字符」版本
	 *
	 * @tparam OutputIt 输出迭代器类型（不能是 @c etl::istring 的派生类）
	 * @tparam N        字面量数组长度
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param n       最多写入的字符数
	 * @param fmt_str 字符串字面量
	 * @param args    待格式化的实参
	 * @return 已推进的输出迭代器
	 */
	template <typename OutputIt, size_t N, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to_n(OutputIt out, size_t n, const char (&fmt_str)[N], const Args &...args)
	{
		return etl_fmt::format_to_n(out, n, etl::string_view(fmt_str, N - 1), args...);
	}

	/**
	 * @brief 写入 @c etl::istring （与 ETL 的非标准重载同名同义）
	 *
	 * @tparam Args 实参类型包
	 * @param out     目标字符串
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 指向写入末尾的迭代器
	 *
	 * @note 会调用 @c uninitialized_resize 把 @p out 的长度调整为实际写入长度。
	 */
	template <typename... Args>
	etl::istring::iterator format_to(etl::istring &out, etl::string_view fmt_str, const Args &...args)
	{
		etl::istring::iterator result = etl_fmt::format_to_n(out.begin(), out.max_size(), fmt_str, args...);
		out.uninitialized_resize(static_cast<size_t>(result - out.begin()));
		return result;
	}

	/**
	 * @brief 写入 @c etl::istring ，格式串为字符串字面量
	 *
	 * @tparam N    字面量数组长度
	 * @tparam Args 实参类型包
	 * @param out     目标字符串
	 * @param fmt_str 字符串字面量
	 * @param args    待格式化的实参
	 * @return 指向写入末尾的迭代器
	 */
	template <size_t N, typename... Args>
	etl::istring::iterator format_to(etl::istring &out, const char (&fmt_str)[N], const Args &...args)
	{
		return etl_fmt::format_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/**
	 * @brief 只计算所需字符数（ETL 的 @c formatted_size 对应物）
	 *
	 * @tparam Args 实参类型包
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 完整格式化本应产生的字符数
	 *
	 * @note 用 ETL 的 @c counter_iterator 实现「只计数不输出」，不需要任何栈缓冲。
	 */
	template <typename... Args>
	size_t formatted_size(etl::string_view fmt_str, const Args &...args)
	{
		etl::private_format::counter_iterator it;
		it = etl_fmt::vformat_to(it, fmt_str, args...);
		return it.value();
	}

	/**
	 * @brief 只计算所需字符数（字面量格式串）
	 *
	 * @tparam N    字面量数组长度
	 * @tparam Args 实参类型包
	 * @param fmt_str 字符串字面量
	 * @param args    待格式化的实参
	 * @return 完整格式化本应产生的字符数
	 */
	template <size_t N, typename... Args>
	size_t formatted_size(const char (&fmt_str)[N], const Args &...args)
	{
		return etl_fmt::formatted_size(etl::string_view(fmt_str, N - 1), args...);
	}
} // namespace etl_fmt

EMBMARTIN_NAMESPACE_END
