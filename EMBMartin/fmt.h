/**
 ******************************************************************************
 * @file    fmt.h
 * @author  孙鸣淼
 * @brief   EMBMartin 内置仿 C++23 风格格式化字符串库
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 * 3. 本模块的程序存储器开销较为严重，需要谨慎使用；若用于嵌入式系统，建议将编译器优化调至 -O3 或 -Oz
 * 4. 本模块对于浮点数的处理过于面向结果，性能甚至远不及 printf ，建议谨慎使用
 ******************************************************************************
 */
#ifndef EMBMARTIN_FORMAT_STRING_H
#define EMBMARTIN_FORMAT_STRING_H

#include <cstring>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <string_view>

#include "macro.h"
#include "meta.h"

EMBMARTIN_NAMESPACE_BEGIN

// ------------------------------------------前向声明--------------------------------------------------

// 主模板声明
template <typename T, typename>
struct formatter;

// ------------------------------------------错误码定义--------------------------------------------------

enum class FormatError
{
	Success = 0,
	BufferOverflow = -1,
	InvalidFormatSpec = -2,
	TypeNotFormattable = -3,
	EmptyFormatString = -4,
	MismatchedBraces = -5,
	TooMuchContext = -6 // 说明符过多
};

// ------------------------------------------上下文管理--------------------------------------------------

// 轻量级格式上下文
class FormatContext
{
	char *buffer_;
	size_t capacity_;
	size_t position_;

public:
	FormatContext(char *buffer, size_t capacity)
		: buffer_(buffer), capacity_(capacity), position_(0)
	{
	}

	// 防止溢出
	bool write_safe(std::string_view str)
	{
		if (position_ + str.size() > capacity_)
		{
			return false;
		}
		for (char c : str)
		{
			buffer_[position_++] = c;
		}
		return true;
	}

	// 防止溢出
	bool write_safe(char ch)
	{
		if (position_ >= capacity_)
		{
			return false;
		}
		if (ch)
		{
			buffer_[position_++] = ch;
		}
		else
		{
			buffer_[position_] = ch;
		}

		return true;
	}

	// 获取已写入数据
	std::string_view view() const
	{
		return std::string_view(buffer_, position_);
	}

	// 获取当前位置
	size_t position() const { return position_; }

	// 设置位置（用于回退等操作）
	void set_position(size_t pos)
	{
		position_ = pos;
	}

	// 剩余空间
	size_t remaining() const { return capacity_ - position_; }
};

// ------------------------------------------解析上下文--------------------------------------------------

// 轻量级解析上下文
class FormatParseContext
{
	std::string_view spec_;
	size_t pos_;

public:
	FormatParseContext(const std::string_view &spec)
		: spec_(spec), pos_(0)
	{
	}

	template <typename... Args>
	auto find(Args... args) const
	{
		return spec_.find(args...);
	}

	auto begin() { return spec_.begin() + pos_; }
	auto end() { return spec_.end(); }
	auto begin() const { return spec_.begin() + pos_; }
	auto end() const { return spec_.end(); }

	// 查看下一个字符
	char peek() const
	{
		return pos_ < spec_.size() ? spec_[pos_] : '\0';
	}

	// 获取当前位置字符并前进
	char get()
	{
		return pos_ < spec_.size() ? spec_[pos_++] : '\0';
	}

	// 消耗一个字符
	void consume()
	{
		if (pos_ < spec_.size())
			pos_++;
	}

	// 获取剩余的格式说明符
	std::string_view remainder() const
	{
		return spec_.substr(pos_);
	}

	// 是否已解析完
	bool empty() const { return pos_ >= spec_.size(); }

	// 获取当前位置
	size_t position() const { return pos_; }
};

// ------------------------------------------格式字符串验证--------------------------------------------------

template <size_t N>
class FormatString
{
	const char (&str_)[N];

public:
	template <size_t M>
	constexpr FormatString(const char (&s)[M]) : str_(s)
	{
		static_assert(M > 0, "Format string cannot be empty");
		// static_assert(validate_basic_structure(s),
		//              "Invalid format string structure");
		// TODO: 改进
	}

	// 获取原始字符串
	constexpr std::string_view view() const
	{
		return std::string_view(str_, N - 1);
	}

	// 转换为C字符串
	constexpr const char *c_str() const { return str_; }

private:
	// 基本结构验证：括号匹配和转义字符
	static constexpr bool validate_basic_structure(const char *s)
	{
		int brace_depth = 0;

		while (*s)
		{
			if (*s == '{')
			{
				// 检查是否是转义 {{
				if (*(s + 1) == '{')
				{
					s++; // 跳过第二个{
				}
				else
				{
					brace_depth++;
					if (brace_depth > 1)
						return false; // 不支持嵌套
				}
			}
			else if (*s == '}')
			{
				// 检查是否是转义 }}
				if (*(s + 1) == '}')
				{
					s++; // 跳过第二个}
				}
				else
				{
					brace_depth--;
					if (brace_depth < 0)
						return false; // 多余的}
				}
			}
			s++;
		}
		return brace_depth == 0;
	}
};

// ------------------------------------------核心接口--------------------------------------------------

