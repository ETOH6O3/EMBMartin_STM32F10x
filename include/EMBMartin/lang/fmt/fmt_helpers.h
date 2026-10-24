#pragma once

// fmt 组件的共用辅助设施（自 fmt.h 拆出）：
//   - String<N> 前向声明
//   - namespace pair_spec  （广义 pair 说明符切分，两套后端共用）
//   - namespace fixed_detail（定点小数渲染，两套后端共用）

#include "EMBMartin/lang/fmt/fmt_config.h"

EMBMARTIN_NAMESPACE_BEGIN

// ------------------------------------------前向声明--------------------------------------------------

/**
 * @brief @c String<N> 的前向声明
 *
 * @details @c String<N> 定义在 @c mstring.h ，其 @c etl::formatter 特化亦定义在
 * @c mstring.h 末尾。@c fmt.h 不包含 @c mstring.h ，以免两个头文件互相包含。
 *
 * @tparam N 内部缓冲区总字节数（含结尾 @c '\\0' ）
 */
template <size_t N>
class String;

// ------------------------------------------广义 pair 说明符（两套后端共用）--------------------------------------------------


/**
 * @namespace EMBMartin::pair_spec
 * @brief 广义 pair（@c Coordinate / @c add_result / @c std::pair / @c std::tuple ...）说明符的切分工具
 *
 * @details
 * 本命名空间只做「切分」，不解释子说明符本身；子说明符交给各后端自己的说明符解析器。
 * 两套后端共用这一份切分逻辑，以保证 pair 说明符的语法边界一致。
 *
 * 语法（与旧实现一致）：
 * @verbatim
   [左括号][x 子说明符][,, 或 ;[y 子说明符]][右括号]

   left bracket:  (  [  <  或省略（省略时右括号也必须省略）
   sep:           ",," 或 ';'；不给时 y 沿用 x 的子说明符
   right bracket: )  ]  >，必须与左括号配对
   @endverbatim
 *
 * 例（旧语法原文）：
 * @verbatim
   {:(:#X)}   {:[:#X,,:#b]}   {::#X;:#b}   {:[]}
   @endverbatim
 *
 * @note 「广义 pair」的定义见 @c meta.h 中的 @c is_generalized_pair 。
 */
namespace pair_spec
{
	/**
	 * @brief 刻意保持不完整的「不是可解包 pair」哨兵类型
	 *
	 * @details 该类型只有声明、没有定义。把它用作某个模板特化的模板实参时，只要该特化被
	 * 选中就会因类型不完整而替换失败（SFINAE），从而把「不满足条件」的情况排除掉。
	 *
	 * @warning 不要补全该类型的定义，否则依赖它的 SFINAE 约束会失效。
	 */
	struct not_a_pair;

	/**
	 * @brief 判定 @p _Pair 是否为可安全解包的广义 pair
	 *
	 * @details 比 @c is_generalized_pair_v 多排除指针与引用：ETL 在探测 @c etl::formatter<T>
	 * 时会代入一些内部类型，若把它们误判成 pair，实例化 @c generalized_pair_element_t
	 * 会硬报错（而非优雅地替换失败）。
	 *
	 * @tparam _Pair 待判定的类型
	 *
	 * @see is_generalized_pair_v
	 */
	template <typename _Pair>
	using is_decomposable_pair = std::bool_constant<is_generalized_pair_v<_Pair> &&
													!std::is_pointer_v<_Pair> &&
													!std::is_reference_v<_Pair>>;

	/** @brief 广义 pair 说明符的切分结果 */
	struct parts
	{
		char prefix = '\0'; ///< 左括号；@c '\\0' 表示无括号
		char suffix = '\0'; ///< 右括号；@c '\\0' 表示无括号
		char sep = ',';		///< 元素分隔符，@c ',' 或 @c ';'
		etl::string_view x; ///< 左元素子说明符（含前导 @c ':' ，可为空）
		etl::string_view y; ///< 右元素子说明符（含前导 @c ':' ，可为空）
	};

