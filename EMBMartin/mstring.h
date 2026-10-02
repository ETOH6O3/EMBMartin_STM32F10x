/**
 ******************************************************************************
 * @file    mstring.h
 * @author  孙鸣淼
 * @brief   EMBMartin 定长字符串库
 *
 * 本头文件提供适用于单片机的定长字符串类型 String<N> 及其配套容器，
 * 并提供 Python 风格的常用字符串接口。
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 * 3. 本模块不进行任何动态内存分配，不抛出异常；对象大小即为 N 字节
 * 4. 所有可能导致内容超出容量的操作都不截断，而是把内容覆写为溢出错误标记，
 *    详见 mstring.md
 *
 * @note 本头文件刻意不命名为 string.h ：在 ARM 等工具链下它会遮蔽 C 库的 string.h ，
 * 使 <cstring> 无法再引入标准声明（实测仅 #include <cstring> 即报错）
 ******************************************************************************
 */
#ifndef EMBMARTIN_MSTRING_H
#define EMBMARTIN_MSTRING_H

#include <array>
#include <cstddef>
#include <charconv>
#include <string_view>
#include <type_traits>
#include <utility>

#include "macro.h"
#include "meta.h"
#include "fmt.h"

EMBMARTIN_NAMESPACE_BEGIN

/**
 * @brief std::atoi 的 std::string_view 版本
 *
 * 语义与 atoi 一致：
 *   - 跳过前导空白字符
 *   - 识别可选的 '+' / '-' 号
 *   - 解析十进制整数，遇到非数字字符停止
 *   - 无有效数字时返回 0
 *
 * 溢出时返回 INT_MAX / INT_MIN（比标准 atoi 的 UB 更安全）。
 *
 * @note 无内存分配、不抛异常、constexpr 友好（C++20 起 from_chars 可为 constexpr）。
 */
int atoi(const std::string_view& sv) noexcept;

// ------------------------------------------字符串溢出错误标记--------------------------------------------------

/**
 * @brief 溢出错误标记的前缀
 *
 * 溢出错误标记形如 "7<9" ，含义为“可用字符数上限为 7 ，而本次操作至少还需要 9 个字符”。
 * 前缀默认留空：标记本身必须能在最小容量（capacity() 为 7 的 String<8> ）内完整写出，
 * 否则最该被看到的那两个数字反而会被截掉。如需更醒目的标记，可把它定义为 "E:" 等短串，
 * 但请自行确认在最小容量的串里仍能完整显示数字。
 */
#ifndef EMBMARTIN_STRING_ERROR_TAG
#define EMBMARTIN_STRING_ERROR_TAG ""
#endif // EMBMARTIN_STRING_ERROR_TAG

/**
 * @brief 只匹配“真正的指针”的形参类型，用于把 const char * 与 const char[N] 两个重载区分开
 *
 * 直接写两个重载时，实参是字符串字面量 const char[N] ：形参 const char[N] 的数组到指针转换
 * 与形参 const char (&)[N] 的恒等转换在重载决议中同等级别，会导致“二义性”编译错误。
 * 而模板实参必须精确匹配，把形参写成 template 参数 TEXT=const char* 时，实参 const char[N]
 * 无法完成推导（数组到指针转换发生在推导之后），于是该重载被排除，只剩数组引用重载
 *
 * @tparam TEXT 推导得到的形参类型
 */
template <typename TEXT>
using non_array_char_pointer = std::enable_if_t<
	std::is_same_v<std::remove_reference_t<TEXT>, const char *> ||
		std::is_same_v<std::remove_reference_t<TEXT>, char *>,
	TEXT>;

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

/**
 * @brief 编译期字符串字面量长度
 * @param str 以 '\\0' 结尾的字符串
 * @return 不含结尾 '\\0' 的字符个数
 *
 * @note inspect.h 中也有同名的 cstrlen ，二者互相独立：string.h 不依赖功能更重的 inspect.h
 */
constexpr size_t string_literal_length(const char *str) noexcept
{
	size_t length = 0;
	while (str[length] != '\0')
	{
		++length;
	}
	return length;
}

/**
 * @brief 编译期写十进制无符号数
 *
 * 使用“先递归高位、后写低位”的写法，避免在栈上开临时缓冲区
 *
 * @param destination 目标缓冲区
 * @param capacity 目标缓冲区可用字节数
 * @param position 写入位置的引用，成功后自增
 * @param value 待写入的数值
 */
constexpr void write_decimal(char *destination, const size_t capacity, size_t &position, const size_t value) noexcept
{
	if (value >= 10)
	{
		write_decimal(destination, capacity, position, value / 10);
	}
	if (position < capacity)
	{
		destination[position++] = static_cast<char>('0' + value % 10);
	}
}

/**
 * @brief 写溢出错误标记（不含结尾 '\\0' ）
 *
 * 输出格式为 "<EMBMARTIN_STRING_ERROR_TAG><可用字符数上限><<至少还需的字符数>" ，例如 "7<9" 。
 * 若 capacity 小到放不下完整标记，则写出尽可能长的前缀。
 *
 * @param destination 目标缓冲区，长度不少于 capacity
 * @param capacity 目标缓冲区可用字节数
 * @param capacity_value 标记中报告的已有容量
 * @param required 本次操作需要的字节数
 * @return 实际写出的字节数
 */
constexpr size_t write_error_marker(char *destination, const size_t capacity,
									const size_t capacity_value, const size_t required) noexcept
{
	constexpr char tag[] = EMBMARTIN_STRING_ERROR_TAG;
	constexpr size_t tag_length = sizeof(tag) - 1;

	size_t written = 0;
	for (size_t i = 0; i < tag_length && written < capacity; ++i)
	{
		destination[written++] = tag[i];
	}
	write_decimal(destination, capacity, written, capacity_value);
	if (written < capacity)
	{
		destination[written++] = '<';
	}
	write_decimal(destination, capacity, written, required);
	return written;
}

// ------------------------------------------ASCII 字符分类--------------------------------------------------
// 不直接使用 <cctype> 的 isdigit 等函数：它们的参数必须可表示为 unsigned char 或 EOF ，
// 直接传入普通 char 属于未定义行为，且不可用于常量表达式

constexpr bool ascii_digit(const char c) noexcept
{
	return c >= '0' && c <= '9';
}

constexpr bool ascii_alpha(const char c) noexcept
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool ascii_alnum(const char c) noexcept
{
	return ascii_digit(c) || ascii_alpha(c);
}

constexpr bool ascii_space(const char c) noexcept
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

constexpr bool ascii_upper(const char c) noexcept
{
	return c >= 'A' && c <= 'Z';
}

constexpr bool ascii_lower(const char c) noexcept
{
	return c >= 'a' && c <= 'z';
}

constexpr char ascii_to_upper(const char c) noexcept
{
	return ascii_lower(c) ? static_cast<char>(c - ('a' - 'A')) : c;
}

constexpr char ascii_to_lower(const char c) noexcept
{
	return ascii_upper(c) ? static_cast<char>(c + ('a' - 'A')) : c;
}

/**
 * @brief 把 text 覆写为指定容量的溢出错误标记
 *
 * @param str 目标缓冲区
 * @param buffer_size 目标缓冲区总字节数，需不小于 1
 * @return 写出的字符个数（不含结尾 '\\0' ）
 */
constexpr size_t make_overflow_error(char *str, const size_t buffer_size,
									 const size_t capacity_value, const size_t required) noexcept
{
	const size_t written = write_error_marker(str, buffer_size - 1, capacity_value, required);
	str[written] = '\0';
	return written;
}

EMBMARTIN_DETAIL_NAMESPACE_END

// ------------------------------------------浮点数解析结果--------------------------------------------------