template <typename T, typename = void>
struct formatter
{
};

template <typename T, typename = void, typename = void>
struct is_formattable : std::false_type
{
};

template <typename T>
struct is_formattable<
	T,
	std::void_t<decltype(std::declval<formatter<T>>().parse(std::declval<FormatParseContext &>()))>,
	std::void_t<decltype(std::declval<formatter<T>>().format(std::declval<T>(), std::declval<FormatContext &>()))>>
	: std::true_type
{
};

template <typename T>
constexpr bool is_formattable_v = is_formattable<T>::value;

// ------------------------------------------基础类型特化声明--------------------------------------------------

// 通用格式规格迷你语言词法解析器
template <typename T>
struct FormatLex
{
	/***************************************************************************************************
	format_spec:             				[options][width_and_precision][type]
		options:                 			[[fill]align][sign]["z"]["#"]["0"]
			fill:                    		<any character>
			align:                   		"<" | ">" | "=" | "^"
			sign:                    		"+" | "-" | " "
		width_and_precision:     			[width_with_grouping][precision_with_grouping]
			width_with_grouping:     		[width][grouping]
			precision_with_grouping: "." 	[precision][grouping] | "." grouping
				width:                   	digit+
				precision:               	digit+
				grouping:                	"," | "_"
		type:                   			 "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g"
											| "G" | "o" | "s" | "x" | "X" | "%"
	****************************************************************************************************/
	enum class Align : char
	{
		Left = '<',		// 强制字段在可用空间内左对齐（这是大多数对象的默认值）
		Right = '>',	// 强制字段在可用空间内右对齐（这是数字的默认值）
		Centered = '^', // 	强制字段在可用空间内居中
		ZeroFill = '=', // 强制在符号（如果有）之后数字之前放置填充。 这被用于以 '+000000120' 形式打印字段。 这个对齐选项仅适用于数字类型，complex 除外。 当 '0' 紧接在字段宽度之前时这是默认行为

		Invalid = char(0)
	};

	enum class Sign : char
	{
		All = '+',		   // 表示正负号应当同时用于正数与负数
		NegOnly = '-',	   // 表示正负号应当仅用于负数（这是默认的行为）
		SpaceOrSign = ' ', // 表示应当对正数使用前导空格，而对负数使用负号

		Invalid = char(0)
	};
	enum class Grouping : char
	{
		Comma = ',',	  // 对于整数表示类型 'd' 和 'n' 以外的浮点数表示类型每 3 个数位插入一个逗号。 对于其他表示类型，此选项不受支持。
		Underscore = '_', // 对于整数表示类型 'd' 和 'n' 以外的浮点数表示类型，每 3 个数位插入一个下划线。 对于整数表示类型 'b', 'o', 'x' 和 'X'，则每 4 个数位插入一个下划线。 对于其他表示类型，此选项不受支持。
		None = '\0',	  // 不使用分隔符

		Invalid = char(0)
	};
	enum class Type : char
	{
		// 可用的字符串表示类型
		String = 's', // 字符串格式。这是字符串的默认类型，可以省略。

		// 可用的整数表示类型
		Binary = 'b',	// 二进制格式。 输出以 2 为基数的数字。
		Char = 'c',		// 字符。在打印之前将整数转换为相应的unicode字符。
		Dec = 'd',		// 十进制整数。 输出以 10 为基数的数字。
		Octal = 'o',	// 8进制格式。 输出以 8 为基数的数字。
		HexLower = 'x', // 十六进制格式。 输出以 16 为基数的数字，使用小写字母表示 9 以上的数码。
		HexUpper = 'X', // 十六进制格式。 输出以 16 为基数的数字，使用大写字母表示 9 以上的数码。 在指定 '#' 的情况下，前缀 '0x' 也将被转为大写形式 '0X'。

		// 浮点表示类型；整数可用，若使用则先转化为单精度浮点数
		eScientific = 'e', // 指数记数法。 以小写字母 'e' 引入指数部分。
		EScientific = 'E', // 指数记数法。 以大写字母 'E' 引入指数部分。
		fFixed = 'f',	   // 十进制浮点格式。对于给定的精度 prec( 默认 6 )，将数字格式化为小数点之后恰好有 prec 位的小数形式
		FFixed = 'F',	   // 定点表示。 与 'f' 相似，但会将 nan 转为 NAN 并将 inf 转为 INF。
		gGeneral = 'g',	   // 常规格式。 相当于 %g （printf）
		GGeneral = 'G',	   // 常规格式。 相当于 %G （printf）
		Percentage = '%',  // 百分比。 将数字乘以 100 并显示为定点 ('f') 格式，后面带一个百分号。

		Invalid = char(0)
	};
	constexpr static Align align(char c) noexcept;
	constexpr static Sign sign(char c = '-') noexcept;
	constexpr static Grouping grouping(char c = '\0') noexcept;
	constexpr static Type type(char c) noexcept;

	constexpr static int default_prec = 6;

	char _fill = ' ';			   // 填充字符，默认空格
	Align _align = Align::Invalid; // 对齐方式; 注意：由于字符类型作为字符输出还是作为整数输出需要运行时确定，这里不给默认参数
	Sign _sign = Sign::NegOnly;	   // 符号选项，默认仅负数显示符号