	/**
	 * @brief 切分广义 pair 的说明符
	 *
	 * @param spec 说明符原文，**不含**前导 @c ':' 与收尾 @c '}'
	 * @param out  [out] 切分结果
	 *
	 * @return 始终返回 @c true
	 * @retval true 切分完成
	 *
	 * @note @p spec 为空时 @c out.x 与 @c out.y 均为空，表示两个元素都使用默认表示。
	 * @note 若找到了分隔符，@c out.y 取其后的部分；否则 @c out.y 与 @c out.x 相同。
	 * @note 本函数不修改 @p out 的 @c prefix / @c suffix 之外的既有内容，
	 *       但会先清空 @c out.x 与 @c out.y 。
	 */
	inline bool split(etl::string_view spec, parts &out) noexcept
	{
		out.x = etl::string_view();
		out.y = etl::string_view();

		size_t x_begin = 0;
		size_t y_end = spec.size();

		if (!spec.empty())
		{
			const char pre = spec.front();
			const char suf = spec.back();
			if ((pre == '(' && suf == ')') ||
				(pre == '[' && suf == ']') ||
				(pre == '<' && suf == '>'))
			{
				out.prefix = pre;
				out.suffix = suf;
				x_begin = 1;
				y_end = spec.size() - 1;
			}
		}

		size_t x_end = y_end;
		size_t y_begin = y_end;
		bool has_sep = false;
		for (size_t i = x_begin; i < y_end; ++i)
		{
			if (spec[i] == ',' && i + 1 < y_end && spec[i + 1] == ',')
			{
				x_end = i;
				y_begin = i + 2;
				out.sep = ',';
				has_sep = true;
			}
			else if (spec[i] == ';')
			{
				x_end = i;
				y_begin = i + 1;
				out.sep = ';';
				has_sep = true;
			}
		}

		out.x = spec.substr(x_begin, x_end - x_begin);
		out.y = has_sep ? spec.substr(y_begin, y_end - y_begin) : out.x;
		return true;
	}

	/**
	 * @brief 判断元素级子说明符是否被显式给出
	 *
	 * @details 用于决定格式化时是否用元素级说明符覆盖外层说明符：只要有一侧给了子说明符，
	 * 就认为用户意图是「分别指定两个元素的格式」。
	 *
	 * @param p 已切分好的说明符
	 *
	 * @return 是否有元素级子说明符
	 * @retval true  @c p.x 或 @c p.y 非空
	 * @retval false 两者皆空，表示使用外层说明符
	 */
	inline bool has_element_spec(const parts &p) noexcept
	{
		return !p.x.empty() || !p.y.empty();
	}
} // namespace pair_spec

// ------------------------------------------定点小数（fixed<N>）--------------------------------------------------