/**
 * @brief 浮点数文本解析结果
 *
 * 约定：解析失败或数值超出 float 范围时 ok 为 false ，此时 value 无意义
 */
struct FloatingResult
{
	float value = 0.0f;
	bool ok = false;

	constexpr explicit operator bool() const noexcept
	{
		return ok;
	}
};

// ------------------------------------------定长字符串--------------------------------------------------

/**
 * @brief 定长字符串
 *
 * 内容恒存放在对象内部的 char[N] 中，不做任何动态分配。内部恒保持以 '\\0' 结尾，
 * 且 size() 恒为 '\\0' 之前的字符个数。
 *
 * 容量语义与标准库一致：size() 是当前字符个数，capacity() 是字符个数上限。
 * String<N> 中 N 是内部缓冲区的总字节数，故最多容纳 N - 1 个字符，
 * 即 String<8> 与 std::array<char, 8> 占用同样大小，可存放 7 个字符。
 *
 * 与 std::string 的关键差异：
 * 1. 不截断：任何会使 size() 超过 capacity() 的操作都不丢弃多出来的内容，
 *    而是把整个内容覆写为溢出错误标记（见 EMBMARTIN_STRING_ERROR_TAG ），
 *    使溢出在显示到控制台时立即暴露，而不是静默丢失数据；
 * 2. 迭代器是 const char * ：内容只能经由成员函数修改，以保证 '\\0' 结尾不变量；
 * 3. 不抛异常：越界访问由 at() 返回 '\\0' 表达，查找失败由 npos 表达。
 *
 * @tparam N 内部缓冲区总字节数（含结尾 '\\0' ），至少为 8
 */
template <size_t N = 128>
class String
{
	static_assert(N >= 8, "String<N> 的缓冲区至少需要 8 字节");

public:
	// ------------------------------------------类型别名--------------------------------------------------

	using value_type = char;
	using traits_type = std::char_traits<char>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = char &;
	using const_reference = const char &;
	using pointer = char *;
	using const_pointer = const char *;
	/** @brief 迭代器为只读指针，见类注释第 2 条 */
	using iterator = const char *;
	using const_iterator = const char *;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	static constexpr size_type npos = std::string_view::npos;

private:
	// ------------------------------------------数据成员--------------------------------------------------

	/**
	 * @brief 内部缓冲区（含结尾 '\\0' ）
	 *
	 * 命名为 _str 而非 _buffer ：本类对外提供 buffer() 接口，避免重名造成混淆
	 */
	char _str[N];

	/** @brief 当前字符个数（不含结尾 '\\0' ） */
	size_type _size;

	// ------------------------------------------内部工具--------------------------------------------------

	/** @brief 把内容覆写为溢出错误标记 */
	constexpr void _report_overflow(const size_type required) noexcept
	{
		this->_size = detail::make_overflow_error(this->_str, N, N - 1, required);
	}

	/** @brief 把内容覆写为格式错误标记，仅供格式化失败时使用 */
	constexpr void _report_format_error() noexcept
	{
		constexpr char marker[] = "E:fmt";
		constexpr size_type marker_length = sizeof(marker) - 1;
		constexpr size_type copy_length = marker_length < N - 1 ? marker_length : N - 1;

		for (size_type i = 0; i < copy_length; ++i)
		{
			this->_str[i] = marker[i];
		}
		this->_size = copy_length;
		this->_str[copy_length] = '\0';
	}

	/** @brief 判断 position 是否落在 [0, size()] 内；越界时内容被覆写为错误标记并返回 false */
	constexpr bool _position_valid(const size_type position) noexcept
	{
		if (position > this->_size)
		{
			this->_report_overflow(position + 1);
			return false;
		}
		return true;
	}