	bool _z = false;		 // -0.0 强制转 +0.0（四舍五入后）
	bool _prefix = false;	 // 数值类型前缀（0b/0o/0x）, 十进制任何时候不使用前缀
	bool _zero_fill = false; // 启用感知正负号的零填充; (对标 python 3.10 版本)在 width 字段之前添加 '0' 不影响字符串的默认对齐

	int _width = 0;								  // 最小字段宽度, 包括任何任何前缀、分隔符和其他格式化字符; 默认不指定, 宽度将由内容确定
	Grouping _int_part_grouping = Grouping::None; // 整数部分分隔符
	/***************************************************************************************************
	precision 是一个十进制整数，它表示对于以表示类型 'f' 和 'F' 格式化的数值应当在小数点后显示多少个数位，
	或者对于以表示类型 'g' 或 'G' 格式化的数值应当在小数点前后显示多少个数位。
	对于字符串表示类型，该字段表示最大的字段大小 ——换句话说，就是要使用多少个来自字段内容的字符。
	不允许对整数表示类型指定 precision 字段。
	****************************************************************************************************/
	int _prec = default_prec;
	Grouping _frac_part_grouping = Grouping::None; // 小数部分分隔符

	Type _type = is_string_like_v<T> ? Type::String
									 : (is_character_v<T> ? Type::Char
														  : (std::is_integral_v<T> ? Type::Dec
																				   : (std::is_floating_point_v<T> ? Type::gGeneral
																												  : Type::String)));

	/**
	 * @brief 解析格式说明符（运行时）
	 *
	 * @param ctx 解析上下文
	 * @return int 返回0表示成功，负数表示错误码
	 */
	int parse(FormatParseContext &ctx);
};

template <typename T>
constexpr typename FormatLex<T>::Align FormatLex<T>::align(char c) noexcept
{
	switch (c)
	{
	case '<':
		return Align::Left;
	case '>':
		return Align::Right;
	case '^':
		return Align::Centered;
	case '=':
		return Align::ZeroFill;
	default:
		return Align::Invalid;
	}
}
template <typename T>
constexpr typename FormatLex<T>::Sign FormatLex<T>::sign(char c) noexcept
{
	switch (c)
	{
	case '+':
		return Sign::All;
	case '-':
		return Sign::NegOnly;
	case ' ':
		return Sign::SpaceOrSign;
	default:
		return Sign::Invalid;
	}
}

template <typename T>
constexpr typename FormatLex<T>::Grouping FormatLex<T>::grouping(char c) noexcept
{
	switch (c)
	{
	case ',':
		return Grouping::Comma;
	case '_':
		return Grouping::Underscore;
	case '\0':
		return Grouping::None;
	default:
		return Grouping::Invalid;
	}
}

template <typename T>
constexpr typename FormatLex<T>::Type FormatLex<T>::type(char c) noexcept
{
	switch (c)
	{
		// 可用的字符串表示类型
	case 's':
		return Type::String;

		// 可用的整数表示类型
	case 'b':
		return Type::Binary;
	case 'c':
		return Type::Char;
	case 'd':
		return Type::Dec;
	case 'o':
		return Type::Octal;
	case 'x':
		return Type::HexLower;
	case 'X':
		return Type::HexUpper;

		// 浮点表示类型；整数可用，若使用则先转化为单精度浮点数
	case 'e':
		return Type::eScientific;
	case 'E':
		return Type::EScientific;
	case 'f':
		return Type::fFixed;
	case 'F':
		return Type::FFixed;
	case 'g':
		return Type::gGeneral;
	case 'G':
		return Type::GGeneral;
	case '%':
		return Type::Percentage;

	default:
		return Type::Invalid;
	}
}