/**
 * @brief 固定小数位的浮点包装类型
 *
 * @details
 * @c fixed<N> 本身不是格式化器，而是一个**标记类型**：它告诉 fmt 组件「这个值要按
 * 恰好 @c N 位小数输出」。为它提供的 formatter 用**整数运算**渲染，因此：
 * - 两套后端（ETL / 旧核心）输出完全一致，不受 @c EMBMARTIN_FMT_USE_ETL 影响；
 * - 不受 ETL 20.49.0 那个「浮点 precision 被忽略、永远 6 位」缺陷的影响
 *   （该缺陷在 ETL 上游 master 仍未修复，详见 @ref fmt_fixed_precision 一节）；
 * - 不依赖 @c long@c double ，故不会出现旧核心那种「平局舍入随编译器变」的问题。
 *
 * 用法：
 * @code
 * double d = 1.2345;
 * s.format("Dist: {}m", EMBMartin::fixed<2>{d});   // "Dist: 1.23m"
 * s.format("[{:8.3}]", EMBMartin::fixed<2>{d});    // 说明符里的 precision 覆盖 N → 1.234
 * oled.println("Dist: {}m", EMBMartin::fixed<2>{ultrasonic_sensor});
 * @endcode
 *
 * 支持的说明符：@c [[fill]align][sign][0][width][.precision][f|F]
 * - @c align 取 @c < @c > @c ^ ；
 * - @c sign 取 @c + @c - @c 空格；
 * - @c 0 表示零填充；
 * - @c precision 覆盖模板参数 @c N 。
 *
 * 不支持 @c e / @c E / @c g / @c G / @c a / @c A ：给了会断言失败，不会静默出错。
 *
 * @warning 本类型与 @c std::fixed （I/O 操纵符）**同名**。若某个翻译单元里同时有
 *          @c "using@c namespace@c std;" 与 @c "using@c namespace@c EMBMartin;" ，
 *          非限定的 @c fixed<N> 会二义（是否报错取决于标准库是否间接引入了 @c \<ios\> ：
 *          armclang 的 libc++ 会，MSVC/GCC 在只包含 @c \<memory\> / @c \<string_view\> 时不会）。
 *          此时请写限定名 @c EMBMartin::fixed<N>{...} 。
 *
 * @tparam N 默认小数位数，取值范围 0 ~ 9
 */
template <size_t N>
struct fixed
{
	static_assert(N <= 9, "EMBMartin::fixed<N> 的小数位数最多 9 位");

	/** @brief 被包装的值（统一用 @c double 承载） */
	double value;

	/**
	 * @brief 构造一个定点小数包装
	 *
	 * @param v 被包装的值，默认 0.0
	 */
	constexpr fixed(double v = 0.0) noexcept : value(v) {}
};

/**
 * @namespace EMBMartin::fixed_detail
 * @brief @ref EMBMartin::fixed 的后端无关渲染实现
 *
 * @details 两套后端的 formatter 只负责把各自的说明符翻译成 @ref spec ，渲染全部走这里，
 * 因此输出必然一致。
 */
namespace fixed_detail
{
	/** @brief 与后端无关的格式化说明符 */
	struct spec
	{
		char fill = ' ';		///< 填充字符
		char align = '>';		///< 对齐方式：@c '<' @c '>' @c '^'
		int width = 0;			///< 最小字段宽度；0 表示不限
		char sign = '-';		///< 符号：@c '+' / @c '-' / @c ' '
		bool zero_fill = false; ///< 是否在符号与数字之间补零
		bool upper = false;		///< 表示类型为 @c F 时，NAN / INF 输出大写
		int precision = -1;		///< 大于等于 0 时覆盖 @c fixed<N> 的 @c N
	};

	/** @brief 渲染结果所需的最小缓冲区字节数（符号 1 + 整数 20 + @c '.' + 小数 9 + 余量） */
	constexpr size_t buffer_size = 48;

