/**
 ******************************************************************************
 * @file    mstring.h
 * @author  孙鸣淼
 * @brief   EMBMartin 定长字符串库
 *
 * 本头文件提供适用于单片机的定长字符串类型 String<N> 及其配套容器，
 * 并提供 Python 风格的常用字符串接口。
 *
 * String<N> 的存储与标准库风格接口由 etl::string<N - 1> 组合实现，
 * 自定义接口在 etl::string 之上实现。
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 * 3. 本模块不进行任何动态内存分配，不抛出异常
 * 4. 任何可能超出容量的操作都按 ETL 的契约截断，并把截断状态记录到
 *    truncated() 中，详见 mstring.md
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
#include <etl/string_view.h>
#include <type_traits>
#include <utility>

#include <etl/string.h>
#include <etl/string_view.h>

#include "macro.h"
#include "meta.h"
#include "fmt.h"

EMBMARTIN_NAMESPACE_BEGIN

/**
 * @brief std::atoi 的 etl::string_view 版本
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
int atoi(const etl::string_view& sv) noexcept;

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
 * @brief 把 etl::string_view 转成 etl::string_view
 *
 * 本模块的公共接口统一使用 etl::string_view ，而 etl::string 的成员接受 etl::string_view ，
 * 因此所有转发都在这里集中完成类型转换
 */