template <typename T>
int FormatLex<T>::parse(FormatParseContext &ctx)
{
	// 暂时不支持参数顺序指定，故必须以 : 起头
	if (ctx.peek() == ':')
		ctx.consume();
	else
		return ctx.empty() ? 0 : (int)FormatError::InvalidFormatSpec;

	// [[fill]align]
	char c0 = ctx.peek(), c1 = '\0';
	if (c0 && 1 < ctx.remainder().size())
		c1 = ctx.remainder().data()[1];
	auto temp_align = align(c1);
	if (temp_align != Align::Invalid)
	{
		_fill = c0;
		_align = temp_align;
		ctx.consume();
		ctx.consume();
	}
	else
	{
		temp_align = align(c0);
		if (temp_align != Align::Invalid)
		{
			_align = temp_align;
			ctx.consume();
		}
	}
	// [sign]
	auto temp_sign = sign(ctx.peek());
	if (temp_sign != Sign::Invalid)
	{
		_sign = temp_sign;
		ctx.consume();
	}
	// [z]
	if (ctx.peek() == 'z')
	{
		_z = true;
		ctx.consume();
	}
	// [#]
	if (ctx.peek() == '#')
	{
		_prefix = true;
		ctx.consume();
	}
	// [0]
	if (ctx.peek() == '0')
	{
		_zero_fill = true;
		_fill = '0';
		ctx.consume();
	}

	// [width]
	if (ctx.peek() >= '0' && ctx.peek() <= '9') // 手动指定宽度，不使用自适应宽度
		_width = 0;
	while (ctx.peek() >= '0' && ctx.peek() <= '9')
	{
		_width = _width * 10 + (ctx.get() - '0');
	}
	// [grouping]

	if (bool(_int_part_grouping = grouping(ctx.peek()))) // 无 grouping 参数返回 0
		ctx.consume();

	// [.]
	if (!(ctx.peek() == '.'))
		goto _TYPE;
	ctx.consume();

	// [precision]
	if (ctx.peek() >= '0' && ctx.peek() <= '9')
	{
		_prec = 0;
		while (ctx.peek() >= '0' && ctx.peek() <= '9')
		{
			_prec = _prec * 10 + (ctx.get() - '0');
		}
	}

	// [grouping]
	if (bool(_frac_part_grouping = grouping(ctx.peek()))) // 无 grouping 参数返回 0
		ctx.consume();

_TYPE:

	// [type]
	auto temp_type = type(ctx.peek());
	if (temp_type != Type::Invalid)
	{
		_type = temp_type;
		ctx.consume();
	}

	// 根据 type 决定默认对齐方式
	if (_align == Align::Invalid)
	{
		if (_type == Type::String)
			_align = Align::Left;
		else
			_align = Align::Right;
	}

	return ctx.empty() ? 0 : static_cast<int>(FormatError::TooMuchContext);
}

// 算术类型通用格式化器
template <typename T>
struct formatter<T, std::enable_if_t<std::is_arithmetic_v<T>>> : public FormatLex<T>
{
	using BaseType = FormatLex<T>;
	int format(T value, FormatContext &ctx) const;
};

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

int decimal_exponent(long double value) noexcept;
void round_digits(char *digits, int &length, int keep, char guard) noexcept;
void increment_decimal(char *digits) noexcept;

void make_float_parts(long double value, char type,
					  int precision, char *integer, char *fraction, char *exponent) noexcept;

EMBMARTIN_DETAIL_NAMESPACE_END