	/**
	 * @brief 把 @p value 按 @p default_decimals 位小数渲染进 @p buf
	 *
	 * @param buf              输出缓冲区，至少 @ref buffer_size 字节
	 * @param value            被格式化的值
	 * @param default_decimals 说明符未给 precision 时使用的小数位数
	 * @param s                说明符
	 *
	 * @return 写入 @p buf 的字符数（不含结尾 @c '\\0' ）
	 *
	 * @note 舍入是「四舍五入、半值进位」，全程 @c double 与整数运算，跨编译器可复现。
	 * @note NaN 不输出符号（x86 上 @c 0.0/0.0 会得到负 NaN）；±Inf 保留符号。
	 * @note @c |value|*10^N 会被夹到 @c unsigned@c long@c long 上限内，超出的极端输入只做截断。
	 */
	inline size_t render(char *buf, double value, unsigned default_decimals, const spec &s) noexcept
	{
		unsigned dec = (s.precision >= 0) ? static_cast<unsigned>(s.precision) : default_decimals;
		if (dec > 9)
		{
			dec = 9;
		}

		char body[fixed_detail::buffer_size];
		size_t len = 0;

		bool negative = std::signbit(value) != 0;
		const double abs_value = negative ? -value : value;

		if (std::isnan(abs_value))
		{
			// NaN 的符号位没有意义，不输出符号（x86 上 0.0/0.0 会得到负 NaN）
			negative = false;
			const char *text = s.upper ? "NAN" : "nan";
			while (*text)
			{
				body[len++] = *text++;
			}
		}
		else if (std::isinf(abs_value))
		{
			const char *text = s.upper ? "INF" : "inf";
			while (*text)
			{
				body[len++] = *text++;
			}
		}
		else
		{
			unsigned long long scale = 1ULL;
			for (unsigned i = 0; i < dec; ++i)
			{
				scale *= 10ULL;
			}

			// 保证 abs_value * scale 落在 unsigned long long 内，避免 UB（极端输入只做截断）
			const double limit = 9.0e18 / static_cast<double>(scale);
			const double a = abs_value > limit ? limit : abs_value;

			double integral_d = 0.0;
			const double fractional = std::modf(a, &integral_d);

			unsigned long long integral = static_cast<unsigned long long>(integral_d);
			unsigned long long frac =
				static_cast<unsigned long long>(fractional * static_cast<double>(scale) + 0.5);
			if (frac >= scale) // 0.999... 进位
			{
				frac = 0ULL;
				++integral;
			}

			// 整数部分（逆序生成后翻转），至少一位
			char rev[24];
			int rn = 0;
			if (integral == 0ULL)
			{
				rev[rn++] = '0';
			}
			while (integral != 0ULL)
			{
				rev[rn++] = static_cast<char>('0' + static_cast<int>(integral % 10ULL));
				integral /= 10ULL;
			}
			for (int i = rn - 1; i >= 0; --i)
			{
				body[len++] = rev[i];
			}

			// 小数部分，定长补零
			if (dec > 0)
			{
				body[len++] = '.';
				unsigned long long div = 1ULL;
				for (unsigned i = 1; i < dec; ++i)
				{
					div *= 10ULL;
				}
				for (unsigned i = 0; i < dec; ++i)
				{
					body[len++] = static_cast<char>('0' + static_cast<int>((frac / div) % 10ULL));
					div /= 10ULL;
				}
			}
		}

		// 符号
		char sign_ch = '\0';
		if (negative)
		{
			sign_ch = '-';
		}
		else if (s.sign == '+')
		{
			sign_ch = '+';
		}
		else if (s.sign == ' ')
		{
			sign_ch = ' ';
		}

		const size_t content = len + (sign_ch != '\0' ? 1u : 0u);
		size_t pad = (s.width > 0 && static_cast<size_t>(s.width) > content)
						 ? static_cast<size_t>(s.width) - content
						 : 0u;

		size_t at = 0;
		if (s.zero_fill && s.align == '>')
		{
			// 零填充插在符号与数字之间
			if (sign_ch != '\0')
			{
				buf[at++] = sign_ch;
			}
			for (size_t i = 0; i < pad; ++i)
			{
				buf[at++] = '0';
			}
			for (size_t i = 0; i < len; ++i)
			{
				buf[at++] = body[i];
			}
		}
		else
		{
			size_t pre = 0;
			if (s.align == '^')
			{
				pre = pad / 2;
			}
			else if (s.align == '>')
			{
				pre = pad;
			}

			for (size_t i = 0; i < pre; ++i)
			{
				buf[at++] = s.fill;
			}
			if (sign_ch != '\0')
			{
				buf[at++] = sign_ch;
			}
			for (size_t i = 0; i < len; ++i)
			{
				buf[at++] = body[i];
			}
			for (size_t i = pre; i < pad; ++i)
			{
				buf[at++] = s.fill;
			}
		}

		buf[at] = '\0';
		return at;
	}
} // namespace fixed_detail

EMBMARTIN_NAMESPACE_END