	/** @brief 判断全部字符是否满足 predicate ，空串返回 false */
	template <typename PREDICATE>
	constexpr bool _all_of(PREDICATE predicate) const noexcept
	{
		if (this->_size == 0)
		{
			return false;
		}
		for (size_type i = 0; i < this->_size; ++i)
		{
			if (!predicate(this->_str[i]))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * @brief 自后向前拷贝 length 个字节
	 *
	 * 不使用 std::memcpy / std::memmove ：它们不是常量表达式，会令“编译期构造字符串”不可用。
	 * 本函数适用于“目标在源之后且区间重叠”的右移场景
	 *
	 * @param position 目标起始下标
	 * @param source 源起始指针，与本缓冲区重叠时须满足 source <= destination
	 * @param length 拷贝长度
	 */
	constexpr void _copy_chars_backward(const size_type position, const char *source, const size_type length) noexcept
	{
		for (size_type i = length; i > 0; --i)
		{
			this->_str[position + i - 1] = source[i - 1];
		}
	}

	/**
	 * @brief 自前向后拷贝 length 个字节
	 *
	 * 适用于“目标在源之前且区间重叠”的左移场景，以及源不在本缓冲区内的普通拷贝
	 *
	 * @param position 目标起始下标
	 * @param source 源起始指针
	 * @param length 拷贝长度
	 */
	constexpr void _copy_chars_forward(const size_type position, const char *source, const size_type length) noexcept
	{
		for (size_type i = 0; i < length; ++i)
		{
			this->_str[position + i] = source[i];
		}
	}

	/** @brief 把 [text, text + length) 赋值为内容；以字节数（含结尾 '\\0' ）判断容量，不截断 */
	constexpr void _assign_string(const char *text, const size_type length) noexcept
	{
		if (length + 1 > N)
		{
			this->_report_overflow(length + 1);
			return;
		}
		this->_copy_chars_forward(0, text, length);
		this->_size = length;
		this->_str[length] = '\0';
	}

	/** @brief 把内容赋值为 count 个字符 c */
	constexpr void _assign_fill(const size_type count, const char c) noexcept
	{
		if (count + 1 > N)
		{
			this->_report_overflow(count + 1);
			return;
		}
		for (size_type i = 0; i < count; ++i)
		{
			this->_str[i] = c;
		}
		this->_size = count;
		this->_str[count] = '\0';
	}

	/** @brief 在末尾追加 [text, text + length) */
	constexpr String &_append_string(const char *text, const size_type length) noexcept
	{
		const size_type required = this->_size + length + 1;
		if (required > N)
		{
			this->_report_overflow(required);
			return *this;
		}
		this->_copy_chars_forward(this->_size, text, length);
		this->_size += length;
		this->_str[this->_size] = '\0';
		return *this;
	}

	/** @brief 把 [position, size()) 区间连同结尾 '\\0' 一起后移 offset 字节，offset 由调用方保证空间足够 */
	constexpr void _shift_tail(const size_type position, const size_type offset) noexcept
	{
		this->_copy_chars_backward(position + offset, this->_str + position, this->_size - position + 1);
	}

	/** @brief 在 position 处插入 [text, text + length) */
	constexpr String &_insert_string(const size_type position, const char *text, const size_type length) noexcept
	{
		if (!this->_position_valid(position))
		{
			return *this;
		}
		const size_type required = this->_size + length + 1;
		if (required > N)
		{
			this->_report_overflow(required);
			return *this;
		}
		this->_shift_tail(position, length);
		this->_copy_chars_forward(position, text, length);
		this->_size += length;
		this->_str[this->_size] = '\0';
		return *this;
	}

	/**
	 * @brief 格式化写入的公共实现
	 *
	 * 不使用 fmt.h 的 format_to ：它在“含占位符”时返回剩余格式串长度而非实际写入长度，
	 * 无法用来确定结果长度；且它在无占位符溢出时返回 0 ，与“成功写出 0 字符”无法区分。
	 * 这里直接驱动 FormatContext ，以 position() 为准，并单独处理上述两类返回值
	 *
	 * @param format_string 被解析的格式串
	 */
	template <typename... ARGS>
	String &_format_impl(const std::string_view &format_string, const ARGS &...args) noexcept
	{
		this->_size = 0;
		this->_str[0] = '\0';

		// 容量取 N - 1 ，为结尾 '\\0' 留位
		FormatContext context(this->_str, N - 1);
		const int result = detail::FormatImpl<ARGS...>::format(context, format_string, args...);
		const size_type written = context.position();

		/*
		 * fmt.h 的两种溢出上报方式不一致：
		 * 1. 有占位符的路径返回 FormatError::BufferOverflow ；
		 * 2. 无占位符的路径在 write_safe 失败时返回 0 —— 0 同时也是“成功写出 0 个字符”的返回值。
		 * 故这里用“返回 0 但一个字符也没写出、而格式串非空”来识别后者
		 */
		const bool overflowed =
			(result == static_cast<int>(FormatError::BufferOverflow)) ||
			(result == 0 && written == 0 && !format_string.empty());

		if (overflowed)
		{
			/*
			 * 溢出时 position() 会退回本次 write_safe 前的值，故 written + format_string.size()
			 * 是“至少还需要多少个字符”的下界（真实所需长度取决于各实参展开后的宽度，无法预知）。
			 * 例：capacity() 为 7 时格式化 "abcdefgh" 得到 "7<8"
			 */
			this->_report_overflow(written + format_string.size());
			return *this;
		}
		if (result < 0)
		{
			this->_report_format_error();
			return *this;
		}

		this->_size = written < N - 1 ? written : N - 1;
		this->_str[this->_size] = '\0';
		return *this;
	}

	/**
	 * @brief 字符串字面量格式串的格式化写入
	 *
	 * 额外构造 FormatString 只为启用 fmt.h 的编译期格式串检查（当前其检查体被注释掉，
	 * 属预留），解析仍走 _format_impl
	 */
	template <size_t M, typename... ARGS>
	String &_format_literal(const char (&format_string)[M], const ARGS &...args) noexcept
	{
		const FormatString<M> checked(format_string);
		return this->_format_impl(checked.view(), args...);
	}

	/**
	 * @brief 运行期格式串的格式化写入
	 *
	 * 运行期字符串无法直接构造 FormatString ，故按本串容量截入栈上缓冲区后转成视图。
	 * 超长部分会被丢弃
	 *
	 * @param format_string 以 '\\0' 结尾的格式串
	 */
	template <typename... ARGS>
	String &_format_runtime(const char *format_string, const ARGS &...args) noexcept
	{
		char cropped[N]{};
		size_type length = 0;
		while (length < N - 1 && format_string[length] != '\0')
		{
			cropped[length] = format_string[length];
			++length;
		}
		return this->_format_impl(std::string_view(cropped, length), args...);
	}

	/** @brief 用 [text, text + length) 替换 [position, position + count) */
	constexpr String &_replace_string(const size_type position, const size_type count, const char *text, const size_type length) noexcept
	{
		if (!this->_position_valid(position))
		{
			return *this;
		}
		const size_type removable = this->_size - position;
		const size_type removed = count < removable ? count : removable;
		const size_type required = this->_size - removed + length + 1;
		if (required > N)
		{
			this->_report_overflow(required);
			return *this;
		}
		const size_type tail = this->_size - position - removed;
		if (length > removed)
		{
			this->_shift_tail(position + removed, length - removed);
		}
		else if (length < removed)
		{
			this->_copy_chars_forward(position + length, this->_str + position + removed, tail + 1);
		}
		// text 可能指向本对象内部（例如原地 replace），因此搬到移位之后再拷贝
		this->_copy_chars_forward(position, text, length);
		this->_size = required - 1;
		return *this;
	}

public:
	// ------------------------------------------构造与赋值--------------------------------------------------

	/** @brief 构造空串 */
	constexpr String() noexcept : _str{}, _size(0)
	{
	}

	/** @brief 从 C 字符串构造；超出容量时内容被覆写为溢出错误标记 */
	constexpr String(const char *text) noexcept : _str{}, _size(0)
	{
		this->_assign_string(text, detail::string_literal_length(text));
	}

	/** @brief 从字符数组构造；长度取到首个 '\\0' 为止，故从字面量构造属于常量表达式 */
	template <size_t M>
	constexpr String(const char (&text)[M]) noexcept : _str{}, _size(0)
	{
		this->_assign_string(text, detail::string_literal_length(text));
	}

	/** @brief 从字符串视图构造 */
	constexpr String(const std::string_view &text) noexcept : _str{}, _size(0)
	{
		this->_assign_string(text.data(), text.size());
	}

	/** @brief 从另一个定长字符串构造，由于 text 的长度可能大于自身，此行为必须显式转换*/
	template <size_t M, typename = std::enable_if_t<N < M>>
	explicit constexpr String(const String<M> &text) noexcept : _str{}, _size(0)
	{
		this->_assign_string(text.data(), text.size());
	}
	/** @brief 从另一个定长字符串构造，text 长度必定不会超过自身最大长度，故允许显式转换 */
	template <size_t M, typename = std::enable_if_t<N >= M>, typename = void>
	constexpr String(const String<M> &text) noexcept : _str{}, _size(0)
	{
		this->_assign_string(text.data(), text.size());
	}

	/** @brief 从其它“字符串类”类型构造，例如 std::string */
	template <typename TEXT,
			  typename = std::enable_if_t<is_string_like_v<TEXT> &&
										  !std::is_same_v<remove_cvref_t<TEXT>, String>>>
	explicit constexpr String(TEXT &&text) noexcept : _str{}, _size(0)
	{
		const std::string_view view{std::forward<TEXT>(text)};
		this->_assign_string(view.data(), view.size());
	}

	/** @brief 构造 count 个字符 c */
	constexpr String(const size_type count, const char c) noexcept : _str{}, _size(0)
	{
		this->_assign_fill(count, c);
	}

	constexpr String(const String &other) noexcept = default;
	constexpr String(String &&other) noexcept = default;
	constexpr String &operator=(const String &other) noexcept = default;
	constexpr String &operator=(String &&other) noexcept = default;

	constexpr String &operator=(const char *text) noexcept
	{
		this->_assign_string(text, detail::string_literal_length(text));
		return *this;
	}

	// template <size_t M>
	// constexpr String &operator=(const char (&text)[M]) noexcept
	// {
	// 	this->_assign_string(text, detail::string_literal_length(text));
	// 	return *this;
	// }

	constexpr String &operator=(const std::string_view &text) noexcept
	{
		this->_assign_string(text.data(), text.size());
		return *this;
	}

	template <typename TEXT,
			  typename = std::enable_if_t<is_string_like_v<TEXT> &&
										  !std::is_same_v<remove_cvref_t<TEXT>, String>>>
	constexpr String &operator=(TEXT &&text) noexcept
	{
		const std::string_view view{std::forward<TEXT>(text)};
		this->_assign_string(view.data(), view.size());
		return *this;
	}

	// ------------------------------------------容量--------------------------------------------------

	/** @brief 当前字符个数（不含结尾 '\\0' ） */
	constexpr size_type size() const noexcept
	{
		return this->_size;
	}

	/** @brief 同 size() */
	constexpr size_type length() const noexcept
	{
		return this->_size;
	}

	/** @brief 字符个数上限，恒为 N - 1 */
	constexpr size_type capacity() const noexcept
	{
		return N - 1;
	}

	/** @brief 同 capacity() */
	constexpr size_type max_size() const noexcept
	{
		return N - 1;
	}

	/** @brief 缓冲区总字节数，恒为 N */
	constexpr size_type buffer_size() const noexcept
	{
		return N;
	}

	constexpr bool empty() const noexcept
	{
		return this->_size == 0;
	}

	/** @brief 本类型恒不进行动态分配，此接口仅用于与标准库容器接口对齐 */
	constexpr void shrink_to_fit() noexcept
	{
	}

	// ------------------------------------------元素访问--------------------------------------------------

	/**
	 * @brief 只读随机访问
	 *
	 * index 等于 size() 时返回结尾 '\\0' ；超出 size() 时按 std::string 的规定属于未定义行为，
	 * 此处退化为返回结尾 '\\0'
	 */
	constexpr const char &operator[](const size_type index) const noexcept
	{
		return this->_str[index < this->_size ? index : this->_size];
	}

	// /** @brief 带边界检查访问，越界返回结尾 '\\0' */
	// constexpr const char &at(const size_type index) const noexcept
	// {
	// 	return this->_str[index < this->_size ? index : this->_size];
	// }

	/** @brief 首字符，空串返回结尾 '\\0' */
	constexpr const char &front() const noexcept
	{
		return this->_str[0];
	}

	/** @brief 末字符，空串返回结尾 '\\0' */
	constexpr const char &back() const noexcept
	{
		return this->_str[this->_size == 0 ? 0 : this->_size - 1];
	}

	/** @brief 只读裸指针，等价于 c_str() */
	constexpr const char *data() const noexcept
	{
		return this->_str;
	}

	constexpr const char *c_str() const noexcept
	{
		return this->_str;
	}

	/**
	 * @brief 可写裸指针
	 *
	 * 本类无法得知外部写了什么，约定：调用方写完后必须立即调用 commit() ，
	 * 或者自行保证不破坏“以 '\\0' 结尾”与 size() 的一致性
	 */
	constexpr char *data_mutable() noexcept
	{
		return this->_str;
	}

	/** @brief 重新按 '\\0' 计算长度，用于配合 data_mutable() */
	constexpr void commit() noexcept
	{
		size_type length = 0;
		while (length < N - 1 && this->_str[length] != '\0')
		{
			++length;
		}
		this->_size = length;
		this->_str[length] = '\0';
	}

	/** @brief 重新按指定长度计算长度，用于配合 data_mutable() */
	constexpr void commit(const size_type length) noexcept
	{
		if (length < N)
		{
			this->_size = length;
			this->_str[length] = '\0';
		}
		else
		{
			this->_report_overflow(length + 1);
		}
	}

	// ------------------------------------------迭代器--------------------------------------------------

	constexpr iterator begin() const noexcept
	{
		return this->_str;
	}

	constexpr iterator end() const noexcept
	{
		return this->_str + this->_size;
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return this->_str;
	}

	constexpr const_iterator cend() const noexcept
	{
		return this->_str + this->_size;
	}

	constexpr reverse_iterator rbegin() const noexcept
	{
		return reverse_iterator(this->end());
	}

	constexpr reverse_iterator rend() const noexcept
	{
		return reverse_iterator(this->begin());
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator(this->cend());
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(this->cbegin());
	}

	// ------------------------------------------视图与转换--------------------------------------------------

	/** @brief 内容视图，长度为 size() */
	constexpr std::string_view view() const noexcept
	{
		return std::string_view(this->_str, this->_size);
	}

	/**
	 * @brief 整个缓冲区的视图
	 *
	 * 长度恒为 N ，便于把整个缓冲区交给需要定长缓冲区的接口
	 */
	constexpr std::string_view buffer() const noexcept
	{
		return std::string_view(this->_str, N);
	}

	/** @brief 隐式转换为字符串视图，使得 OutStream::show / println 等既有接口可直接使用 */
	constexpr operator std::string_view() const noexcept
	{
		return this->view();
	}

	// ------------------------------------------修改--------------------------------------------------

	/** @brief 清空内容 */
	constexpr void clear() noexcept
	{
		this->_str[0] = '\0';
		this->_size = 0;
	}

	/** @brief 调整字符个数，变长部分用 c 填充；超出容量时内容被覆写为溢出错误标记 */
	constexpr void resize(const size_type count, const char c = '\0') noexcept
	{
		if (count > N - 1)
		{
			this->_report_overflow(count + 1);
			return;
		}
		if (count > this->_size)
		{
			for (size_type i = this->_size; i < count; ++i)
			{
				this->_str[i] = c;
			}
		}
		this->_size = count;
		this->_str[count] = '\0';
	}

	/** @brief 在末尾追加一个字符 */
	constexpr void push_back(const char c) noexcept
	{
		this->append(1, c);
	}

	/** @brief 删除末尾的字符，空串时为无操作 */
	constexpr void pop_back() noexcept
	{
		if (this->_size > 0)
		{
			this->_size--;
			this->_str[this->_size] = '\0';
		}
	}

	/** @brief 追加一个字符 */
	constexpr String &append(const char c) noexcept
	{
		return this->append(1, c);
	}

	/** @brief 追加 count 个字符 c */
	constexpr String &append(const size_type count, const char c) noexcept
	{
		const size_type required = this->_size + count + 1;
		if (required > N)
		{
			this->_report_overflow(required);
			return *this;
		}
		for (size_type i = 0; i < count; ++i)
		{
			this->_str[this->_size + i] = c;
		}
		this->_size += count;
		this->_str[this->_size] = '\0';
		return *this;
	}

	/** @brief 追加 C 字符串 */
	constexpr String &append(const char *text) noexcept
	{
		return this->_append_string(text, detail::string_literal_length(text));
	}

	/** @brief 追加字符数组 */
	template <size_t M>
	constexpr String &append(const char (&text)[M]) noexcept
	{
		return this->_append_string(text, detail::string_literal_length(text));
	}

	/** @brief 追加字符串视图 */
	constexpr String &append(const std::string_view &text) noexcept
	{
		return this->_append_string(text.data(), text.size());
	}

	/** @brief 追加另一个定长字符串 */
	template <size_t M>
	constexpr String &append(const String<M> &text) noexcept
	{
		return this->_append_string(text.data(), text.size());
	}

	/** @brief 追加其它“字符串类”类型，例如 std::string */
	template <typename TEXT,
			  typename = std::enable_if_t<is_string_like_v<TEXT> &&
										  !std::is_same_v<remove_cvref_t<TEXT>, String>>>
	constexpr String &append(TEXT &&text) noexcept
	{
		const std::string_view view{std::forward<TEXT>(text)};
		return this->_append_string(view.data(), view.size());
	}

	/** @brief 拼接并返回自身，语义同 std::string::operator+= */
	constexpr String &operator+=(const std::string_view &text) noexcept
	{
		return this->append(text);
	}

	constexpr String &operator+=(const char c) noexcept
	{
		return this->append(c);
	}

	constexpr String &operator+=(const char *text) noexcept
	{
		return this->append(text);
	}

	template <size_t M>
	constexpr String &operator+=(const String<M> &text) noexcept
	{
		return this->append(text);
	}

	/** @brief 在 position 处插入 count 个字符 c */
	constexpr String &insert(const size_type position, const size_type count, const char c) noexcept
	{
		if (!this->_position_valid(position))
		{
			return *this;
		}
		const size_type required = this->_size + count + 1;
		if (required > N)
		{
			this->_report_overflow(required);
			return *this;
		}
		this->_shift_tail(position, count);
		for (size_type i = 0; i < count; ++i)
		{
			this->_str[position + i] = c;
		}
		this->_size += count;
		this->_str[this->_size] = '\0';
		return *this;
	}

	/** @brief 在 position 处插入 C 字符串 */
	constexpr String &insert(const size_type position, const char *text) noexcept
	{
		return this->_insert_string(position, text, detail::string_literal_length(text));
	}

	/** @brief 在 position 处插入字符数组 */
	template <size_t M>
	constexpr String &insert(const size_type position, const char (&text)[M]) noexcept
	{
		return this->_insert_string(position, text, detail::string_literal_length(text));
	}

	/** @brief 在 position 处插入字符串视图 */
	constexpr String &insert(const size_type position, const std::string_view &text) noexcept
	{
		return this->_insert_string(position, text.data(), text.size());
	}

	/** @brief 在 position 处插入另一个定长字符串 */
	template <size_t M>
	constexpr String &insert(const size_type position, const String<M> &text) noexcept
	{
		return this->_insert_string(position, text.data(), text.size());
	}

	/** @brief 在 position 处插入单个字符，返回插入字符的位置 */
	constexpr iterator insert(const_iterator position, const char c) noexcept
	{
		const size_type index = static_cast<size_type>(position - this->cbegin());
		this->insert(index, 1, c);
		return this->begin() + index;
	}

	/** @brief 删除 [position, position + count) 范围内的字符 */
	constexpr String &erase(const size_type position = 0, const size_type count = npos) noexcept
	{
		if (!this->_position_valid(position))
		{
			return *this;
		}
		const size_type removable = this->_size - position;
		const size_type removed = count < removable ? count : removable;
		this->_copy_chars_forward(position, this->_str + position + removed, this->_size - position - removed + 1);
		this->_size -= removed;
		return *this;
	}

	/** @brief 删除 position 处的单个字符，返回该位置 */
	constexpr iterator erase(const_iterator position) noexcept
	{
		const size_type index = static_cast<size_type>(position - this->cbegin());
		this->erase(index, 1);
		return this->begin() + index;
	}

	/** @brief 用 count_replacement 个字符 c 替换 [position, position + count) 范围 */
	constexpr String &replace(const size_type position, const size_type count,
							  const size_type count_replacement, const char c) noexcept
	{
		if (!this->_position_valid(position))
		{
			return *this;
		}
		const size_type removable = this->_size - position;
		const size_type removed = count < removable ? count : removable;
		const size_type required = this->_size - removed + count_replacement + 1;
		if (required > N)
		{
			this->_report_overflow(required);
			return *this;
		}
		const size_type tail = this->_size - position - removed;
		if (count_replacement > removed)
		{
			this->_shift_tail(position + removed, count_replacement - removed);
		}
		else if (count_replacement < removed)
		{
			this->_copy_chars_forward(position + count_replacement, this->_str + position + removed, tail + 1);
		}
		for (size_type i = 0; i < count_replacement; ++i)
		{
			this->_str[position + i] = c;
		}
		this->_size = required - 1;
		return *this;
	}

	/** @brief 用字符串视图替换 [position, position + count) 范围 */
	constexpr String &replace(const size_type position, const size_type count, const std::string_view &text) noexcept
	{
		return this->_replace_string(position, count, text.data(), text.size());
	}

	/** @brief 用 C 字符串替换 [position, position + count) 范围 */
	constexpr String &replace(const size_type position, const size_type count, const char *text) noexcept
	{
		return this->_replace_string(position, count, text, detail::string_literal_length(text));
	}

	/** @brief 用字符数组替换 [position, position + count) 范围 */
	template <size_t M>
	constexpr String &replace(const size_type position, const size_type count, const char (&text)[M]) noexcept
	{
		return this->_replace_string(position, count, text, detail::string_literal_length(text));
	}

	/** @brief 用另一个定长字符串替换 [position, position + count) 范围 */
	template <size_t M>
	constexpr String &replace(const size_type position, const size_type count, const String<M> &text) noexcept
	{
		return this->_replace_string(position, count, text.data(), text.size());
	}

	/**
	 * @brief 替换全部 old_text 为 new_text ，语义同 Python 的 str.replace
	 *
	 * @return 实际替换的次数
	 */
	constexpr size_type replace(const std::string_view &old_text, const std::string_view &new_text) noexcept
	{
		if (old_text.empty())
		{
			return 0;
		}
		size_type replacements = 0;
		size_type position = this->find(old_text);
		while (position != npos)
		{
			this->_replace_string(position, old_text.size(), new_text.data(), new_text.size());
			++replacements;
			position = this->find(old_text, position + new_text.size());
		}
		return replacements;
	}

	/** @brief 交换两个同型字符串的内容 */
	constexpr void swap(String &other) noexcept
	{
		const String temporary{other};
		other = *this;
		*this = temporary;
	}

	// ------------------------------------------比较--------------------------------------------------

	/** @brief 按字典序比较，返回值含义同 std::string::compare */
	constexpr int compare(const std::string_view &other) const noexcept
	{
		const int result = this->view().compare(other);
		return result < 0 ? -1 : (result > 0 ? 1 : 0);
	}

	constexpr int compare(const char *other) const noexcept
	{
		return this->compare(std::string_view(other, detail::string_literal_length(other)));
	}

	template <size_t M>
	constexpr int compare(const String<M> &other) const noexcept
	{
		return this->compare(other.view());
	}

	/** @brief 是否以 prefix 开头 */
	constexpr bool starts_with(const std::string_view &prefix) const noexcept
	{
		return this->_size >= prefix.size() && this->view().substr(0, prefix.size()) == prefix;
	}

	/** @brief 是否以 suffix 结尾 */
	constexpr bool ends_with(const std::string_view &suffix) const noexcept
	{
		return this->_size >= suffix.size() && this->view().substr(this->_size - suffix.size()) == suffix;
	}

	/** @brief 是否包含 needle */
	constexpr bool contains(const std::string_view &needle) const noexcept
	{
		return this->find(needle) != npos;
	}

	// ------------------------------------------查找--------------------------------------------------

	/** @brief 由前向后查找 needle ，失败返回 npos */
	constexpr size_type find(const std::string_view &needle, const size_type position = 0) const noexcept
	{
		return this->view().find(needle, position);
	}

	constexpr size_type find(const char c, const size_type position = 0) const noexcept
	{
		return this->view().find(c, position);
	}

	constexpr size_type find(const char *needle, const size_type position = 0) const noexcept
	{
		return this->find(std::string_view(needle, detail::string_literal_length(needle)), position);
	}

	/** @brief 由后向前查找 needle ，失败返回 npos */
	constexpr size_type rfind(const std::string_view &needle, const size_type position = npos) const noexcept
	{
		return this->view().rfind(needle, position);
	}

	constexpr size_type rfind(const char c, const size_type position = npos) const noexcept
	{
		return this->view().rfind(c, position);
	}

	/** @brief 查找集合 set 中任意字符首次出现的位置 */
	constexpr size_type find_first_of(const std::string_view &set, const size_type position = 0) const noexcept
	{
		return this->view().find_first_of(set, position);
	}

	/** @brief 查找不属于集合 set 的字符首次出现的位置 */
	constexpr size_type find_first_not_of(const std::string_view &set, const size_type position = 0) const noexcept
	{
		return this->view().find_first_not_of(set, position);
	}

	/** @brief 查找集合 set 中任意字符末次出现的位置 */
	constexpr size_type find_last_of(const std::string_view &set, const size_type position = npos) const noexcept
	{
		return this->view().find_last_of(set, position);
	}

	/** @brief 查找不属于集合 set 的字符末次出现的位置 */
	constexpr size_type find_last_not_of(const std::string_view &set, const size_type position = npos) const noexcept
	{
		return this->view().find_last_not_of(set, position);
	}

	/**
	 * @brief 统计 needle 出现的次数
	 *
	 * @param overlap 为 true 时统计可重叠的出现次数，例如 "aaa" 中的 "aa" 计 2 次；
	 *                为 false 时统计不重叠的贪心匹配次数，与 Python 的 str.count 一致
	 */
	constexpr size_type count(const std::string_view &needle, const size_type position = 0, const bool overlap = false) const noexcept
	{
		if (needle.empty())
		{
			return 0;
		}
		size_type occurrences = 0;
		size_type cursor = position;
		while (cursor < this->_size)
		{
			const size_type found = this->find(needle, cursor);
			if (found == npos)
			{
				break;
			}
			++occurrences;
			cursor = found + (overlap ? 1 : needle.size());
		}
		return occurrences;
	}

	/** @brief 统计字符 c 出现的次数 */
	constexpr size_type count(const char c, const size_type position = 0) const noexcept
	{
		size_type occurrences = 0;
		for (size_type i = position; i < this->_size; ++i)
		{
			if (this->_str[i] == c)
			{
				++occurrences;
			}
		}
		return occurrences;
	}

	/**
	 * @brief 截取 [position, position + count) 子串
	 * 
	 * @param position 起始位置，默认为 0
	 * @param count 截取长度，默认为 npos ，表示截取到末尾
	 *
	 * @note position 超出 size() 属于调用方错误：与 std::string 抛出 out_of_range 不同，
	 * 此处返回内容为溢出错误标记的串，使该误用不会被静默忽略
	 */
	constexpr String substr(const size_type position = 0, const size_type count = npos) const noexcept
	{
		if (position > this->_size)
		{
			String result;
			result._report_overflow(position + 1);
			return result;
		}
		const size_type taken = count < this->_size - position ? count : this->_size - position;
		return String(std::string_view(this->_str + position, taken));
	}

	/** @brief 拷贝 [position, position + count) 到 destination ，返回拷贝的字符个数 */
	constexpr size_type copy(char *destination, const size_type count, const size_type position = 0) const noexcept
	{
		if (position > this->_size || destination == nullptr)
		{
			return 0;
		}
		const size_type taken = count < this->_size - position ? count : this->_size - position;
		for (size_type i = 0; i < taken; ++i)
		{
			destination[i] = this->_str[position + i];
		}
		return taken;
	}

	// ------------------------------------------格式化--------------------------------------------------

	/**
	 * @brief python / C++23 风格格式化写入，语法见 fmt.h 与 STREAM.md
	 *
	 * @param format_string 以 '\\0' 结尾的格式串
	 *
	 * @note 本重载的格式串长度在运行期才确定，故格式串合法性只能由 fmt.h 在运行期检查，
	 * 合法性检查失败时内容被覆写为 "E:fmt" ；若格式串是字符串字面量，请优先使用数组重载
	 * @note 本接口依赖 fmt.h 的模板实例化，程序存储器开销不可忽略
	 */
	template <typename... ARGS, typename TEXT, typename = non_array_char_pointer<TEXT>>
	String &format(TEXT format_string, const ARGS &...args) noexcept
	{
		return this->_format_runtime(format_string, args...);
	}

	/**
	 * @brief 以字符串字面量作为格式串的格式化写入
	 *
	 * 此重载能在编译期得到格式串长度，故可启用 fmt.h 的编译期格式串检查
	 */
	template <size_t M, typename... ARGS>
	String &format(const char (&format_string)[M], const ARGS &...args) noexcept
	{
		return this->_format_literal(format_string, args...);
	}

	/** @brief 清空后格式化写入 */
	template <typename... ARGS, typename TEXT, typename = non_array_char_pointer<TEXT>>
	String &assign_format(TEXT format_string, const ARGS &...args) noexcept
	{
		return this->format(format_string, args...);
	}

	// ------------------------------------------Python 风格接口--------------------------------------------------

	/** @brief 全部转为小写，语义同 Python 的 str.lower */
	constexpr String lower() const noexcept
	{
		String result;
		result._assign_string(this->_str, this->_size);
		for (size_type i = 0; i < result._size; ++i)
		{
			result._str[i] = detail::ascii_to_lower(result._str[i]);
		}
		return result;
	}

	/** @brief 全部转为大写，语义同 Python 的 str.upper */
	constexpr String upper() const noexcept
	{
		String result;
		result._assign_string(this->_str, this->_size);
		for (size_type i = 0; i < result._size; ++i)
		{
			result._str[i] = detail::ascii_to_upper(result._str[i]);
		}
		return result;
	}

	/** @brief 去掉首尾的空白字符，语义同 Python 的 str.strip */
	constexpr String strip() const noexcept
	{
		return this->strip(std::string_view(" \t\n\v\f\r"));
	}

	/** @brief 去掉首尾出现在 characters 中的字符，语义同 Python 的 str.strip(chars) */
	constexpr String strip(const std::string_view &characters) const noexcept
	{
		const size_type first = this->find_first_not_of(characters);
		if (first == npos)
		{
			return String();
		}
		const size_type last = this->find_last_not_of(characters);
		return this->substr(first, last - first + 1);
	}

	/** @brief 去掉首部空白字符，语义同 Python 的 str.lstrip */
	constexpr String lstrip() const noexcept
	{
		return this->lstrip(std::string_view(" \t\n\v\f\r"));
	}

	/** @brief 去掉首部出现在 characters 中的字符，语义同 Python 的 str.lstrip(chars) */
	constexpr String lstrip(const std::string_view &characters) const noexcept
	{
		const size_type first = this->find_first_not_of(characters);
		return first == npos ? String() : this->substr(first);
	}

	/** @brief 去掉尾部空白字符，语义同 Python 的 str.rstrip */
	constexpr String rstrip() const noexcept
	{
		return this->rstrip(std::string_view(" \t\n\v\f\r"));
	}

	/** @brief 去掉尾部出现在 characters 中的字符，语义同 Python 的 str.rstrip(chars) */
	constexpr String rstrip(const std::string_view &characters) const noexcept
	{
		const size_type last = this->find_last_not_of(characters);
		return last == npos ? String() : this->substr(0, last + 1);
	}

	/** @brief 全部字符是否都是十进制数字，语义同 Python 的 str.isdigit */
	constexpr bool isdigit() const noexcept
	{
		return this->_all_of(detail::ascii_digit);
	}

	/** @brief 全部字符是否都是字母，语义同 Python 的 str.isalpha */
	constexpr bool isalpha() const noexcept
	{
		return this->_all_of(detail::ascii_alpha);
	}

	/** @brief 全部字符是否都是字母或数字，语义同 Python 的 str.isalnum */
	constexpr bool isalnum() const noexcept
	{
		return this->_all_of(detail::ascii_alnum);
	}

	/** @brief 全部字符是否都是空白字符，语义同 Python 的 str.isspace */
	constexpr bool isspace() const noexcept
	{
		return this->_all_of(detail::ascii_space);
	}

	/** @brief 是否含至少一个字母，且所有字母均为大写，语义同 Python 的 str.isupper */
	constexpr bool isupper() const noexcept
	{
		bool has_cased = false;
		for (size_type i = 0; i < this->_size; ++i)
		{
			if (detail::ascii_lower(this->_str[i]))
			{
				return false;
			}
			if (detail::ascii_upper(this->_str[i]))
			{
				has_cased = true;
			}
		}
		return has_cased;
	}

	/** @brief 是否含至少一个字母，且所有字母均为小写，语义同 Python 的 str.islower */
	constexpr bool islower() const noexcept
	{
		bool has_cased = false;
		for (size_type i = 0; i < this->_size; ++i)
		{
			if (detail::ascii_upper(this->_str[i]))
			{
				return false;
			}
			if (detail::ascii_lower(this->_str[i]))
			{
				has_cased = true;
			}
		}
		return has_cased;
	}

	/** @brief 是否含至少一个十进制数字 */
	constexpr bool has_digit() const noexcept
	{
		for (size_type i = 0; i < this->_size; ++i)
		{
			if (detail::ascii_digit(this->_str[i]))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief 解析十进制浮点数，语义同 Python 的 float()
	 *
	 * 接受首尾空白、可选的 +/- 、可选的小数点与指数部分，例如 "  -1.5e-3  " 。
	 * 不接受 inf / nan / 十六进制浮点 / 数字分隔符。
	 *
	 * @return 解析成功时 ok 为 true ；内容非法或数值超出 float 范围时 ok 为 false
	 *
	 * @note 本接口不依赖 strtod ，但解析本身的代码量仍不可忽略，请按需使用
	 */
	constexpr FloatingResult to_float() const noexcept
	{
		FloatingResult result;
		size_type cursor = 0;
		while (cursor < this->_size && detail::ascii_space(this->_str[cursor]))
		{
			++cursor;
		}

		bool negative = false;
		if (cursor < this->_size && (this->_str[cursor] == '+' || this->_str[cursor] == '-'))
		{
			negative = (this->_str[cursor] == '-');
			++cursor;
		}

		double value = 0.0;
		size_type integer_digits = 0;
		size_type fraction_digits = 0;
		while (cursor < this->_size && detail::ascii_digit(this->_str[cursor]))
		{
			value = value * 10.0 + (this->_str[cursor] - '0');
			++cursor;
			++integer_digits;
			if (value > 1.0e308)
			{
				return result;
			}
		}
		if (cursor < this->_size && this->_str[cursor] == '.')
		{
			++cursor;
			while (cursor < this->_size && detail::ascii_digit(this->_str[cursor]))
			{
				value = value * 10.0 + (this->_str[cursor] - '0');
				++cursor;
				++fraction_digits;
				if (value > 1.0e308)
				{
					return result;
				}
			}
		}
		if (integer_digits == 0 && fraction_digits == 0)
		{
			return result;
		}

		int exponent = 0;
		if (cursor < this->_size && (this->_str[cursor] == 'e' || this->_str[cursor] == 'E'))
		{
			++cursor;
			bool exponent_negative = false;
			if (cursor < this->_size && (this->_str[cursor] == '+' || this->_str[cursor] == '-'))
			{
				exponent_negative = (this->_str[cursor] == '-');
				++cursor;
			}
			// 有 'e' 却无指数数字属于非法，不做“忽略指数”的宽容处理
			if (cursor >= this->_size || !detail::ascii_digit(this->_str[cursor]))
			{
				return result;
			}
			while (cursor < this->_size && detail::ascii_digit(this->_str[cursor]))
			{
				if (exponent < 100000)
				{
					exponent = exponent * 10 + (this->_str[cursor] - '0');
				}
				++cursor;
			}
			if (exponent_negative)
			{
				exponent = -exponent;
			}
		}

		while (cursor < this->_size && detail::ascii_space(this->_str[cursor]))
		{
			++cursor;
		}
		if (cursor != this->_size)
		{
			return result;
		}

		// 小数点右移 fraction_digits 位后，再按指数整体缩放
		const int scale = exponent - static_cast<int>(fraction_digits);
		if (scale > 308 || scale < -308)
		{
			return result;
		}
		if (scale > 0)
		{
			for (int i = 0; i < scale; ++i)
			{
				value *= 10.0;
			}
		}
		else if (scale < 0)
		{
			for (int i = 0; i < -scale; ++i)
			{
				value *= 0.1;
			}
		}

		if (value > 3.402823567e38 || (value != 0.0 && value < 1.401298464e-45))
		{
			return result;
		}

		result.value = static_cast<float>(negative ? -value : value);
		result.ok = true;
		return result;
	}

	inline auto atoi() const noexcept{
		return atoi(this->view());
	}
};

// ------------------------------------------定长字符串数组--------------------------------------------------

/**
 * @brief String<N> 的定长数组，用于接收 split 等多值结果
 *
 * 数组长度由模板参数给定，压入超过长度的元素记为溢出：此时数组内容被清空（size() 为 0），
 * 溢出次数由 overflow_count() 报告。这与 String 把内容覆写为错误标记的策略一致：
 * 溢出结果一定不会被误当成正常结果使用。
 *
 * @tparam N 每个元素的缓冲区总字节数，语义同 String<N>
 * @tparam M 元素个数上限
 */
template <size_t N, size_t M>
class StringArray
{
	static_assert(M > 0, "StringArray 至少需要 1 个元素");

public:
	using value_type = String<N>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = String<N> &;
	using const_reference = const String<N> &;
	using iterator = typename std::array<String<N>, M>::iterator;
	using const_iterator = typename std::array<String<N>, M>::const_iterator;

private:
	std::array<String<N>, M> _elements{};
	size_type _count = 0;
	/** @brief 溢出次数，用于区分“恰好写满”与“溢出被丢弃” */
	size_type _overflow_count = 0;

public:
	/** @brief 元素个数上限 */
	constexpr static size_type capacity() noexcept
	{
		return M;
	}

	/** @brief 已压入的元素个数 */
	constexpr size_type size() const noexcept
	{
		return this->_count;
	}

	constexpr bool empty() const noexcept
	{
		return this->_count == 0;
	}

	/** @brief 溢出次数，非 0 表示有元素未能压入 */
	constexpr size_type overflow_count() const noexcept
	{
		return this->_overflow_count;
	}

	constexpr void clear() noexcept
	{
		this->_count = 0;
		this->_overflow_count = 0;
	}

	constexpr void push_back(const String<N> &element) noexcept
	{
		if (this->_overflow_count != 0)
		{
			return;
		}
		if (this->_count >= M)
		{
			this->clear();
			this->_overflow_count = 1;
			return;
		}
		this->_elements[this->_count++] = element;
	}

	/** @brief 压入一个字符串视图 */
	constexpr void push_back(const std::string_view &element) noexcept
	{
		this->push_back(String<N>(element));
	}

	constexpr reference operator[](const size_type index) noexcept
	{
		return this->_elements[index];
	}

	constexpr const_reference operator[](const size_type index) const noexcept
	{
		return this->_elements[index];
	}

	constexpr iterator begin() noexcept
	{
		return this->_elements.begin();
	}

	constexpr const_iterator begin() const noexcept
	{
		return this->_elements.begin();
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return this->_elements.cbegin();
	}

	constexpr iterator end() noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	constexpr const_iterator end() const noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	constexpr const_iterator cend() const noexcept
	{
		return this->_elements.cbegin() + static_cast<difference_type>(this->_count);
	}
};

/**
 * @brief 字符串切分结果视图
 *
 * 元素是指向源字符串内部的 std::string_view ，不持有内容；只要源字符串仍存活，结果即可用，
 * 适合零拷贝地遍历各个字段。切分结果的最大元素个数由模板参数给出。
 *
 * 溢出时的处理与 StringArray 一致：清空结果并累计 overflow_count()
 */
template <size_t M>
class StringViewArray
{
public:
	using value_type = std::string_view;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = std::string_view &;
	using const_reference = const std::string_view &;
	using iterator = typename std::array<std::string_view, M>::iterator;
	using const_iterator = typename std::array<std::string_view, M>::const_iterator;

private:
	std::array<std::string_view, M> _elements{};
	size_type _count = 0;
	size_type _overflow_count = 0;

public:
	constexpr static size_type capacity() noexcept
	{
		return M;
	}

	constexpr size_type size() const noexcept
	{
		return this->_count;
	}

	constexpr bool empty() const noexcept
	{
		return this->_count == 0;
	}

	constexpr size_type overflow_count() const noexcept
	{
		return this->_overflow_count;
	}

	constexpr void clear() noexcept
	{
		this->_count = 0;
		this->_overflow_count = 0;
	}

	constexpr void push_back(const std::string_view &element) noexcept
	{
		if (this->_overflow_count != 0)
		{
			return;
		}
		if (this->_count >= M)
		{
			this->clear();
			this->_overflow_count = 1;
			return;
		}
		this->_elements[this->_count++] = element;
	}

	constexpr reference operator[](const size_type index) noexcept
	{
		return this->_elements[index];
	}

	constexpr const_reference operator[](const size_type index) const noexcept
	{
		return this->_elements[index];
	}

	constexpr iterator begin() noexcept
	{
		return this->_elements.begin();
	}

	constexpr const_iterator begin() const noexcept
	{
		return this->_elements.begin();
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return this->_elements.cbegin();
	}

	constexpr iterator end() noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	constexpr const_iterator end() const noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	constexpr const_iterator cend() const noexcept
	{
		return this->_elements.cbegin() + static_cast<difference_type>(this->_count);
	}
};

// ------------------------------------------切分与拼接--------------------------------------------------

/**
 * @brief 按分隔符切分，语义同 Python 的 str.split
 *
 * @param source 待切分的字符串
 * @param container 结果容器，需支持 clear() 、push_back(std::string_view) 与 size() ，
 *                  例如 StringViewArray<N> ；典型用法是令元素个数上限等于源串容量
 * @param separator 分隔符，不可为空
 * @param limit 最大切分次数，负值表示不限制；语义同 Python 的 str.split(sep, maxsplit)
 * @return 写入 container 的元素个数；分隔符为空时返回 0 并清空 container
 *
 * @note 相邻分隔符之间产生空字段，"a,,b" 切分为 3 个元素，与 Python 一致
 */
template <typename CONTAINER>
constexpr size_t split(const std::string_view source, CONTAINER &container,
					   const std::string_view &separator, const int limit = -1) noexcept
{
	container.clear();
	if (separator.empty())
	{
		return 0;
	}

	std::string_view rest = source;
	int splits = 0;
	while (limit < 0 || splits < limit)
	{
		const size_t position = rest.find(separator);
		if (position == std::string_view::npos)
		{
			break;
		}
		container.push_back(rest.substr(0, position));
		rest.remove_prefix(position + separator.size());
		++splits;
	}
	container.push_back(rest);
	return container.size();
}

/**
 * @brief 按空白切分，语义同 Python 的无参 str.split
 *
 * 连续的空白字符视为一个分隔符，且不产生首尾空字段
 *
 * @param source 待切分的字符串
 * @param container 结果容器，同 split
 * @return 写入 container 的元素个数
 */
template <typename CONTAINER>
constexpr size_t split_whitespace(const std::string_view source, CONTAINER &container) noexcept
{
	container.clear();
	size_t cursor = 0;
	while (cursor < source.size())
	{
		while (cursor < source.size() && detail::ascii_space(source[cursor]))
		{
			++cursor;
		}
		const size_t start = cursor;
		while (cursor < source.size() && !detail::ascii_space(source[cursor]))
		{
			++cursor;
		}
		if (cursor > start)
		{
			container.push_back(source.substr(start, cursor - start));
		}
	}
	return container.size();
}

/**
 * @brief 以 separator 拼接容器内的所有元素，语义同 Python 的 str.join
 *
 * @param separator 元素之间的分隔符
 * @param container 元素容器，元素需可构造 std::string_view ，例如 StringArray<N, M> 或 StringViewArray<M>
 * @return 拼接结果；超出目标容量时结果为溢出错误标记
 */
template <size_t N, typename CONTAINER>
constexpr String<N> join(const std::string_view &separator, const CONTAINER &container) noexcept
{
	using element_type = typename CONTAINER::value_type;
	static_assert(std::is_constructible_v<std::string_view, element_type>,
				  "join 的容器元素必须可构造 std::string_view");

	String<N> result;
	for (typename CONTAINER::size_type i = 0; i < container.size(); ++i)
	{
		if (i > 0)
		{
			result.append(separator);
		}
		result.append(std::string_view(container[i]));
	}
	return result;
}

/** @brief 以 C 字符串为分隔符的 join 重载 */
template <size_t N, typename CONTAINER>
constexpr String<N> join(const char *separator, const CONTAINER &container) noexcept
{
	return join<N>(std::string_view(separator, detail::string_literal_length(separator)), container);
}

// ------------------------------------------比较运算符--------------------------------------------------

template <size_t N, size_t M>
constexpr bool operator==(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() == right.view();
}

template <size_t N, size_t M>
constexpr bool operator!=(const String<N> &left, const String<M> &right) noexcept
{
	return !(left == right);
}

template <size_t N, size_t M>
constexpr bool operator<(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() < right.view();
}

template <size_t N, size_t M>
constexpr bool operator>(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() > right.view();
}

template <size_t N, size_t M>
constexpr bool operator<=(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() <= right.view();
}

template <size_t N, size_t M>
constexpr bool operator>=(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() >= right.view();
}

/** @brief 与字符串视图比较；字符串字面量与 C 字符串经隐式转换同样可用 */
template <size_t N>
constexpr bool operator==(const String<N> &left, const std::string_view &right) noexcept
{
	return left.view() == right;
}

template <size_t N>
constexpr bool operator==(const std::string_view &left, const String<N> &right) noexcept
{
	return left == right.view();
}

template <size_t N>
constexpr bool operator!=(const String<N> &left, const std::string_view &right) noexcept
{
	return left.view() != right;
}

template <size_t N>
constexpr bool operator!=(const std::string_view &left, const String<N> &right) noexcept
{
	return left != right.view();
}

template <size_t N>
constexpr bool operator<(const String<N> &left, const std::string_view &right) noexcept
{
	return left.view() < right;
}

template <size_t N>
constexpr bool operator<(const std::string_view &left, const String<N> &right) noexcept
{
	return left < right.view();
}

template <size_t N>
constexpr bool operator>(const String<N> &left, const std::string_view &right) noexcept
{
	return left.view() > right;
}

template <size_t N>
constexpr bool operator>(const std::string_view &left, const String<N> &right) noexcept
{
	return left > right.view();
}

template <size_t N>
constexpr bool operator<=(const String<N> &left, const std::string_view &right) noexcept
{
	return left.view() <= right;
}

template <size_t N>
constexpr bool operator<=(const std::string_view &left, const String<N> &right) noexcept
{
	return left <= right.view();
}

template <size_t N>
constexpr bool operator>=(const String<N> &left, const std::string_view &right) noexcept
{
	return left.view() >= right;
}

template <size_t N>
constexpr bool operator>=(const std::string_view &left, const String<N> &right) noexcept
{
	return left >= right.view();
}

// ------------------------------------------拼接运算符--------------------------------------------------

/** @brief 拼接两个定长字符串，结果缓冲区长度为二者之和 */
template <size_t N, size_t M>
constexpr String<N + M> operator+(const String<N> &left, const String<M> &right) noexcept
{
	String<N + M> result;
	result.append(left);
	result.append(right);
	return result;
}

/** @brief 定长字符串与字符串视图拼接 */
template <size_t N>
constexpr String<N + 1> operator+(const String<N> &left, const std::string_view &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N>
constexpr String<N + 1> operator+(const std::string_view &left, const String<N> &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N, typename TEXT, typename = non_array_char_pointer<TEXT>>
constexpr String<N + 1> operator+(const String<N> &left, TEXT right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N, typename TEXT, typename = non_array_char_pointer<TEXT>>
constexpr String<N + 1> operator+(TEXT left, const String<N> &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

/** @brief 定长字符串与单个字符拼接 */
template <size_t N>
constexpr String<N + 1> operator+(const String<N> &left, const char right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N>
constexpr String<N + 1> operator+(const char left, const String<N> &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

EMBMARTIN_NAMESPACE_END

// ------------------------------------------与库内流设施的对接--------------------------------------------------
// 不提供 operator<< ：库内暂未实现流运算符。
// OutStream 已有 show(std::string_view) 重载，而 String 可隐式转换为 std::string_view ，
// 因此 console.show(str) / console.println("{}", str) 无需任何额外改动即可工作。

#endif // EMBMARTIN_MSTRING_H