template <typename T>
int formatter<T, std::enable_if_t<std::is_arithmetic_v<T>>>::format(T value, FormatContext &ctx) const
{
	// ------------------------------------------前置类型转换判断--------------------------------------------------
	// 如果 using_float_type == 1 && T 为整数，则转化为浮点数后再格式化
	bool using_float_type = this->_type == BaseType::Type::eScientific ||
							this->_type == BaseType::Type::EScientific ||
							this->_type == BaseType::Type::fFixed ||
							this->_type == BaseType::Type::FFixed ||
							this->_type == BaseType::Type::gGeneral ||
							this->_type == BaseType::Type::GGeneral ||
							this->_type == BaseType::Type::Percentage;
	bool using_char_type = this->_type == BaseType::Type::Char;
	if (this->_type == BaseType::Type::String)
	{
		return (int)FormatError::InvalidFormatSpec;
	}

	// ------------------------------------------前缀生成--------------------------------------------------
	char prefix_buff[3];
	prefix_buff[0] = this->_prefix ? '0' : '\0';
	if (this->_prefix)
	{
		switch (this->_type)
		{
		case BaseType::Type::Binary:
			prefix_buff[1] = 'b';
			break;
		case BaseType::Type::Octal:
			prefix_buff[1] = 'o';
			break;
		case BaseType::Type::HexLower:
			prefix_buff[1] = 'x';
			break;
		case BaseType::Type::HexUpper:
			prefix_buff[1] = 'X';
			break;
		case BaseType::Type::Dec:
		default:
			prefix_buff[0] = '\0';
			break;
		};
		prefix_buff[2] = '\0';
	}

	// ------------------------------------------符号生成--------------------------------------------------
	char sign_char[2] = "";
	if (using_float_type)
		goto PREFIX_GEN_END;

	switch (this->_sign)
	{
	case BaseType::Sign::All:
		sign_char[0] = (value < 0) ? '-' : '+';
		break;
	case BaseType::Sign::NegOnly:
		sign_char[0] = (value < 0) ? '-' : '\0';
		break;
	case BaseType::Sign::SpaceOrSign:
		sign_char[0] = (value < 0) ? '-' : ' ';
		break;
	default:
		return (int)FormatError::InvalidFormatSpec;
	};
PREFIX_GEN_END:
	// ------------------------------------------数字+分组生成--------------------------------------------------

	// 基数判断 + 非整数非法指定基数
	uint8_t base = 10;
	switch (this->_type)
	{
	case BaseType::Type::Binary:
		if constexpr (!std::is_integral_v<T>)
		{
			return (int)FormatError::InvalidFormatSpec;
		}
		base = 2;
		break;
	case BaseType::Type::Octal:
		if constexpr (!std::is_integral_v<T>)
		{
			return (int)FormatError::InvalidFormatSpec;
		}
		base = 8;
		break;
	case BaseType::Type::HexLower:
	case BaseType::Type::HexUpper:
		if constexpr (!std::is_integral_v<T>)
		{
			return (int)FormatError::InvalidFormatSpec;
		}
		base = 16;
		break;
	case BaseType::Type::Dec:
		if constexpr (!std::is_integral_v<T>)
		{
			return (int)FormatError::InvalidFormatSpec;
		}
	default:
		break;
	};

	// 分组符 + 每组项数 + 非法处理
	const char int_group_char[2]{char(this->_int_part_grouping)};
	const char frac_group_char[2]{char(this->_frac_part_grouping)};
	if (frac_group_char[0] && !using_float_type)
		return (int)FormatError::InvalidFormatSpec;

	uint8_t int_grouping_num = 0, frac_grouping_num = 0;

	if (int_group_char[0] == ',')
	{
		/***************************************************************************************************
		对于整数表示类型 'd' 和浮点数表示类型每 3 个数位插入一个逗号。
		对于其他表示类型，此选项不受支持。
		****************************************************************************************************/
		if (!using_float_type)
		{
			if (!(this->_type == BaseType::Type::Dec))
				return (int)FormatError::InvalidFormatSpec;
		}

		int_grouping_num = 3;
	}
	else
	{
		/***************************************************************************************************
		对于整数表示类型 'd' 和浮点数表示类型，每 3 个数位插入一个下划线。
		对于整数表示类型 'b', 'o', 'x' 和 'X'，则每 4 个数位插入一个下划线。
		****************************************************************************************************/
		int_grouping_num = (base == 10) ? 3 : 4;
	}
	// 同上
	if (frac_group_char[0] == ',')
	{
		if (!using_float_type)
		{
			if (!(this->_type == BaseType::Type::Dec))
				return (int)FormatError::InvalidFormatSpec;
		}

		frac_grouping_num = 3;
	}
	else
	{
		frac_grouping_num = (base == 10) ? 3 : 4;
	}

	// 整数部分（逆序,含分组符）
	char rev_int_digits[128];
	// 小数+指数部分（顺序,含分组符，含小数点, 最多64个字符）
	char frac_digits[64];

	if (using_char_type)
	{
		rev_int_digits[0] = char(value);
		rev_int_digits[1] = '\0';
		frac_digits[0] = '\0';
	}
	else if (!using_float_type)
	{
		if constexpr (std::is_integral_v<T>)
		{
			T abs_value;
			if constexpr (std::is_signed_v<T>)
			{
				abs_value = abs(value);
			}
			else
			{
				abs_value = value;
			}

			for (int i = 0;; i++)
			{
				auto temp = char(abs_value % base);
				if (temp <= 9)
					rev_int_digits[i] = temp + '0';
				else // hex
					rev_int_digits[i] = temp - 10 + (this->_type == EMBMartin::formatter<T>::Type::HexLower ? 'a' : 'A');

				abs_value /= base;

				if (abs_value == 0) // 终止条件；注意：必须放在一次循环之后，否则 0 输出空
				{
					rev_int_digits[++i] = '\0';
					break;
				}
				// 分隔符条件，找规律易证
				if ((i % (int_grouping_num + 1)) == (int_grouping_num - 1))
				{
					if ((*int_group_char) != '\0')
						rev_int_digits[++i] = *int_group_char;
				}
			}
			frac_digits[0] = '\0';
		}
		else
		{
			return (int)FormatError::InvalidFormatSpec;
		}
	}
	else
	{
		long double numeric_value = static_cast<long double>(value);
		bool negative = std::signbit(numeric_value);
		if (negative)
			numeric_value = -numeric_value;
		if (this->_type == BaseType::Type::Percentage)
			numeric_value *= 100.0L;
		if (this->_z && negative && numeric_value < 0.5L * std::pow(10.0L, -this->_prec))
			negative = false;
		if (negative)
			sign_char[0] = '-';
		else if (this->_sign == BaseType::Sign::All)
			sign_char[0] = '+';
		else if (this->_sign == BaseType::Sign::SpaceOrSign)
			sign_char[0] = ' ';

		if (!std::isfinite(numeric_value))
		{
			const char *special = std::isnan(numeric_value) ? "nan" : "inf";
			std::strcpy(rev_int_digits, special);
			frac_digits[0] = '\0';
		}
		else
		{
			char integer_digits[128]{};
			char fraction_digits[64]{};
			char exponent_digits[8]{};
			int precision = this->_prec < 0 ? 0 : this->_prec;
			detail::make_float_parts(numeric_value, char(this->_type), precision,
									 integer_digits, fraction_digits, exponent_digits);

			int ri = 0;
			int integer_length = static_cast<int>(std::strlen(integer_digits));
			int group_count = 0;
			for (int i = integer_length - 1; i >= 0 && ri < 127; --i)
			{
				rev_int_digits[ri++] = integer_digits[i];
				if (int_group_char[0] && ++group_count == int_grouping_num && i > 0)
				{
					rev_int_digits[ri++] = *int_group_char;
					group_count = 0;
				}
			}
			rev_int_digits[ri] = '\0';

			int rf = 0;
			if (fraction_digits[0])
			{
				frac_digits[rf++] = '.';
				int fraction_length = static_cast<int>(std::strlen(fraction_digits));
				for (int i = 0; i < fraction_length && rf < 62; ++i)
				{
					frac_digits[rf++] = fraction_digits[i];
					if (frac_group_char[0] && (i + 1) % frac_grouping_num == 0 && i + 1 < fraction_length)
						frac_digits[rf++] = *frac_group_char;
				}
			}
			if (exponent_digits[0] && rf < 63)
			{
				frac_digits[rf++] = (this->_type == BaseType::Type::EScientific ||
									 this->_type == BaseType::Type::GGeneral)
										? 'E'
										: 'e';
				for (int i = 0; exponent_digits[i] && rf < 63; ++i)
					frac_digits[rf++] = exponent_digits[i];
			}
			if (this->_type == BaseType::Type::Percentage && rf < 63)
				frac_digits[rf++] = '%';
			frac_digits[rf] = '\0';
		}
	}

	// ------------------------------------------总拼接--------------------------------------------------

	// 判断是否需要填充
	int16_t used_width = strlen(prefix_buff) + strlen(sign_char) +
						 strlen(rev_int_digits) + strlen(frac_digits);
	// int16_t fill_num = std::clamp(this->_width - used_width, 0 , std::numeric_limits<uint16_t>::max());
	int16_t fill_num = this->_width - used_width; // 负数不影响

	if (this->_zero_fill || char(this->_align) == '=') // 在符号和数之间填充
	{
		ctx.write_safe(sign_char);
		ctx.write_safe(prefix_buff);
		for (int16_t i = 0; i < fill_num; i++)
		{
			ctx.write_safe(this->_fill);
		}
		for (int8_t i = strlen(rev_int_digits) - 1; i >= 0; i--)
		{
			ctx.write_safe(rev_int_digits[i]);
		}
		ctx.write_safe(frac_digits);
	}
	else // 根据对齐类型填充
	{
		int8_t pre_fill_num;
		int8_t suf_fill_num;
		if (char(this->_align) == '^')
		{
			pre_fill_num = fill_num / 2;				  // floor div 2
			suf_fill_num = fill_num / 2 + (fill_num % 2); // ceil div 2
		}
		else if (char(this->_align) == '>')
		{
			pre_fill_num = fill_num;
			suf_fill_num = 0;
		}
		else // '<'
		{
			pre_fill_num = 0;
			suf_fill_num = fill_num;
		}

		for (int8_t i = 0; i < pre_fill_num; i++)
		{
			ctx.write_safe(this->_fill);
		}
		ctx.write_safe(sign_char);
		ctx.write_safe(prefix_buff);
		for (int8_t i = strlen(rev_int_digits) - 1; i >= 0; i--)
		{
			ctx.write_safe(rev_int_digits[i]);
		}
		ctx.write_safe(frac_digits);
		for (int8_t i = 0; i < suf_fill_num; i++)
		{
			ctx.write_safe(this->_fill);
		}
	}

	// ctx.write_safe('\0');
	return (int)FormatError::Success;
}