inline etl::string_view to_etl_view(const etl::string_view &view) noexcept
{
	return etl::string_view(view.data(), view.size());
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
 * 存储与标准库风格接口由 etl::string<N - 1> 组合实现，不做任何动态分配。
 * 内部恒保持以 '\\0' 结尾，且 size() 恒为 '\\0' 之前的字符个数。
 *
 * 容量语义与标准库一致：size() 是当前字符个数，capacity() 是字符个数上限。
 * String<N> 中 N 是内部缓冲区的总字节数，故最多容纳 N - 1 个字符，
 * 即 String<8> 可存放 7 个字符。底层 etl::string 的模板参数是“最大字符数”，
 * 因此实际使用 etl::string<N - 1> 。
 *
 * 与 std::string 的关键差异：
 * 1. 溢出按 ETL 契约截断：超出容量的内容被丢弃，并置位 truncated() ；
 * 2. 迭代器是 const char * ：内容只能经由成员函数修改，以保证 '\\0' 结尾不变量；
 * 3. 不抛异常：查找失败由 npos 表达。
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
	using traits_type = etl::char_traits<char>;
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

	static constexpr size_type npos = etl::string_view::npos;

private:
	// ------------------------------------------数据成员--------------------------------------------------

	/** @brief 底层存储，N - 1 是 etl::string 的“最大字符数” */
	etl::string<N - 1> _data;

	/**
	 * @brief 本模块自行写入缓冲区（format 系列）时的截断状态
	 *
	 * etl::string 自己维护的截断标记无法覆盖“外部直接写缓冲区”的情况，故单独记录
	 */
	bool _truncated = false;

	// ------------------------------------------内部工具--------------------------------------------------

	/** @brief 本模块使用的结尾 '\\0' 引用，供越界访问与空串访问返回 */
	static const char &_terminator() noexcept
	{
		static const char terminator = '\0';
		return terminator;
	}

	/** @brief 设置“内容已被覆写、长度由调用方给出”的结果 */
	void _commit_length(const size_type length) noexcept
	{
		this->_data.uninitialized_resize(length < N ? length : N - 1);
	}

	/** @brief 判断全部字符是否满足 predicate ，空串返回 false */
	template <typename PREDICATE>
	bool _all_of(PREDICATE predicate) const noexcept
	{
		if (this->_data.empty())
		{
			return false;
		}
		for (const char c : this->_data)
		{
			if (!predicate(c))
			{
				return false;
			}
		}
		return true;
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
	String &_format_impl(const etl::string_view &format_string, const ARGS &...args) noexcept
	{
		/*
		 * 先把底层缓冲区整体放开，再交给 fmt.h 直接写入。
		 * FormatContext 的容量取 capacity() ，正好是底层可写字符数。
		 */
		this->_data.uninitialized_resize(N);

		FormatContext context(this->_data.data(), this->capacity());
		const int result = detail::FormatImpl<ARGS...>::format(context, format_string, args...);
		size_type written = context.position();

		/*
		 * fmt.h 的两种溢出上报方式不一致：
		 * 1. 有占位符的路径返回 FormatError::BufferOverflow ；
		 * 2. 无占位符的路径在 write_safe 失败时返回 0 —— 0 同时也是“成功写出 0 个字符”的返回值。
		 * 故用“返回 0 但一个字符也没写出、而格式串非空”来识别后者
		 */
		const bool overflowed =
			(result == static_cast<int>(FormatError::BufferOverflow)) ||
			(result == 0 && written == 0 && !format_string.empty());

		/*
		 * 无参数时 fmt.h 整串一次性写出：容量不足会一个字符都不写。
		 * 截断契约要求保留能写下的部分，故这里退化为逐字符写入
		 */
		if (overflowed && etl::string_view::npos == format_string.find('{'))
		{
			const size_type capacity = this->capacity();
			const size_type copied = format_string.size() < capacity ? format_string.size() : capacity;
			for (size_type i = 0; i < copied; ++i)
			{
				this->_data.data()[i] = format_string[i];
			}
			written = copied;
		}

		this->_commit_length(written);
		this->_truncated = overflowed;
		if (result < 0 && !overflowed)
		{
			// 格式串非法，内容被覆写为格式错误标记
			this->_data.assign("E:fmt");
			this->_truncated = false;
		}
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
		return this->_format_impl(etl::string_view(cropped, length), args...);
	}

public:
	// ------------------------------------------构造与赋值--------------------------------------------------

	/** @brief 构造空串 */
	String() noexcept
	{
	}

	/** @brief 从 C 字符串构造；超出容量时按 ETL 契约截断 */
	String(const char *text) noexcept
	{
		this->_data.assign(text);
	}

	/** @brief 从字符数组构造；长度取到首个 '\\0' 为止 */
	template <size_t M>
	String(const char (&text)[M]) noexcept
	{
		this->_data.assign(text);
	}

	/** @brief 从字符串视图构造；超出容量时按 ETL 契约截断 */
	String(const etl::string_view &text) noexcept
	{
		this->_data.assign(detail::to_etl_view(text));
	}

	/** @brief 从另一个定长字符串构造，由于 text 的长度可能大于自身，此行为必须显式转换*/
	template <size_t M, typename = std::enable_if_t<(N < M)>>
	explicit String(const String<M> &text) noexcept
	{
		this->_data.assign(detail::to_etl_view(text.view()));
	}
	/** @brief 从另一个定长字符串构造，text 长度必定不会超过自身最大长度，故允许隐式转换 */
	template <size_t M, typename = std::enable_if_t<(N >= M)>, typename = void>
	String(const String<M> &text) noexcept
	{
		this->_data.assign(detail::to_etl_view(text.view()));
	}

	/** @brief 从其它“字符串类”类型构造，例如 std::string */
	template <typename TEXT,
			  typename = std::enable_if_t<is_string_like_v<TEXT> &&
										  !std::is_same_v<remove_cvref_t<TEXT>, String>>>
	explicit String(TEXT &&text) noexcept
	{
		const etl::string_view view{std::forward<TEXT>(text)};
		this->_data.assign(detail::to_etl_view(view));
	}

	/** @brief 构造 count 个字符 c ；超出容量时按 ETL 契约截断 */
	String(const size_type count, const char c) noexcept
	{
		this->_data.assign(count, c);
	}

	String(const String &other) noexcept = default;
	String(String &&other) noexcept = default;
	String &operator=(const String &other) noexcept = default;
	String &operator=(String &&other) noexcept = default;

	String &operator=(const char *text) noexcept
	{
		this->_data.assign(text);
		return *this;
	}

	String &operator=(const etl::string_view &text) noexcept
	{
		this->_data.assign(detail::to_etl_view(text));
		return *this;
	}

	template <typename TEXT,
			  typename = std::enable_if_t<is_string_like_v<TEXT> &&
										  !std::is_same_v<remove_cvref_t<TEXT>, String>>>
	String &operator=(TEXT &&text) noexcept
	{
		const etl::string_view view{std::forward<TEXT>(text)};
		this->_data.assign(detail::to_etl_view(view));
		return *this;
	}

	// ------------------------------------------容量--------------------------------------------------

	/** @brief 当前字符个数（不含结尾 '\\0' ） */
	size_type size() const noexcept
	{
		return this->_data.size();
	}

	/** @brief 同 size() */
	size_type length() const noexcept
	{
		return this->_data.length();
	}

	/** @brief 字符个数上限，恒为 N - 1 */
	size_type capacity() const noexcept
	{
		return this->_data.capacity();
	}

	/** @brief 同 capacity() */
	size_type max_size() const noexcept
	{
		return this->_data.max_size();
	}

	/** @brief 缓冲区总字节数，恒为 N */
	size_type buffer_size() const noexcept
	{
		return N;
	}

	bool empty() const noexcept
	{
		return this->_data.empty();
	}

	/**
	 * @brief 上一次操作是否发生过截断
	 *
	 * 语义与 ETL 一致：只表示“上一次操作是否丢弃过内容”，成功操作不会清除它，
	 * 需要重用时请调用 clear_truncated()
	 */
	bool truncated() const noexcept
	{
		return this->_data.is_truncated() || this->_truncated;
	}

	/** @brief 清除截断状态 */
	void clear_truncated() noexcept
	{
		this->_data.clear_truncated();
		this->_truncated = false;
	}

	/** @brief 本类型恒不进行动态分配，此接口仅用于与标准库容器接口对齐 */
	void shrink_to_fit() noexcept
	{
	}

	// ------------------------------------------元素访问--------------------------------------------------

	/**
	 * @brief 只读随机访问
	 *
	 * index 等于 size() 时返回结尾 '\\0' ；超出 size() 属于越界访问，
	 * 此处同样返回结尾 '\\0'（ETL 的缓冲区末字节被用作截断标记，不能直接读）
	 */
	const char &operator[](const size_type index) const noexcept
	{
		if (index >= this->_data.size())
		{
			return _terminator();
		}
		return this->_data[index];
	}

	/** @brief 首字符，空串返回结尾 '\\0' */
	const char &front() const noexcept
	{
		return this->_data.empty() ? _terminator() : this->_data.front();
	}

	/** @brief 末字符，空串返回结尾 '\\0' */
	const char &back() const noexcept
	{
		return this->_data.empty() ? _terminator() : this->_data.back();
	}

	/** @brief 只读裸指针，等价于 c_str() */
	const char *data() const noexcept
	{
		return this->_data.data();
	}

	const char *c_str() const noexcept
	{
		return this->_data.c_str();
	}

	/**
	 * @brief 可写裸指针
	 *
	 * 可安全写入的字节数是 buffer_size() - 1 ，随后必须给出结尾 '\\0' 。
	 * 本类无法得知外部写了什么，约定：调用方写完后必须立即调用 commit() ，
	 * 或者自行保证不破坏“以 '\\0' 结尾”与 size() 的一致性
	 */
	char *data_mutable() noexcept
	{
		return this->_data.data();
	}

	/** @brief 重新按 '\\0' 计算长度，用于配合 data_mutable() */
	void commit() noexcept
	{
		size_type length = 0;
		while (length < N - 1 && this->_data.data()[length] != '\0')
		{
			++length;
		}
		this->_commit_length(length);
		this->_truncated = false;
	}

	/** @brief 按指定长度重新计算长度，用于配合 data_mutable() */
	void commit(const size_type length) noexcept
	{
		this->_commit_length(length);
		this->_truncated = false;
	}

	// ------------------------------------------迭代器--------------------------------------------------

	iterator begin() const noexcept
	{
		return this->_data.begin();
	}

	iterator end() const noexcept
	{
		return this->_data.end();
	}

	const_iterator cbegin() const noexcept
	{
		return this->_data.cbegin();
	}

	const_iterator cend() const noexcept
	{
		return this->_data.cend();
	}

	reverse_iterator rbegin() const noexcept
	{
		return reverse_iterator(this->end());
	}

	reverse_iterator rend() const noexcept
	{
		return reverse_iterator(this->begin());
	}

	const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator(this->cend());
	}

	const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(this->cbegin());
	}

	// ------------------------------------------视图与转换--------------------------------------------------

	/** @brief 内容视图，长度为 size() */
	etl::string_view view() const noexcept
	{
		return etl::string_view(this->_data.data(), this->_data.size());
	}

	/**
	 * @brief 整个缓冲区的视图
	 *
	 * 长度恒为 N ，便于把整个缓冲区交给需要定长缓冲区的接口
	 */
	etl::string_view buffer() const noexcept
	{
		return etl::string_view(this->_data.data(), N);
	}

	/** @brief 隐式转换为字符串视图，使得 OutStream::show / println 等既有接口可直接使用 */
	operator etl::string_view() const noexcept
	{
		return this->view();
	}

	// ------------------------------------------修改--------------------------------------------------

	/** @brief 清空内容 */
	void clear() noexcept
	{
		this->_data.clear();
		this->_truncated = false;
	}

	/** @brief 调整字符个数，变长部分用 c 填充；超出容量时按 ETL 契约截断 */
	void resize(const size_type count, const char c = '\0') noexcept
	{
		this->_data.resize(count, c);
	}

	/** @brief 在末尾追加一个字符；容量不足时按 ETL 契约截断 */
	void push_back(const char c) noexcept
	{
		this->_data.push_back(c);
	}

	/** @brief 删除末尾的字符，空串时为无操作 */
	void pop_back() noexcept
	{
		this->_data.pop_back();
	}

	/** @brief 追加一个字符 */
	String &append(const char c) noexcept
	{
		this->_data.append(1, c);
		return *this;
	}

	/** @brief 追加 count 个字符 c */
	String &append(const size_type count, const char c) noexcept
	{
		this->_data.append(count, c);
		return *this;
	}

	/** @brief 追加 C 字符串 */
	String &append(const char *text) noexcept
	{
		this->_data.append(text);
		return *this;
	}

	/** @brief 追加字符数组 */
	template <size_t M>
	String &append(const char (&text)[M]) noexcept
	{
		this->_data.append(text);
		return *this;
	}

	/** @brief 追加字符串视图 */
	String &append(const etl::string_view &text) noexcept
	{
		this->_data.append(detail::to_etl_view(text));
		return *this;
	}

	/** @brief 追加另一个定长字符串 */
	template <size_t M>
	String &append(const String<M> &text) noexcept
	{
		this->append(text.view());
		return *this;
	}

	/** @brief 追加其它“字符串类”类型，例如 std::string */
	template <typename TEXT,
			  typename = std::enable_if_t<is_string_like_v<TEXT> &&
										  !std::is_same_v<remove_cvref_t<TEXT>, String>>>
	String &append(TEXT &&text) noexcept
	{
		const etl::string_view view{std::forward<TEXT>(text)};
		return this->append(view);
	}

	/** @brief 拼接并返回自身，语义同 std::string::operator+= */
	String &operator+=(const etl::string_view &text) noexcept
	{
		return this->append(text);
	}

	String &operator+=(const char c) noexcept
	{
		return this->append(c);
	}

	String &operator+=(const char *text) noexcept
	{
		return this->append(text);
	}

	template <size_t M>
	String &operator+=(const String<M> &text) noexcept
	{
		return this->append(text);
	}

	/** @brief 在 position 处插入 count 个字符 c ；位置越界由 ETL 断言暴露 */
	String &insert(const size_type position, const size_type count, const char c) noexcept
	{
		this->_data.insert(position, count, c);
		return *this;
	}

	/** @brief 在 position 处插入 C 字符串 */
	String &insert(const size_type position, const char *text) noexcept
	{
		this->_data.insert(position, text);
		return *this;
	}

	/** @brief 在 position 处插入字符数组 */
	template <size_t M>
	String &insert(const size_type position, const char (&text)[M]) noexcept
	{
		this->_data.insert(position, detail::to_etl_view(etl::string_view(text)));
		return *this;
	}

	/** @brief 在 position 处插入字符串视图 */
	String &insert(const size_type position, const etl::string_view &text) noexcept
	{
		this->_data.insert(position, detail::to_etl_view(text));
		return *this;
	}

	/** @brief 在 position 处插入另一个定长字符串 */
	template <size_t M>
	String &insert(const size_type position, const String<M> &text) noexcept
	{
		return this->insert(position, text.view());
	}

	/** @brief 在 position 处插入单个字符，返回插入字符的位置 */
	iterator insert(const_iterator position, const char c) noexcept
	{
		return this->_data.insert(position, c);
	}

	/** @brief 删除 [position, position + count) 范围内的字符 */
	String &erase(const size_type position = 0, const size_type count = npos) noexcept
	{
		this->_data.erase(position, count);
		return *this;
	}

	/** @brief 删除 position 处的单个字符，返回该位置 */
	iterator erase(const_iterator position) noexcept
	{
		return this->_data.erase(position);
	}

	/** @brief 用 count_replacement 个字符 c 替换 [position, position + count) 范围 */
	String &replace(const size_type position, const size_type count,
					const size_type count_replacement, const char c) noexcept
	{
		this->_data.replace(position, count, count_replacement, c);
		return *this;
	}

	/** @brief 用字符串视图替换 [position, position + count) 范围 */
	String &replace(const size_type position, const size_type count, const etl::string_view &text) noexcept
	{
		this->_data.replace(position, count, detail::to_etl_view(text));
		return *this;
	}

	/** @brief 用 C 字符串替换 [position, position + count) 范围 */
	String &replace(const size_type position, const size_type count, const char *text) noexcept
	{
		this->_data.replace(position, count, text);
		return *this;
	}

	/** @brief 用字符数组替换 [position, position + count) 范围 */
	template <size_t M>
	String &replace(const size_type position, const size_type count, const char (&text)[M]) noexcept
	{
		this->_data.replace(position, count, text);
		return *this;
	}

	/** @brief 用另一个定长字符串替换 [position, position + count) 范围 */
	template <size_t M>
	String &replace(const size_type position, const size_type count, const String<M> &text) noexcept
	{
		return this->replace(position, count, text.view());
	}

	/**
	 * @brief 替换全部 old_text 为 new_text ，语义同 Python 的 str.replace
	 *
	 * @return 实际替换的次数
	 */
	size_type replace(const etl::string_view &old_text, const etl::string_view &new_text) noexcept
	{
		if (old_text.empty())
		{
			return 0;
		}
		size_type replacements = 0;
		size_type position = this->find(old_text);
		while (position != npos)
		{
			this->_data.replace(position, old_text.size(), detail::to_etl_view(new_text));
			++replacements;
			position = this->find(old_text, position + new_text.size());
		}
		return replacements;
	}

	/** @brief 交换两个同型字符串的内容 */
	void swap(String &other) noexcept
	{
		std::swap(this->_data, other._data);
		const bool truncated = this->_truncated;
		this->_truncated = other._truncated;
		other._truncated = truncated;
	}

	// ------------------------------------------比较--------------------------------------------------

	/** @brief 按字典序比较，返回值含义同 std::string::compare */
	int compare(const etl::string_view &other) const noexcept
	{
		const int result = this->_data.compare(detail::to_etl_view(other));
		return result < 0 ? -1 : (result > 0 ? 1 : 0);
	}

	int compare(const char *other) const noexcept
	{
		return this->compare(etl::string_view(other, detail::string_literal_length(other)));
	}

	template <size_t M>
	int compare(const String<M> &other) const noexcept
	{
		return this->compare(other.view());
	}

	/** @brief 是否以 prefix 开头 */
	bool starts_with(const etl::string_view &prefix) const noexcept
	{
		return this->_data.starts_with(detail::to_etl_view(prefix));
	}

	/** @brief 是否以 suffix 结尾 */
	bool ends_with(const etl::string_view &suffix) const noexcept
	{
		return this->_data.ends_with(detail::to_etl_view(suffix));
	}

	/** @brief 是否包含 needle */
	bool contains(const etl::string_view &needle) const noexcept
	{
		return this->_data.contains(detail::to_etl_view(needle));
	}

	// ------------------------------------------查找--------------------------------------------------

	/** @brief 由前向后查找 needle ，失败返回 npos */
	size_type find(const etl::string_view &needle, const size_type position = 0) const noexcept
	{
		return this->_data.find(detail::to_etl_view(needle), position);
	}

	size_type find(const char c, const size_type position = 0) const noexcept
	{
		return this->_data.find(c, position);
	}

	size_type find(const char *needle, const size_type position = 0) const noexcept
	{
		return this->find(etl::string_view(needle, detail::string_literal_length(needle)), position);
	}

	/** @brief 由后向前查找 needle ，失败返回 npos */
	size_type rfind(const etl::string_view &needle, const size_type position = npos) const noexcept
	{
		return this->_data.rfind(detail::to_etl_view(needle), position);
	}

	size_type rfind(const char c, const size_type position = npos) const noexcept
	{
		return this->_data.rfind(c, position);
	}

	/** @brief 查找集合 set 中任意字符首次出现的位置 */
	size_type find_first_of(const etl::string_view &set, const size_type position = 0) const noexcept
	{
		return this->_data.find_first_of(detail::to_etl_view(set), position);
	}

	/** @brief 查找不属于集合 set 的字符首次出现的位置 */
	size_type find_first_not_of(const etl::string_view &set, const size_type position = 0) const noexcept
	{
		return this->_data.find_first_not_of(detail::to_etl_view(set), position);
	}

	/** @brief 查找集合 set 中任意字符末次出现的位置 */
	size_type find_last_of(const etl::string_view &set, const size_type position = npos) const noexcept
	{
		return this->_data.find_last_of(detail::to_etl_view(set), position);
	}

	/** @brief 查找不属于集合 set 的字符末次出现的位置 */
	size_type find_last_not_of(const etl::string_view &set, const size_type position = npos) const noexcept
	{
		return this->_data.find_last_not_of(detail::to_etl_view(set), position);
	}

	/**
	 * @brief 统计 needle 出现的次数
	 *
	 * @param overlap 为 true 时统计可重叠的出现次数，例如 "aaa" 中的 "aa" 计 2 次；
	 *                为 false 时统计不重叠的贪心匹配次数，与 Python 的 str.count 一致
	 */
	size_type count(const etl::string_view &needle, const size_type position = 0, const bool overlap = false) const noexcept
	{
		if (needle.empty())
		{
			return 0;
		}
		size_type occurrences = 0;
		size_type cursor = position;
		while (cursor < this->_data.size())
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
	size_type count(const char c, const size_type position = 0) const noexcept
	{
		size_type occurrences = 0;
		for (size_type i = position; i < this->_data.size(); ++i)
		{
			if (this->_data[i] == c)
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
	 * 这里交由 ETL 的越界断言处理，未开启断言时返回空串
	 */
	String substr(const size_type position = 0, const size_type count = npos) const noexcept
	{
		const etl::string<N - 1> part = this->_data.substr(position, count);
		return String(etl::string_view(part.data(), part.size()));
	}

	/** @brief 拷贝 [position, position + count) 到 destination ，返回拷贝的字符个数 */
	size_type copy(char *destination, const size_type count, const size_type position = 0) const noexcept
	{
		if (destination == nullptr)
		{
			return 0;
		}
		return this->_data.copy(destination, count, position);
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
		this->clear();
		return this->format(format_string, args...);
	}

	// ------------------------------------------Python 风格接口--------------------------------------------------

	/** @brief 全部转为小写，语义同 Python 的 str.lower */
	String lower() const noexcept
	{
		String result{*this};
		for (size_type i = 0; i < result._data.size(); ++i)
		{
			result._data[i] = detail::ascii_to_lower(result._data[i]);
		}
		return result;
	}

	/** @brief 全部转为大写，语义同 Python 的 str.upper */
	String upper() const noexcept
	{
		String result{*this};
		for (size_type i = 0; i < result._data.size(); ++i)
		{
			result._data[i] = detail::ascii_to_upper(result._data[i]);
		}
		return result;
	}

	/** @brief 去掉首尾的空白字符，语义同 Python 的 str.strip */
	String strip() const noexcept
	{
		return this->strip(etl::string_view(" \t\n\v\f\r"));
	}

	/** @brief 去掉首尾出现在 characters 中的字符，语义同 Python 的 str.strip(chars) */
	String strip(const etl::string_view &characters) const noexcept
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
	String lstrip() const noexcept
	{
		return this->lstrip(etl::string_view(" \t\n\v\f\r"));
	}

	/** @brief 去掉首部出现在 characters 中的字符，语义同 Python 的 str.lstrip(chars) */
	String lstrip(const etl::string_view &characters) const noexcept
	{
		const size_type first = this->find_first_not_of(characters);
		return first == npos ? String() : this->substr(first);
	}

	/** @brief 去掉尾部空白字符，语义同 Python 的 str.rstrip */
	String rstrip() const noexcept
	{
		return this->rstrip(etl::string_view(" \t\n\v\f\r"));
	}

	/** @brief 去掉尾部出现在 characters 中的字符，语义同 Python 的 str.rstrip(chars) */
	String rstrip(const etl::string_view &characters) const noexcept
	{
		const size_type last = this->find_last_not_of(characters);
		return last == npos ? String() : this->substr(0, last + 1);
	}

	/** @brief 全部字符是否都是十进制数字，语义同 Python 的 str.isdigit */
	bool isdigit() const noexcept
	{
		return this->_all_of(detail::ascii_digit);
	}

	/** @brief 全部字符是否都是字母，语义同 Python 的 str.isalpha */
	bool isalpha() const noexcept
	{
		return this->_all_of(detail::ascii_alpha);
	}

	/** @brief 全部字符是否都是字母或数字，语义同 Python 的 str.isalnum */
	bool isalnum() const noexcept
	{
		return this->_all_of(detail::ascii_alnum);
	}

	/** @brief 全部字符是否都是空白字符，语义同 Python 的 str.isspace */
	bool isspace() const noexcept
	{
		return this->_all_of(detail::ascii_space);
	}

	/** @brief 是否含至少一个字母，且所有字母均为大写，语义同 Python 的 str.isupper */
	bool isupper() const noexcept
	{
		bool has_cased = false;
		for (const char c : this->_data)
		{
			if (detail::ascii_lower(c))
			{
				return false;
			}
			if (detail::ascii_upper(c))
			{
				has_cased = true;
			}
		}
		return has_cased;
	}

	/** @brief 是否含至少一个字母，且所有字母均为小写，语义同 Python 的 str.islower */
	bool islower() const noexcept
	{
		bool has_cased = false;
		for (const char c : this->_data)
		{
			if (detail::ascii_upper(c))
			{
				return false;
			}
			if (detail::ascii_lower(c))
			{
				has_cased = true;
			}
		}
		return has_cased;
	}

	/** @brief 是否含至少一个十进制数字 */
	bool has_digit() const noexcept
	{
		for (const char c : this->_data)
		{
			if (detail::ascii_digit(c))
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
	FloatingResult to_float() const noexcept
	{
		FloatingResult result;
		const char *text = this->_data.data();
		const size_type total = this->_data.size();

		size_type cursor = 0;
		while (cursor < total && detail::ascii_space(text[cursor]))
		{
			++cursor;
		}

		bool negative = false;
		if (cursor < total && (text[cursor] == '+' || text[cursor] == '-'))
		{
			negative = (text[cursor] == '-');
			++cursor;
		}

		double value = 0.0;
		size_type integer_digits = 0;
		size_type fraction_digits = 0;
		while (cursor < total && detail::ascii_digit(text[cursor]))
		{
			value = value * 10.0 + (text[cursor] - '0');
			++cursor;
			++integer_digits;
			if (value > 1.0e308)
			{
				return result;
			}
		}
		if (cursor < total && text[cursor] == '.')
		{
			++cursor;
			while (cursor < total && detail::ascii_digit(text[cursor]))
			{
				value = value * 10.0 + (text[cursor] - '0');
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
		if (cursor < total && (text[cursor] == 'e' || text[cursor] == 'E'))
		{
			++cursor;
			bool exponent_negative = false;
			if (cursor < total && (text[cursor] == '+' || text[cursor] == '-'))
			{
				exponent_negative = (text[cursor] == '-');
				++cursor;
			}
			// 有 'e' 却无指数数字属于非法，不做“忽略指数”的宽容处理
			if (cursor >= total || !detail::ascii_digit(text[cursor]))
			{
				return result;
			}
			while (cursor < total && detail::ascii_digit(text[cursor]))
			{
				if (exponent < 100000)
				{
					exponent = exponent * 10 + (text[cursor] - '0');
				}
				++cursor;
			}
			if (exponent_negative)
			{
				exponent = -exponent;
			}
		}

		while (cursor < total && detail::ascii_space(text[cursor]))
		{
			++cursor;
		}
		if (cursor != total)
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

	/** @brief 语义同 C 的 atoi() ，返回 int */
	int atoi() const noexcept
	{
		return EMBMartin::atoi(this->view());
	}
};

// ------------------------------------------定长字符串数组--------------------------------------------------

/**
 * @brief String<N> 的定长数组，用于接收 split 等多值结果
 *
 * 数组长度由模板参数给定，压入超过长度的元素记为溢出：此时数组内容被清空（size() 为 0），
 * 溢出次数由 overflow_count() 报告。
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
	static constexpr size_type capacity() noexcept
	{
		return M;
	}

	/** @brief 已压入的元素个数 */
	size_type size() const noexcept
	{
		return this->_count;
	}

	bool empty() const noexcept
	{
		return this->_count == 0;
	}

	/** @brief 溢出次数，非 0 表示有元素未能压入 */
	size_type overflow_count() const noexcept
	{
		return this->_overflow_count;
	}

	void clear() noexcept
	{
		this->_count = 0;
		this->_overflow_count = 0;
	}

	void push_back(const String<N> &element) noexcept
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
	void push_back(const etl::string_view &element) noexcept
	{
		this->push_back(String<N>(element));
	}

	reference operator[](const size_type index) noexcept
	{
		return this->_elements[index];
	}

	const_reference operator[](const size_type index) const noexcept
	{
		return this->_elements[index];
	}

	iterator begin() noexcept
	{
		return this->_elements.begin();
	}

	const_iterator begin() const noexcept
	{
		return this->_elements.begin();
	}

	const_iterator cbegin() const noexcept
	{
		return this->_elements.cbegin();
	}

	iterator end() noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	const_iterator end() const noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	const_iterator cend() const noexcept
	{
		return this->_elements.cbegin() + static_cast<difference_type>(this->_count);
	}
};

/**
 * @brief 字符串切分结果视图
 *
 * 元素是指向源字符串内部的 etl::string_view ，不持有内容；只要源字符串仍存活，结果即可用，
 * 适合零拷贝地遍历各个字段。切分结果的最大元素个数由模板参数给出。
 *
 * 溢出时的处理与 StringArray 一致：清空结果并累计 overflow_count()
 */
template <size_t M>
class StringViewArray
{
public:
	using value_type = etl::string_view;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = etl::string_view &;
	using const_reference = const etl::string_view &;
	using iterator = typename std::array<etl::string_view, M>::iterator;
	using const_iterator = typename std::array<etl::string_view, M>::const_iterator;

private:
	std::array<etl::string_view, M> _elements{};
	size_type _count = 0;
	size_type _overflow_count = 0;

public:
	static constexpr size_type capacity() noexcept
	{
		return M;
	}

	size_type size() const noexcept
	{
		return this->_count;
	}

	bool empty() const noexcept
	{
		return this->_count == 0;
	}

	size_type overflow_count() const noexcept
	{
		return this->_overflow_count;
	}

	void clear() noexcept
	{
		this->_count = 0;
		this->_overflow_count = 0;
	}

	void push_back(const etl::string_view &element) noexcept
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

	reference operator[](const size_type index) noexcept
	{
		return this->_elements[index];
	}

	const_reference operator[](const size_type index) const noexcept
	{
		return this->_elements[index];
	}

	iterator begin() noexcept
	{
		return this->_elements.begin();
	}

	const_iterator begin() const noexcept
	{
		return this->_elements.begin();
	}

	const_iterator cbegin() const noexcept
	{
		return this->_elements.cbegin();
	}

	iterator end() noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	const_iterator end() const noexcept
	{
		return this->_elements.begin() + static_cast<difference_type>(this->_count);
	}

	const_iterator cend() const noexcept
	{
		return this->_elements.cbegin() + static_cast<difference_type>(this->_count);
	}
};

// ------------------------------------------切分与拼接--------------------------------------------------

/**
 * @brief 按分隔符切分，语义同 Python 的 str.split
 *
 * @param source 待切分的字符串
 * @param container 结果容器，需支持 clear() 、push_back(etl::string_view) 与 size() ，
 *                  例如 StringViewArray<N> ；典型用法是令元素个数上限等于源串容量
 * @param separator 分隔符，不可为空
 * @param limit 最大切分次数，负值表示不限制；语义同 Python 的 str.split(sep, maxsplit)
 * @return 写入 container 的元素个数；分隔符为空时返回 0 并清空 container
 *
 * @note 相邻分隔符之间产生空字段，"a,,b" 切分为 3 个元素，与 Python 一致
 */
template <typename CONTAINER>
size_t split(const etl::string_view source, CONTAINER &container,
			 const etl::string_view &separator, const int limit = -1) noexcept
{
	container.clear();
	if (separator.empty())
	{
		return 0;
	}

	etl::string_view rest = source;
	int splits = 0;
	while (limit < 0 || splits < limit)
	{
		const size_t position = rest.find(separator);
		if (position == etl::string_view::npos)
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
size_t split_whitespace(const etl::string_view source, CONTAINER &container) noexcept
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
 * @param container 元素容器，元素需可构造 etl::string_view ，例如 StringArray<N, M> 或 StringViewArray<M>
 * @return 拼接结果；超出目标容量时结果为截断后的内容
 */
template <size_t N, typename CONTAINER>
String<N> join(const etl::string_view &separator, const CONTAINER &container) noexcept
{
	using element_type = typename CONTAINER::value_type;
	static_assert(std::is_constructible_v<etl::string_view, element_type>,
				  "join 的容器元素必须可构造 etl::string_view");

	String<N> result;
	for (typename CONTAINER::size_type i = 0; i < container.size(); ++i)
	{
		if (i > 0)
		{
			result.append(separator);
		}
		result.append(etl::string_view(container[i]));
	}
	return result;
}

/** @brief 以 C 字符串为分隔符的 join 重载 */
template <size_t N, typename CONTAINER>
String<N> join(const char *separator, const CONTAINER &container) noexcept
{
	return join<N>(etl::string_view(separator, detail::string_literal_length(separator)), container);
}

// ------------------------------------------比较运算符--------------------------------------------------

template <size_t N, size_t M>
bool operator==(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() == right.view();
}

template <size_t N, size_t M>
bool operator!=(const String<N> &left, const String<M> &right) noexcept
{
	return !(left == right);
}

template <size_t N, size_t M>
bool operator<(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() < right.view();
}

template <size_t N, size_t M>
bool operator>(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() > right.view();
}

template <size_t N, size_t M>
bool operator<=(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() <= right.view();
}

template <size_t N, size_t M>
bool operator>=(const String<N> &left, const String<M> &right) noexcept
{
	return left.view() >= right.view();
}

/** @brief 与字符串视图比较；字符串字面量与 C 字符串经隐式转换同样可用 */
template <size_t N>
bool operator==(const String<N> &left, const etl::string_view &right) noexcept
{
	return left.view() == right;
}

template <size_t N>
bool operator==(const etl::string_view &left, const String<N> &right) noexcept
{
	return left == right.view();
}

template <size_t N>
bool operator!=(const String<N> &left, const etl::string_view &right) noexcept
{
	return left.view() != right;
}

template <size_t N>
bool operator!=(const etl::string_view &left, const String<N> &right) noexcept
{
	return left != right.view();
}

template <size_t N>
bool operator<(const String<N> &left, const etl::string_view &right) noexcept
{
	return left.view() < right;
}

template <size_t N>
bool operator<(const etl::string_view &left, const String<N> &right) noexcept
{
	return left < right.view();
}

template <size_t N>
bool operator>(const String<N> &left, const etl::string_view &right) noexcept
{
	return left.view() > right;
}

template <size_t N>
bool operator>(const etl::string_view &left, const String<N> &right) noexcept
{
	return left > right.view();
}

template <size_t N>
bool operator<=(const String<N> &left, const etl::string_view &right) noexcept
{
	return left.view() <= right;
}

template <size_t N>
bool operator<=(const etl::string_view &left, const String<N> &right) noexcept
{
	return left <= right.view();
}

template <size_t N>
bool operator>=(const String<N> &left, const etl::string_view &right) noexcept
{
	return left.view() >= right;
}

template <size_t N>
bool operator>=(const etl::string_view &left, const String<N> &right) noexcept
{
	return left >= right.view();
}

// ------------------------------------------拼接运算符--------------------------------------------------

/** @brief 拼接两个定长字符串，结果缓冲区长度为二者之和 */
template <size_t N, size_t M>
String<N + M> operator+(const String<N> &left, const String<M> &right) noexcept
{
	String<N + M> result;
	result.append(left);
	result.append(right);
	return result;
}

/** @brief 定长字符串与字符串视图拼接 */
template <size_t N>
String<N + 1> operator+(const String<N> &left, const etl::string_view &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N>
String<N + 1> operator+(const etl::string_view &left, const String<N> &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N, typename TEXT, typename = non_array_char_pointer<TEXT>>
String<N + 1> operator+(const String<N> &left, TEXT right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N, typename TEXT, typename = non_array_char_pointer<TEXT>>
String<N + 1> operator+(TEXT left, const String<N> &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

/** @brief 定长字符串与单个字符拼接 */
template <size_t N>
String<N + 1> operator+(const String<N> &left, const char right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

template <size_t N>
String<N + 1> operator+(const char left, const String<N> &right) noexcept
{
	String<N + 1> result;
	result.append(left);
	result.append(right);
	return result;
}

EMBMARTIN_NAMESPACE_END

// ------------------------------------------与库内流设施的对接--------------------------------------------------
// 不提供 operator<< ：库内暂未实现流运算符。
// OutStream 已有 show(etl::string_view) 重载，而 String 可隐式转换为 etl::string_view ，
// 因此 console.show(str) / console.println("{}", str) 无需任何额外改动即可工作。

#endif // EMBMARTIN_MSTRING_H