// 字符串类型格式化器
// 注意：这里判断的是“能否由 T 构造出 std::string_view”（即 T 是字符串类对象/字符指针/字符数组），
// 而不是“能否由 std::string_view 构造出 T”。后者对 const char* 恒为 false，
// 会导致 const char* 匹配不到本特化而回退到空的主模板。
template <typename T>
struct formatter<T, std::enable_if_t<is_string_like_v<T> || is_character_array_v<T>>>
	: public FormatLex<T>
{
	using BaseType = FormatLex<T>;
	int format(const T &value, FormatContext &ctx) const;
};

template <typename T>
int formatter<T, std::enable_if_t<is_string_like_v<T> || is_character_array_v<T>>>::format(const T &value, FormatContext &ctx) const
{
	// 参数检查
	if (this->_int_part_grouping != BaseType::grouping() ||
		this->_frac_part_grouping != BaseType::grouping() ||
		this->_prec != BaseType::default_prec ||
		this->_prefix != false ||
		this->_z != false ||
		this->_sign != BaseType::sign() ||
		this->_zero_fill != false ||
		this->_type != BaseType::type('s') ||
		this->_align == BaseType::align('='))
	{
		return (int)FormatError::InvalidFormatSpec;
	}

	// 填充计算
	auto s = std::string_view(value);
	int l = s.length();
	int fill_num = this->_width - l;

	int8_t pre_fill_num;
	int8_t suf_fill_num;
	if (char(this->_align) == '^')
	{
		pre_fill_num = fill_num / 2;				  // floor div 2
		suf_fill_num = fill_num / 2 + (fill_num % 2); // ceil div 2
	}
	else if (char(this->_align) == '>')
	{
		pre_fill_num = fill_num;
		suf_fill_num = 0;
	}
	else // '<'
	{
		pre_fill_num = 0;
		suf_fill_num = fill_num;
	}

	// 写入上下文
	for (int8_t i = 0; i < pre_fill_num; i++)
	{
		ctx.write_safe(this->_fill);
	}
	ctx.write_safe(s);
	for (int8_t i = 0; i < suf_fill_num; i++)
	{
		ctx.write_safe(this->_fill);
	}

	// ctx.write_safe('\0');
	return (int)FormatError::Success;
}

// pair 概念通用格式化器
template <typename _Pair>
struct formatter<_Pair, std::enable_if_t<is_generalized_pair_v<_Pair>>>
{
	/***************************************************************************************************
	format_spec:  			[:[left bracket][x_format_spec][sep[y_format_spec]][right bracket]]
		left bracket:  		(  [  <  or none
		sep:           		",,"  or ';' | if not given, both x & y use x_format_spec; else, format x & y separately
		right bracket: 		)  ]  >  or none (must match left bracket)
		x_format:     		any valid format spec for type _Ty; if [sep[y_format_spec]] not given, this formats y too
		y_format:     		any valid format spec for type _Ty

	examples:
		console.println("coord: {::#X;:#b}", Coordinate<int>(0xff,0b101010));		// coord: 0XFF; 0b101010
		console.println("coord: {:(:#X)}", Coordinate<int>(0xff,0b101010));			// coord: (0XFF, 0X2A)
		console.println("coord: {:(:#X,,:#b)}", Coordinate<int>(0xff,0b101010));	// coord: [0XFF, 0b101010]
		console.println("aggr:  {:[]}\n", add_result<int>{20,false});				// aggr:  [20, 0]
	****************************************************************************************************/
	using value_type = _Pair;
	using _Tx = generalized_pair_element_t<0, _Pair>;
	using _Ty = generalized_pair_element_t<1, _Pair>;

	enum class Prefix : char
	{
		SquareBracketBegin = '[',
		ParenthesisBegin = '(',
		AngleBracketBegin = '<',
		None = '\0',

		Invalid = '\0'
	};
	enum class Suffix : char
	{
		SquareBracketClose = ']',
		ParenthesisClose = ')',
		AngleBracketClose = '>',
		None = '\0',

		Invalid = '\0'
	};
	enum class Sep : char
	{
		Comma = ',',
		Semicolon = ';'
	};

	formatter<_Tx> _formatter_x;
	formatter<_Ty> _formatter_y;
	Prefix _prefix = Prefix::None;
	Suffix _suffix = Suffix::None;
	Sep _sep = Sep::Comma;

	static constexpr auto bracket(char pre, char suf) noexcept;

	auto parse(FormatParseContext &ctx);
	auto format(const _Pair &value, FormatContext &ctx) const;
};

template <typename _Pair>
constexpr auto formatter<_Pair, std::enable_if_t<is_generalized_pair_v<_Pair>>>::bracket(char pre, char suf) noexcept
{
	if (pre == '(' && suf == ')')
		return std::make_tuple(Prefix::ParenthesisBegin, Suffix::ParenthesisClose);
	else if (pre == '<' && suf == '>')
		return std::make_tuple(Prefix::AngleBracketBegin, Suffix::AngleBracketClose);
	else if (pre == '[' && suf == ']')
		return std::make_tuple(Prefix::SquareBracketBegin, Suffix::SquareBracketClose);
	else
		return std::make_tuple(Prefix::Invalid, Suffix::Invalid);
}

template <typename _Pair>
auto formatter<_Pair, std::enable_if_t<is_generalized_pair_v<_Pair>>>::parse(FormatParseContext &ctx)
{

	if (ctx.peek() == ':')
		ctx.consume();
	else
		return ctx.empty() ? 0 : (int)FormatError::InvalidFormatSpec;

	// 说明符只有 ':' 时，两个字段都使用默认格式
	if (ctx.empty())
		return (int)FormatError::Success;

	// 注意：这里统一用「下标偏移 + string_view::substr」来切分子说明符，
	// 而不要拿 string_view 的迭代器去构造 string_view。各标准库中
	// string_view::iterator 的具体类型不同（MSVC 上并非裸指针，
	// 且 _HAS_CXX23 在 C++17 模式下也会被定义为 0，使原先的 #if 分支判断失效）。
	const std::string_view spec = ctx.remainder();

	std::tie(this->_prefix, this->_suffix) = bracket(spec.front(), spec.back());

	size_t x_begin = 0;
	size_t y_end = spec.size();
	if (Prefix::None != this->_prefix)
	{
		x_begin++;
		y_end--;
	}

	// 查找分隔符
	size_t y_begin = x_begin;
	size_t x_end = y_end;
	for (size_t i = 0; i < spec.size(); ++i)
	{
		if (spec[i] == ',' && i + 1 < spec.size() && spec[i + 1] == ',')
		{
			x_end = i;
			y_begin = i + 2;
			this->_sep = Sep::Comma;
		}
		else if (spec[i] == ';')
		{
			x_end = i;
			y_begin = i + 1;
			this->_sep = Sep::Semicolon;
		}
	}

	FormatParseContext ctx_x = spec.substr(x_begin, x_end - x_begin);
	FormatParseContext ctx_y = spec.substr(y_begin, y_end - y_begin);

	auto rslt = (this->_formatter_x).parse(ctx_x);
	if (rslt != (int)FormatError::Success)
	{
		return rslt;
	}
	return (this->_formatter_y).parse(ctx_y);
}
template <typename _Pair>
auto formatter<_Pair, std::enable_if_t<is_generalized_pair_v<_Pair>>>::format(const _Pair &value, FormatContext &ctx) const
{
	const auto &[x, y] = value;
	ctx.write_safe(char(this->_prefix));
	auto rslt = this->_formatter_x.format(x, ctx);
	if (rslt != (int)FormatError::Success)
	{
		return rslt;
	}
	ctx.write_safe(char(this->_sep));
	ctx.write_safe(' ');
	rslt = this->_formatter_y.format(y, ctx);
	ctx.write_safe(char(this->_suffix));
	return rslt;
}

// TODO:添加更多特化...

// ------------------------------------------实现细节--------------------------------------------------

EMBMARTIN_DETAIL_NAMESPACE_BEGIN

// 编译期计算占位符数量
constexpr size_t count_placeholders(std::string_view fmt)
{
	size_t count = 0;
	bool in_brace = false;

	for (size_t i = 0; i < fmt.size(); ++i)
	{
		if (fmt[i] == '{')
		{
			// 检查转义
			if (i + 1 < fmt.size() && fmt[i + 1] == '{')
			{
				++i; // 跳过转义
			}
			else
			{
				in_brace = true;
			}
		}
		else if (fmt[i] == '}')
		{
			if (in_brace)
			{
				++count;
				in_brace = false;
			}
		}
	}
	return count;
}

// 解析格式字符串中的一个部分
template <typename Arg>
int format_arg(FormatContext &ctx, std::string_view fmt_spec, const Arg &arg)
{
	// 创建格式化器
	formatter<Arg> fmt;
	FormatParseContext parse_ctx(fmt_spec);

	// 解析格式说明符
	int parse_result = fmt.parse(parse_ctx);
	if (parse_result < 0)
	{
		return parse_result;
	}

	// 格式化参数
	auto rslt = fmt.format(arg, ctx);
	return rslt;
}

// 递归展开参数包的辅助函数
template <typename... Args>
struct FormatImpl;

// 基础情况：没有参数
template <>
struct FormatImpl<>
{
	static int format(FormatContext &ctx, std::string_view fmt)
	{
		// 直接输出剩余的格式字符串
		ctx.write_safe(fmt);
		return ctx.write_safe('\0') ? int(FormatError::Success) : static_cast<int>(FormatError::BufferOverflow);
	}
};

// 递归情况：有参数需要处理
template <typename Arg, typename... Rest>
struct FormatImpl<Arg, Rest...>
{
	static int format(FormatContext &ctx, std::string_view fmt,
					  const Arg &arg, const Rest &...rest)
	{

		static_assert(is_formattable_v<Arg>, "arg not formattable!");

		size_t pos = 0;
		while (pos < fmt.size())
		{
			// 查找下一个占位符
			if (fmt[pos] == '{')
			{
				// 检查是否是转义 {{
				if (pos + 1 < fmt.size() && fmt[pos + 1] == '{')
				{
					// 输出单个{
					if (!ctx.write_safe('{'))
					{
						return static_cast<int>(FormatError::BufferOverflow);
					}
					pos += 2;
					continue;
				}

				// 找到占位符开始，查找结束}
				size_t end = pos + 1;
				while (end < fmt.size() && fmt[end] != '}')
				{
					++end;
				}

				if (end >= fmt.size())
				{
					// 没有找到匹配的}
					return static_cast<int>(FormatError::MismatchedBraces);
				}

				// 输出占位符前的文本
				if (pos > 0)
				{
					if (!ctx.write_safe(fmt.substr(0, pos)))
					{
						return static_cast<int>(FormatError::BufferOverflow);
					}
				}

				// 提取格式说明符
				std::string_view spec = fmt.substr(pos + 1, end - pos - 1);

				// 格式化当前参数
				int result = format_arg(ctx, spec, arg);
				if (result < 0)
				{
					return result;
				}

				// 继续处理剩余的格式字符串和参数
				std::string_view remaining_fmt = fmt.substr(end + 1);
				return FormatImpl<Rest...>::format(ctx, remaining_fmt, rest...);
			}
			++pos;
		}

		// 没有找到占位符，直接输出剩余文本
		return ctx.write_safe(fmt) ? static_cast<int>(fmt.size()) : static_cast<int>(FormatError::BufferOverflow);
	}
};

EMBMARTIN_DETAIL_NAMESPACE_END

// ------------------------------------------主接口函数--------------------------------------------------

template <size_t N, typename... Args>
int format_to(char *buffer, size_t capacity,
			  const FormatString<N> &fmt_str, const Args &...args)
{
	// 参数数量检查（编译期）
	// constexpr size_t expected_args = detail::count_placeholders(fmt_str.view());
	// static_assert(sizeof...(Args) == expected_args,
	// 	"Number of arguments does not match format string");
	// TODO:

	FormatContext ctx(buffer, capacity);
	return detail::FormatImpl<Args...>::format(ctx, fmt_str.view(), args...);
}

EMBMARTIN_NAMESPACE_END

#endif // EMBMARTIN_FORMAT_STRING_H