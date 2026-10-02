/**
 ******************************************************************************
 * @file    fmt.h
 * @author  孙鸣淼
 * @brief   EMBMartin 格式化字符串组件（旧核心 / ETL 双后端）
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 * 3. 本模块的程序存储器开销较为严重，需要谨慎使用；若用于嵌入式系统，建议将编译器优化调至 -O3 或 -Oz
 * 4. 本模块对于浮点数的处理过于面向结果，性能甚至远不及 printf ，建议谨慎使用
 ******************************************************************************
 *
 * ============================== 双后端与命名空间设计 ==============================
 *
 * 本组件同时存在两套底层实现，由 `EMBMARTIN_FMT_USE_ETL`（定义见 macro.h，可由预定义宏覆盖）
 * 选择哪一个是**内联命名空间**：
 *
 *   +---------------------------+---------------------------------------------------------------------------------+
 *   | EMBMARTIN_FMT_USE_ETL = 1 | `embmartin::etl_fmt` 内联；底层直转发第三方库 ETL 的 format                    |
 *   | EMBMARTIN_FMT_USE_ETL = 0 | `embmartin::legacy_fmt` 内联；底层为本库既有的旧格式化核心（原样保留）          |
 *   +---------------------------+---------------------------------------------------------------------------------+
 *
 * 命名空间分层（`embmartin` 即 `EMBMartin`）：
 *
 *   EMBMartin
 *   ├── etl_fmt        ETL 直转发后端。宏为 1 时它是内联命名空间
 *   ├── legacy_fmt     旧格式化核心 + ETL 风格接口适配。宏为 0 时它是内联命名空间
 *   └── pair_spec      两套后端共用的「广义 pair 说明符」切分工具
 *
 * @note `EMBMARTIN_FMT_USE_ETL` 只决定哪一个命名空间内联，**不取消**另一个的定义：两套实现
 *       始终都被编译，因此始终可以写出 `EMBMartin::legacy_fmt::...` 或 `EMBMartin::etl_fmt::...`
 *       来显式指定后端。任何时刻都恰好只有一个内联命名空间，故 `EMBMartin::format_to(...)`
 *       这类写法不会二义。
 *
 * @note 两套后端都对外的**同一套 ETL 风格接口**（见下），所以调用方不需要用宏切换写法：
 *         format_to / format_to_n / vformat_to / formatted_size
 *       外加 ETL 类型别名 formatter / format_context / format_parse_context / format_string /
 *       format_args / basic_format_arg / make_format_args 。
 *
 * ---------------------------------------- 错误处理 ----------------------------------------
 *
 * 与 ETL 一致的**风格**：非法格式串、无法满足的格式说明符**不返回错误码**，而是断言。
 * 两套后端共用一个上报点 `EMBMARTIN_FMT_ASSERT`（默认 `assert`），因此对外语义完全相同。
 *
 * @note 为什么默认是 `assert` 而不是直接用 ETL 的 `ETL_ASSERT`？
 *       本工程使用 `TPL/my_etl_profile.h`，其中定义了 `ETL_NO_EXCEPTIONS` 且没有定义
 *       `ETL_DEBUG` / `ETL_LOG_ERRORS` / `ETL_USE_ASSERT_FUNCTION`，于是 ETL 的
 *       `ETL_ASSERT(b, e)` 在 error_handler.h 里落到 `static_cast<void>(sizeof(b))`
 *       ——**整条检查会被编译掉**。若把本组件的断言也改成 `ETL_ASSERT`，两套后端在发布构建里
 *       都会静默放过非法格式串。为保持「新旧后端行为一致且可控」，这里默认用 `assert`；
 *       想改成走 ETL 的错误处理器，预定义
 *       `EMBMARTIN_FMT_ASSERT(_EXPR) ETL_ASSERT(_EXPR, ETL_ERROR(etl::bad_format_string_exception))`
 *       即可。
 *
 * @warning `assert` 在 `NDEBUG` 下同样会被移除。需要运行时保护请覆盖 `EMBMARTIN_FMT_ASSERT`
 *          （例如改走自定义的错误处理器），或使用旧接口的 `FormatError` 错误码（仅 `legacy_fmt`）。
 *
 * ---------------------------------------- 旧接口兼容层 ----------------------------------------
 *
 * `legacy_fmt` 内部**原样保留**旧实现及其旧接口，供仍需 `FormatError` 错误码的代码使用：
 *
 *   FormatError / FormatContext / FormatParseContext / FormatString<N> / FormatLex<T> /
 *   formatter<T>（旧签名的自定义点：parse(FormatParseContext&) -> int、
 *                 format(const T&, FormatContext&) -> int）
 *   旧的 int format_to(char *buffer, size_t capacity, const FormatString<N> &, const Args &...)
 *
 * @note 这不是新接口，不随 `EMBMARTIN_FMT_USE_ETL` 变化；使用它必须写 `EMBMartin::legacy_fmt::`。
 *
 * ---------------------------------------- 语法差异 ----------------------------------------
 *
 * ETL 后端完全遵循 ETL / C++20 std::format 语法：对齐只有 `<` `>` `^`（**没有** `=`），
 * 支持 `+` `-` ` ` `#` `0` `L`、`{}` 形式的嵌套动态宽度与精度、`{0}` 手动参数索引，
 * 表示类型为 `s ? b B c d o x X a A e E f F g G p P`；**不支持** `z`、数值分组（`,` / `_`）、
 * `%`。旧后端的旧语法（`z`、`=` 对齐、数值分组、`%` 等）原样保留。
 *
 * 两套后端都可用的公共子集（仓库内调用点只使用这个子集）：`{}`、`{:d}`、`{:x}`、`{:X}`、`{:#x}`、
 * `{:b}`、`{:o}`、`{:c}`、`{:s}`、`{:N}`、`{:>N}`、`{:<N}`、`{:^N}`、`{:*^N}`、`{:0N}`、`{:+}`、`{: }`。
 *
 * 只有一侧支持的语法（用错后端会断言，不会静默出错）：
 *   - 仅 ETL 后端：`{:?}`、`{:p}`、`B`/`A` 等表示类型、`{0}` 手动索引、`{}` 嵌套宽度/精度、
 *     字符串 precision `{:.Ns}`、指针形参。
 *   - 仅旧后端：`z`、`=` 对齐、数值分组（`,` / `_`）、`%`、`{::x规格;y规格}` 与 `<...>` 括号的
 *     pair 元素级说明符。
 *
 * ---------------------------------------- 浮点小数位（重要） ----------------------------------------
 *
 * **ETL 后端无法控制浮点的小数位数**：ETL 20.49.0 的三个浮点格式化函数
 * （`format_floating_default` / `format_floating_f` / `format_floating_e`）都把小数位数硬编码为 6，
 * 从不读取 `spec.precision`；`{:.2f}`、`{:.0f}`、`{:.4f}` 的输出因此完全相同。
 * 这是 ETL 上游**至今未实现**的功能（已核对 master 分支，代码里仍写着
 * `const size_t fractional_decimals = 6; // default`），不是配置问题。
 *
 * 需要固定小数位时请使用 @ref EMBMartin::fixed "EMBMartin::fixed<N>"：
 *
 * @code
 *   oled.println("Dist: {}m", EMBMartin::fixed<2>{distance});   // "Dist: 1.23m"
 * @endcode
 *
 * `fixed<N>` 用整数运算渲染，**两套后端输出逐字符一致**，也不受旧核心 `long double` 舍入差异影响；
 * 说明符里的 `.precision` 会覆盖 `N`。详见该类型的文档。
 *
 * ---------------------------------------- 已知限制 ----------------------------------------
 *
 * - 旧后端没有「只计数」通道，适配层用 `EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE`（默认 512）字节的
 *   栈缓冲承接一次完整结果；单次格式化结果超过该大小时断言失败。可调大该宏。
 * - 旧后端不支持 `{0}` 手动索引与 `{}` 嵌套动态宽度/精度（旧核心本身不支持）。
 * - ETL 20.49.0 的浮点格式化忽略 `precision`（见上一节）。不要在 ETL 后端依赖 `{:.Nf}`；
 *   要固定小数位就用 `fixed<N>`。
 * - 旧核心的浮点舍入建立在 `long double` 之上。当被丢弃的部分**恰好以 5 开头**（平局）时，
 *   结果取决于 `long double` 的有效位数：MinGW/x86-64 为 80 位（进位的概率更高），
 *   MSVC 与 armclang(ARM) 为 64 位。例如旧后端 `{:.3e}` 对 `1234.5f` 可能是 `1.235e+003`
 *   也可能是 `1.234e+003`。这是被原样保留的旧实现自身的性质，与本次迁移无关；
 *   `fixed<N>` 与 ETL 后端的其它路径不受影响。
 */
#ifndef EMBMARTIN_FORMAT_STRING_H
#define EMBMARTIN_FORMAT_STRING_H

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <tuple>
#include <type_traits>

#include <etl/format.h>
#include <etl/string.h>
#include <etl/string_view.h>

#include "macro.h"
#include "meta.h"

// ------------------------------------------错误上报策略--------------------------------------------------

/**
 * @brief 新版（ETL 风格）接口的错误上报宏
 *
 * 与 ETL 的 `ETL_ASSERT` 同构：非法格式串、无法满足的格式说明符直接断言。
 * 两套后端都用这一个宏，故对外错误语义完全一致。
 *
 * @note 默认用 `assert` 而非 `ETL_ASSERT`：本工程的 ETL profile 让 `ETL_ASSERT` 变成空操作
 *       （详见本文件头部「错误处理」一节的说明）。想接入 ETL 的错误处理器，
 *       预定义 `EMBMARTIN_FMT_ASSERT(_EXPR) ETL_ASSERT(_EXPR, ETL_ERROR(etl::bad_format_string_exception))`。
 */
#ifndef EMBMARTIN_FMT_ASSERT
#define EMBMARTIN_FMT_ASSERT(_EXPR) assert(_EXPR)
#endif // EMBMARTIN_FMT_ASSERT

/**
 * @brief 旧后端适配层承接结果的栈缓冲区大小
 *
 * 旧核心只能写入一整块连续缓冲区，且没有「只计数」模式，适配层因此需要一块栈缓冲。
 * 单次格式化结果超过此大小时断言失败（而不是静默给出错误结果）。
 */
#ifndef EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE
#define EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE 512
#endif // EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE

// ------------------------------------------内联命名空间选择--------------------------------------------------

/**
 * @brief 把 `EMBMARTIN_FMT_USE_ETL` 映射成命名空间定义上的 `inline` 限定符
 *
 * 用宏而不是在命名空间定义之后写 `inline namespace xxx;`，是为了让两套后端各自只被定义一次
 * （避免为了实现「只有一个内联」而把两份实现正文重复两遍）。
 */
#if EMBMARTIN_FMT_USE_ETL
#define EMBMARTIN_FMT_INLINE_ETL inline
#define EMBMARTIN_FMT_INLINE_LEGACY
#else
#define EMBMARTIN_FMT_INLINE_ETL
#define EMBMARTIN_FMT_INLINE_LEGACY inline
#endif // EMBMARTIN_FMT_USE_ETL

EMBMARTIN_NAMESPACE_BEGIN

// ------------------------------------------前向声明--------------------------------------------------

// String<N> 定义在 mstring.h，其 formatter 特化也定义在 mstring.h 末尾。
// fmt.h 不包含 mstring.h，以免两个头文件互相包含。
template <size_t N>
class String;

// ------------------------------------------广义 pair 说明符（两套后端共用）--------------------------------------------------

/**
 * @brief 广义 pair（Coordinate / add_result / std::pair / std::tuple ...）说明符的切分工具
 *
 * 语法（与旧实现一致）：`[左括号][x 子说明符][,, 或 ;[y 子说明符]][右括号]`
 *
 *   left bracket:  `(`  `[`  `<`  或省略（省略时右括号也必须省略）
 *   sep:           `,,` 或 `;`；不给时 y 沿用 x 的子说明符
 *   right bracket: `)`  `]`  `>`，必须与左括号配对
 *
 * 例（旧语法原文）：`{:(:#X)}`、`{:[:#X,,:#b]}`、`{::#X;:#b}`、`{:[]}`
 *
 * @note 这里只做「切分」，不解释子说明符本身——子说明符交给各后端自己的说明符解析器。
 */
namespace pair_spec
{
	/** @brief 刻意保持不完整：作为「不是可解包 pair」的哨兵，使选择它的特化替换失败 */
	struct not_a_pair;

	/**
	 * @brief 判定 `_Pair` 是否为可安全解包的广义 pair
	 *
	 * 比 `is_generalized_pair_v` 多排除指针/引用：ETL 在探测 `etl::formatter<T>` 时会代入一些
	 * 内部类型，若把它们误判成 pair，实例化 `generalized_pair_element_t` 会硬报错。
	 */
	template <typename _Pair>
	using is_decomposable_pair = std::bool_constant<is_generalized_pair_v<_Pair> &&
													!std::is_pointer_v<_Pair> &&
													!std::is_reference_v<_Pair>>;

	/** @brief 切分结果 */
	struct parts
	{
		char prefix = '\0';			  ///< 左括号；'\0' 表示无括号
		char suffix = '\0';			  ///< 右括号；'\0' 表示无括号
		char sep = ',';				  ///< 元素分隔符
		etl::string_view x;			  ///< 左元素子说明符（含前导 ':'，可为空）
		etl::string_view y;			  ///< 右元素子说明符（含前导 ':'，可为空）
	};

	/**
	 * @brief 切分广义 pair 的说明符
	 *
	 * @param spec 说明符原文，**不含**前导 ':' 与收尾 '}'
	 * @param out  切分结果
	 * @return 始终成功；`spec` 为空时 `out.x` / `out.y` 为空（表示两个元素都用默认表示）
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

	/** @brief 元素级子说明符是否被显式给出（用于决定是否覆盖外层 spec） */
	inline bool has_element_spec(const parts &p) noexcept
	{
		return !p.x.empty() || !p.y.empty();
	}
} // namespace pair_spec

// ------------------------------------------定点小数（fixed<N>）--------------------------------------------------

/**
 * @brief 固定小数位的浮点包装类型
 *
 * @c fixed<N> 本身不是格式化器，而是一个**标记类型**：它告诉 fmt 组件「这个值要按
 * 恰好 N 位小数输出」。为它提供的 formatter 用**整数运算**渲染，因此：
 *
 * - 两套后端（ETL / 旧核心）输出完全一致，不受 `EMBMARTIN_FMT_USE_ETL` 影响；
 * - 不受 ETL 20.49.0 那个「浮点 precision 被忽略、永远 6 位」缺陷的影响
 *   （该缺陷在 ETL 上游 master 仍未修复，详见 fmt.h 头部说明）；
 * - 不依赖 `long double`，故不会出现旧核心那种「平局舍入随编译器变」的问题。
 *
 * 用法：
 * @code
 *   double d = 1.2345;
 *   s.format("Dist: {}m", EMBMartin::fixed<2>{d});     // "Dist: 1.23m"
 *   s.format("[{:8.3}]", EMBMartin::fixed<2>{d});      // 说明符里的 precision 覆盖 N → 1.234
 *   oled.println("Dist: {}m", EMBMartin::fixed<2>{ultrasonic_sensor});
 * @endcode
 *
 * 支持的说明符：`[[fill]align][sign][0][width][.precision][f|F]`
 *   （align 为 `<` `>` `^`；sign 为 `+` `-`（空格）；`0` 为零填充；precision 覆盖 N）
 * 不支持 `e` / `E` / `g` / `G` / `a` / `A`：给了会断言失败，不会静默出错。
 *
 * @warning 本类型与 `std::fixed`（I/O 操纵符）**同名**。若某个翻译单元里同时有
 *          `using namespace std;` 与 `using namespace EMBMartin;`，非限定的 `fixed<N>`
 *          会二义（是否报错取决于标准库是否间接引入了 `<ios>`——armclang 的 libc++ 会，
 *          MSVC/GCC 在只包含 `<memory>`/`<string_view>` 时不会）。此时请写限定名：
 *          `EMBMartin::fixed<N>{...}`。
 *
 * @tparam N 小数位数，0 ~ 9
 */
template <size_t N>
struct fixed
{
	static_assert(N <= 9, "EMBMartin::fixed<N> 的小数位数最多 9 位");

	/** @brief 被包装的值（统一用 double 承载） */
	double value;

	constexpr fixed(double v = 0.0) noexcept : value(v) {}
};

/**
 * @brief fixed<N> 的后端无关渲染实现
 *
 * 两套后端的 formatter 只负责把各自的说明符翻译成 @ref spec，渲染全部走这里，
 * 因此输出必然一致。
 */
namespace fixed_detail
{
	/** @brief 与后端无关的格式化说明符 */
	struct spec
	{
		char fill = ' ';		///< 填充字符
		char align = '>';		///< 对齐：`<` `>` `^`
		int width = 0;			///< 最小字段宽度；0 表示不限
		char sign = '-';		///< 符号：`+` / `-` / ` `
		bool zero_fill = false; ///< 是否在符号与数字之间补零
		bool upper = false;		///< `F`：NAN / INF 用大写
		int precision = -1;		///< >= 0 时覆盖 fixed<N> 的 N
	};

	/** @brief 渲染结果所需的最小缓冲区字节数（符号 1 + 整数 20 + '.' + 小数 9 + 余量） */
	constexpr size_t buffer_size = 48;

	/**
	 * @brief 把 value 按 `dec` 位小数渲染进 buf
	 *
	 * @param buf 输出缓冲，至少 @ref buffer_size 字节
	 * @param value 被格式化的值
	 * @param default_decimals 说明符未给 precision 时使用的小数位数
	 * @param s 说明符
	 * @return 写入 buf 的字符数（不含结尾 '\\0'）
	 *
	 * @note 舍入是「四舍五入、半值进位」，全程 double/整数运算，跨编译器可复现。
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

// #################################################################################################
// #                                                                                               #
// #   后端一：legacy_fmt —— 旧格式化核心（原样保留） + ETL 风格接口适配                            #
// #                                                                                               #
// #################################################################################################

EMBMARTIN_FMT_INLINE_LEGACY namespace legacy_fmt
{

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
	bool write_safe(etl::string_view str)
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
	etl::string_view view() const
	{
		return etl::string_view(buffer_, position_);
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
	etl::string_view spec_;
	size_t pos_;

public:
	FormatParseContext(const etl::string_view &spec)
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
	etl::string_view remainder() const
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
	constexpr etl::string_view view() const
	{
		return etl::string_view(str_, N - 1);
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

namespace core_detail
{

int decimal_exponent(long double value) noexcept;
void round_digits(char *digits, int &length, int keep, char guard) noexcept;
void increment_decimal(char *digits) noexcept;

void make_float_parts(long double value, char type,
					  int precision, char *integer, char *fraction, char *exponent) noexcept;

}

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
					rev_int_digits[i] = temp - 10 + (this->_type == BaseType::Type::HexLower ? 'a' : 'A');

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
			core_detail::make_float_parts(numeric_value, char(this->_type), precision,
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
// 注意：这里判断的是“能否由 T 构造出 etl::string_view”（即 T 是字符串类对象/字符指针/字符数组），
// 而不是“能否由 etl::string_view 构造出 T”。后者对 const char* 恒为 false，
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
	auto s = etl::string_view(value);
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
	const etl::string_view spec = ctx.remainder();

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

// ------------------------------------------定点小数 fixed<N> 的旧核心 formatter--------------------------------------------------

/**
 * @brief @ref EMBMartin::fixed "fixed<N>" 在旧核心下的格式化器
 *
 * 说明符的解析复用旧核心的 @ref FormatLex（fill / align / sign / `0` / width / `.precision`），
 * 但**渲染走 @ref EMBMartin::fixed_detail::render 的整数运算**，与 ETL 后端共用同一份实现，
 * 因此两套后端输出逐字符一致，也不会碰到旧核心「平舍入随 long double 位数变化」的坑。
 *
 * @note `_prec` 在调用 @ref FormatLex::parse 之前被置为 -1，用来区分「说明符里没写 precision」
 *       （保持 -1，用 fixed<N> 的 N）与「写了 precision」（≥ 0，覆盖 N）。旧核心自身的算术
 *       formatter 仍然使用 `default_prec`，不受影响。
 */
template <size_t N>
struct formatter<::EMBMartin::fixed<N>, void> : public FormatLex<::EMBMartin::fixed<N>>
{
	using BaseType = FormatLex<::EMBMartin::fixed<N>>;

	int parse(FormatParseContext &ctx)
	{
		// fixed<N> 默认按定点小数处理；说明符里给了别的表示类型会在 format 里被拒绝
		this->_type = BaseType::Type::fFixed;
		this->_prec = -1; // 哨兵：区分「未给 precision」
		return BaseType::parse(ctx);
	}

	int format(const ::EMBMartin::fixed<N> &value, FormatContext &ctx) const
	{
		using Type = typename BaseType::Type;
		if (this->_type != Type::fFixed && this->_type != Type::FFixed)
		{
			// 只支持 f / F：e、g、a 等定点以外的表示类型没有意义
			return static_cast<int>(FormatError::InvalidFormatSpec);
		}
		if (this->_int_part_grouping != BaseType::grouping() ||
			this->_frac_part_grouping != BaseType::grouping() ||
			this->_prefix || this->_z)
		{
			return static_cast<int>(FormatError::InvalidFormatSpec);
		}

		::EMBMartin::fixed_detail::spec s;
		s.fill = this->_fill;
		s.align = char(this->_align);
		s.width = this->_width;
		s.zero_fill = this->_zero_fill;
		s.upper = (this->_type == Type::FFixed);
		s.precision = this->_prec;
		switch (this->_sign)
		{
		case BaseType::Sign::All:
			s.sign = '+';
			break;
		case BaseType::Sign::SpaceOrSign:
			s.sign = ' ';
			break;
		default:
			s.sign = '-';
			break;
		}

		char buf[::EMBMartin::fixed_detail::buffer_size];
		const size_t written = ::EMBMartin::fixed_detail::render(buf, value.value, static_cast<unsigned>(N), s);
		if (!ctx.write_safe(etl::string_view(buf, written)))
		{
			return static_cast<int>(FormatError::BufferOverflow);
		}
		return static_cast<int>(FormatError::Success);
	}
};

// ------------------------------------------实现细节--------------------------------------------------

namespace core_detail
{

// 编译期计算占位符数量
constexpr size_t count_placeholders(etl::string_view fmt)
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
int format_arg(FormatContext &ctx, etl::string_view fmt_spec, const Arg &arg)
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
	static int format(FormatContext &ctx, etl::string_view fmt)
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
	static int format(FormatContext &ctx, etl::string_view fmt,
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
				etl::string_view spec = fmt.substr(pos + 1, end - pos - 1);

				// 格式化当前参数
				int result = format_arg(ctx, spec, arg);
				if (result < 0)
				{
					return result;
				}

				// 继续处理剩余的格式字符串和参数
				etl::string_view remaining_fmt = fmt.substr(end + 1);
				return FormatImpl<Rest...>::format(ctx, remaining_fmt, rest...);
			}
			++pos;
		}

		// 没有找到占位符，直接输出剩余文本
		return ctx.write_safe(fmt) ? static_cast<int>(fmt.size()) : static_cast<int>(FormatError::BufferOverflow);
	}
};

}

// ------------------------------------------主接口函数--------------------------------------------------

template <size_t N, typename... Args>
int format_to(char *buffer, size_t capacity,
			  const FormatString<N> &fmt_str, const Args &...args)
{
	// 参数数量检查（编译期）
	// constexpr size_t expected_args = core_detail::count_placeholders(fmt_str.view());
	// static_assert(sizeof...(Args) == expected_args,
	// 	"Number of arguments does not match format string");
	// TODO:

	FormatContext ctx(buffer, capacity);
	return core_detail::FormatImpl<Args...>::format(ctx, fmt_str.view(), args...);
}

	// ################################################################################################
	// #                                                                                              #
	// #   ETL 风格接口适配（旧核心之上）                                                               #
	// #                                                                                              #
	// #   目的：让 `EMBMARTIN_FMT_USE_ETL` 为 0 时，仓库里所有使用 ETL 风格接口的调用点无需改动        #
	// #         即可编译并正确运行。类型别名与函数名和 etl_fmt 完全一致。                               #
	// #                                                                                              #
	// #   错误处理：与 ETL 一致，非法格式串走 EMBMARTIN_FMT_ASSERT，不返回错误码。                      #
	// #             需要错误码的旧代码请直接使用上面那套旧接口（FormatError / int format_to）。          #
	// #                                                                                              #
	// #   自定义点：`legacy_fmt::formatter<T>` 仍是**旧签名**的（parse(FormatParseContext&) -> int、   #
	// #             format(const T&, FormatContext&) -> int）。这与 ETL 的 formatter 签名不同，        #
	// #             属于两套后端固有的差异（旧实现无法支持 ETL 那套 parse/format 签名）。               #
	// #                                                                                              #
	// ################################################################################################

	// ------------------------------------------ETL 类型别名--------------------------------------------------

	using etl::basic_format_arg;
	using etl::basic_format_args;
	using etl::basic_format_string;
	using etl::format_arg;
	using etl::format_args;
	using etl::format_string;
	using etl::make_format_args;

	using basic_format_parse_context = etl::basic_format_parse_context<char>;
	using format_parse_context = etl::format_parse_context;

	template <typename OutputIt, typename CharT>
	using basic_format_context = etl::basic_format_context<OutputIt, CharT>;

	template <typename OutputIt>
	using format_context = etl::format_context<OutputIt>;

	// `formatter` / `is_formattable` / `is_formattable_v` 沿用上面旧核心定义的那一套（旧签名）

	// ------------------------------------------ETL 风格入口--------------------------------------------------

	/**
	 * @brief 把整串原样逐字符写出（最多 n 个）
	 *
	 * 旧核心在**无实参**时走 `FormatImpl<>`，语义就是「把剩余的格式串原样写出、不做 `{{ }}` 转义」。
	 * 但那条路径是 **all-or-nothing** 的：整串一次 `write_safe` 装不下就一个字符也不写，而且
	 * 忽略失败、仍返回成功。适配层因此对无实参情形直接做逐字符拷贝 —— 与旧语义完全等价，
	 * 同时保留了「按容量截断」的契约（旧实现的 `String::_format_impl` 也是这么兜的）。
	 */
	template <typename OutputIt>
	OutputIt _write_plain(OutputIt out, etl::string_view text, size_t n) noexcept
	{
		const size_t written = text.size() < n ? text.size() : n;
		for (size_t i = 0; i < written; ++i)
		{
			*out = text[i];
			++out;
		}
		return out;
	}

	/**
	 * @brief 运行期格式串 → 输出迭代器
	 *
	 * @warning 旧核心没有「只计数」通道，本函数用 EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE 字节栈缓冲
	 *          承接一次完整结果；超出该大小时断言失败（而不是静默给出错误结果）。
	 *          无实参时不需要缓冲，直接逐字符拷贝。
	 */
	template <typename OutputIt, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt vformat_to(OutputIt out, etl::string_view fmt_str, const Args &...args)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			return legacy_fmt::_write_plain(out, fmt_str, static_cast<size_t>(-1));
		}
		else
		{
			char scratch[EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE];
			FormatContext ctx(scratch, sizeof(scratch));
			const int result = core_detail::FormatImpl<Args...>::format(ctx, fmt_str, args...);
			EMBMARTIN_FMT_ASSERT(result >= 0);

			const size_t produced = ctx.position();
			for (size_t i = 0; i < produced; ++i)
			{
				*out = scratch[i];
				++out;
			}
			return out;
		}
	}

	/** @brief 运行期格式串 → 输出迭代器（`vformat_to` 的别名，便于与 ETL 名字对齐） */
	template <typename OutputIt, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to(OutputIt out, etl::string_view fmt_str, const Args &...args)
	{
		return legacy_fmt::vformat_to(out, fmt_str, args...);
	}

	/** @brief 字面量格式串 → 输出迭代器 */
	template <typename OutputIt, size_t N, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to(OutputIt out, const char (&fmt_str)[N], const Args &...args)
	{
		return legacy_fmt::vformat_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/**
	 * @brief 最多写 n 个字符；返回已推进的输出迭代器
	 *
	 * @note 返回类型与 ETL 的 `etl::format_to_n` 一致（裸迭代器），不是 format_to_n_result。
	 *       需要「本应写入多少字符」请用 `formatted_size`。
	 */
	template <typename OutputIt, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to_n(OutputIt out, size_t n, etl::string_view fmt_str, const Args &...args)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			// 见 _write_plain 的说明：无实参时不需要缓冲，且旧核心那条路径是 all-or-nothing 的
			return legacy_fmt::_write_plain(out, fmt_str, n);
		}
		else
		{
			/*
			 * 栈缓冲取 min(n + 1, SCRATCH)：只要能确认「真实长度 >= n + 1」就足以判定发生了截断。
			 * 这样在 n 不超过 SCRATCH 时本函数与 ETL 后端的语义完全一致（既不漏报也不误报）。
			 */
			const size_t scratch_size =
				(n + 1 < static_cast<size_t>(EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE))
					? (n + 1)
					: static_cast<size_t>(EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE);

			char scratch[EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE];
			FormatContext ctx(scratch, scratch_size);
			const int result = core_detail::FormatImpl<Args...>::format(ctx, fmt_str, args...);

			const size_t produced = ctx.position();
			// 缓冲被写满 ⇒ 只是截断（预期）；否则 result 必须 >= 0，负值说明格式串非法
			EMBMARTIN_FMT_ASSERT(result >= 0 || produced == scratch_size);

			const size_t written = produced < n ? produced : n;
			for (size_t i = 0; i < written; ++i)
			{
				*out = scratch[i];
				++out;
			}
			return out;
		}
	}

	/** @brief 字面量格式串的最多 n 字符版本 */
	template <typename OutputIt, size_t N, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to_n(OutputIt out, size_t n, const char (&fmt_str)[N], const Args &...args)
	{
		return legacy_fmt::format_to_n(out, n, etl::string_view(fmt_str, N - 1), args...);
	}

	/** @brief 写入 etl::istring（与 ETL 的非标准重载同名同义） */
	template <typename... Args>
	etl::istring::iterator format_to(etl::istring &out, etl::string_view fmt_str, const Args &...args)
	{
		etl::istring::iterator result = legacy_fmt::format_to_n(out.begin(), out.max_size(), fmt_str, args...);
		out.uninitialized_resize(static_cast<size_t>(result - out.begin()));
		return result;
	}

	/** @brief 写入 etl::istring，格式串为字符串字面量 */
	template <size_t N, typename... Args>
	etl::istring::iterator format_to(etl::istring &out, const char (&fmt_str)[N], const Args &...args)
	{
		return legacy_fmt::format_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/** @brief 只计算所需字符数 */
	template <typename... Args>
	size_t formatted_size(etl::string_view fmt_str, const Args &...args)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			// 无实参：旧核心把整串原样写出，长度就是格式串长度（见 _write_plain 的说明）
			return fmt_str.size();
		}
		else
		{
			char scratch[EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE];
			FormatContext ctx(scratch, sizeof(scratch));
			const int result = core_detail::FormatImpl<Args...>::format(ctx, fmt_str, args...);
			EMBMARTIN_FMT_ASSERT(result >= 0);
			return ctx.position();
		}
	}

	/** @brief 只计算所需字符数（字面量格式串） */
	template <size_t N, typename... Args>
	size_t formatted_size(const char (&fmt_str)[N], const Args &...args)
	{
		return legacy_fmt::formatted_size(etl::string_view(fmt_str, N - 1), args...);
	}

	// `is_formattable` / `is_formattable_v` 由上面的旧核心提供（旧签名自定义点的探测），此处不重复定义。
} // namespace legacy_fmt

// #################################################################################################
// #                                                                                               #
// #   后端二：etl_fmt —— 直转发第三方库 ETL 的 format                                              #
// #                                                                                               #
// #   本后端不实现任何格式化核心：说明符解析、参数存储、各类型的格式化全部由 ETL 完成。             #
// #   这里只做三件事：                                                                             #
// #     1. 把「字面量 / 运行期 string_view」两种格式串入口收敛到 ETL 的 vformat_to；                #
// #     2. 用 ETL 的 args_mask 把本次调用不可能命中的分支裁掉，避免无谓的代码膨胀；                 #
// #     3. 把 ETL 的类型别名搬到本命名空间，使两套后端的对外名字一致。                              #
// #                                                                                               #
// #################################################################################################

EMBMARTIN_FMT_INLINE_ETL namespace etl_fmt
{
	// ------------------------------------------ETL 类型别名--------------------------------------------------

	using etl::basic_format_arg;
	using etl::basic_format_args;
	using etl::basic_format_string;
	using etl::format_arg;
	using etl::format_args;
	using etl::format_string;
	using etl::formatter;
	using etl::make_format_args;

	using basic_format_parse_context = etl::basic_format_parse_context<char>;
	using format_parse_context = etl::format_parse_context;

	template <typename OutputIt, typename CharT>
	using basic_format_context = etl::basic_format_context<OutputIt, CharT>;

	template <typename OutputIt>
	using format_context = etl::format_context<OutputIt>;

	/** @brief 与 ETL 内部同源的「可格式化」探测（要求 etl::formatter<T> 同时提供 parse 与 format） */
	using etl::private_format::is_formattable;

	template <typename T>
	constexpr bool is_formattable_v = is_formattable<T>::value;

	// ------------------------------------------ETL 风格入口--------------------------------------------------

	/**
	 * @brief 运行期格式串 → 输出迭代器
	 *
	 * @note 格式串是运行期值，无法走 ETL 的 `format_string`（consteval 构造），
	 *       故直接调用 `etl::vformat_to`。
	 */
	template <typename OutputIt, typename... Args>
	OutputIt vformat_to(OutputIt out, etl::string_view fmt_str, const Args &...args)
	{
		auto store = etl::make_format_args<OutputIt>(args...);
		return etl::vformat_to<OutputIt, etl::private_format::args_mask<Args...>::value>(
			out, fmt_str, etl::format_args<OutputIt>(store));
	}

	/** @brief 运行期格式串 → 输出迭代器 */
	template <typename OutputIt, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to(OutputIt out, etl::string_view fmt_str, const Args &...args)
	{
		return etl_fmt::vformat_to(out, fmt_str, args...);
	}

	/**
	 * @brief 字面量格式串 → 输出迭代器
	 *
	 * @note C++20 下额外构造 `etl::format_string` 以启用 ETL 的编译期格式串检查；
	 *       C++17 下 `basic_format_string` 不做检查（ETL 的 `ETL_CONSTEVAL` 为空）。
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

	/** @brief 最多写 n 个字符；返回已推进的输出迭代器（与 ETL 的 `etl::format_to_n` 一致） */
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

	/** @brief 字面量格式串的最多 n 字符版本 */
	template <typename OutputIt, size_t N, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to_n(OutputIt out, size_t n, const char (&fmt_str)[N], const Args &...args)
	{
		return etl_fmt::format_to_n(out, n, etl::string_view(fmt_str, N - 1), args...);
	}

	/** @brief 写入 etl::istring（与 ETL 的非标准重载同名同义） */
	template <typename... Args>
	etl::istring::iterator format_to(etl::istring &out, etl::string_view fmt_str, const Args &...args)
	{
		etl::istring::iterator result = etl_fmt::format_to_n(out.begin(), out.max_size(), fmt_str, args...);
		out.uninitialized_resize(static_cast<size_t>(result - out.begin()));
		return result;
	}

	/** @brief 写入 etl::istring，格式串为字符串字面量 */
	template <size_t N, typename... Args>
	etl::istring::iterator format_to(etl::istring &out, const char (&fmt_str)[N], const Args &...args)
	{
		return etl_fmt::format_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/** @brief 只计算所需字符数（ETL 的 `formatted_size` 对应物） */
	template <typename... Args>
	size_t formatted_size(etl::string_view fmt_str, const Args &...args)
	{
		etl::private_format::counter_iterator it;
		it = etl_fmt::vformat_to(it, fmt_str, args...);
		return it.value();
	}

	/** @brief 只计算所需字符数（字面量格式串） */
	template <size_t N, typename... Args>
	size_t formatted_size(const char (&fmt_str)[N], const Args &...args)
	{
		return etl_fmt::formatted_size(etl::string_view(fmt_str, N - 1), args...);
	}
} // namespace etl_fmt

// #################################################################################################
// #                                                                                               #
// #   自定义格式化器迁移：广义 pair 的 `etl::formatter<T>` 特化                                    #
// #                                                                                               #
// #   为什么必须写在**全局** `etl` 命名空间里：                                                     #
// #     - ETL 后端：ETL 的格式化核心只通过 `etl::formatter<T>` 识别自定义类型；                      #
// #     - 若写在 `EMBMartin` 内部，`etl` 会被 `EMBMartin` 作用域内的名字遮挡，特化落到错误的命名空间。#
// #                                                                                               #
// #   旧后端的同名自定义点仍然是 `legacy_fmt::formatter<T>`（旧签名），由上面的旧核心提供，不受影响。  #
// #                                                                                               #
// #   ETL 的 `vformat_to` 流程是「先由 ETL 的 parse_format_spec 吃掉认识的标准片段，再把剩余部分交给   #
// #   自定义 formatter 的 parse」。本特化因此：                                                      #
// #     1. parse：把 ETL 吃剩的文本当成广义 pair 说明符解释，并整体消费到收尾 '}'；                   #
// #     2. 每个元素的子说明符复用 ETL 自己的 parse_format_spec，不重写任何说明符解析；                #
// #     3. format：分别调用元素类型自己的 `etl::formatter<Tx/Ty>`，把结果拼成 `[x, y]`。              #
// #                                                                                               #
// #   @note 语法差异（ETL 后端）：`<...>` 括号形式会与 ETL 的对齐符 `<` 冲突，故 ETL 后端下只能用     #
// #         `(...)` 与 `[...]` 两种括号；无括号形式的 `{::#X;:#b}` 也不可用（':' 已被 ETL 消费）。     #
// #         需要完整旧语法时请用 `EMBMARTIN_FMT_USE_ETL=0`，旧后端的 pair 语法与旧实现完全一致。      #
// #                                                                                               #
// #################################################################################################

EMBMARTIN_NAMESPACE_END

namespace etl
{
	/**
	 * @brief 广义 pair 的 ETL 自定义格式化器
	 *
	 * @note 关于特化的写法：`etl::formatter` 的第二个模板参数是**字符类型**（默认 `char`），
	 *       不是 `std::enable_if` 的落点。实测（armclang 6.24）：
	 *         - `formatter<T, char>`            → 能匹配，但无法约束；
	 *         - `formatter<T, enable_if_t<...>>`→ 落点是 `void`，**永远匹配不上**（ETL 只用 char）；
	 *         - `formatter<T, conditional_t<cond, char, 不完整类型>>` → 既能匹配又能约束。
	 *       故这里用第三种：条件成立时槽位是 `char`，否则替换失败（SFINAE）。
	 */
	template <typename _Pair>
	struct formatter<_Pair,
					 etl::conditional_t<::EMBMartin::pair_spec::is_decomposable_pair<_Pair>::value,
										char,
										::EMBMartin::pair_spec::not_a_pair>>
	{
		using _Tx = ::EMBMartin::generalized_pair_element_t<0, _Pair>;
		using _Ty = ::EMBMartin::generalized_pair_element_t<1, _Pair>;

		using _Spec = etl::private_format::format_spec_t;

		::EMBMartin::pair_spec::parts _parts{};
		_Spec _spec_x{};
		_Spec _spec_y{};
		bool _has_element_spec = false;

		etl::format_parse_context::iterator parse(etl::format_parse_context &parse_ctx)
		{
			// 本替换字段的收尾 '}'（广义 pair 说明符里不会出现嵌套的 {} ）
			auto begin = parse_ctx.begin();
			auto end = parse_ctx.end();
			auto close = begin;
			while (close != end && *close != '}')
			{
				++close;
			}

			// 切分广义 pair 说明符
			::EMBMartin::pair_spec::parts parts;
			::EMBMartin::pair_spec::split(etl::string_view(begin, static_cast<size_t>(close - begin)), parts);
			_parts = parts;
			_has_element_spec = ::EMBMartin::pair_spec::has_element_spec(parts);

			// 元素子说明符：复用 ETL 的说明符解析器 + 元素自己的 formatter
			if (_has_element_spec)
			{
				_parse_element(parts.x, _spec_x, _formatter_x);
				_parse_element(parts.y, _spec_y, _formatter_y);
			}
			// 没有元素级说明符时不需要 _spec_x/_spec_y：format 里直接用外层说明符

			return close;
		}

		template <typename OutputIt>
		typename etl::format_context<OutputIt>::iterator
		format(const _Pair &value, etl::format_context<OutputIt> &fmt_ctx)
		{
			const auto &[x, y] = value;
			const _Spec outer = fmt_ctx.format_spec;
			OutputIt out = fmt_ctx.out();

			if (_parts.prefix != '\0')
			{
				*out = _parts.prefix;
				++out;
				fmt_ctx.advance_to(out);
			}

			fmt_ctx.format_spec = _has_element_spec ? _spec_x : outer;
			out = _formatter_x.format(x, fmt_ctx);
			fmt_ctx.advance_to(out);

			*out = _parts.sep;
			++out;
			*out = ' ';
			++out;
			fmt_ctx.advance_to(out);

			fmt_ctx.format_spec = _has_element_spec ? _spec_y : outer;
			out = _formatter_y.format(y, fmt_ctx);
			fmt_ctx.advance_to(out);

			if (_parts.suffix != '\0')
			{
				*out = _parts.suffix;
				++out;
				fmt_ctx.advance_to(out);
			}

			fmt_ctx.format_spec = outer;
			return out;
		}

	private:
		etl::formatter<_Tx> _formatter_x{};
		etl::formatter<_Ty> _formatter_y{};

		/**
		 * @brief 解析一个元素子说明符
		 *
		 * 完全照搬 ETL 的顺序：先用 ETL 自己的 `parse_format_spec` 吃掉标准片段，
		 * 再把剩余部分交给元素自己的 formatter（内置 formatter 的 parse 是空操作）。
		 */
		template <typename Formatter>
		static void _parse_element(etl::string_view spec, _Spec &out, Formatter &element_formatter)
		{
			etl::format_parse_context sub_ctx(spec, 1);
			etl::private_format::parse_format_spec(sub_ctx, out);
			element_formatter.parse(sub_ctx);
		}
	};

	/**
	 * @brief @ref EMBMartin::fixed "fixed<N>" 的 ETL 自定义格式化器
	 *
	 * ETL 已经把它认识的说明符片段解析进 `fmt_ctx.format_spec`（fill / align / sign / `0` /
	 * width / precision / type），这里只把这些字段翻译成 @ref EMBMartin::fixed_detail::spec，
	 * 渲染交给与旧后端共用的整数运算实现——所以 ETL 那个「浮点 precision 被忽略」的缺陷
	 * 不会影响 `fixed<N>`。
	 */
	template <size_t N>
	struct formatter<::EMBMartin::fixed<N>, char>
	{
		etl::format_parse_context::iterator parse(etl::format_parse_context &parse_ctx)
		{
			// fixed<N> 没有自己的额外说明符语法，把 ETL 吃剩的部分整体消费到收尾 '}' 即可。
			// 若用户给了 e / g / a 之类定点以外的表示类型，统一在 format 里断言拒绝。
			auto it = parse_ctx.begin();
			while (it != parse_ctx.end() && *it != '}')
			{
				++it;
			}
			return it;
		}

		template <typename OutputIt>
		typename format_context<OutputIt>::iterator format(const ::EMBMartin::fixed<N> &value,
														   format_context<OutputIt> &fmt_ctx)
		{
			using spec_t = etl::private_format::format_spec_t;

			const spec_t &fs = fmt_ctx.format_spec;

			if (fs.type.has_value() && fs.type.value() != 'f' && fs.type.value() != 'F')
			{
				// 只支持 f / F
				EMBMARTIN_FMT_ASSERT(fs.type.value() == 'f' || fs.type.value() == 'F');
			}

			::EMBMartin::fixed_detail::spec s;
			s.fill = fs.fill;
			s.zero_fill = fs.zero;
			s.upper = fs.type.has_value() && fs.type.value() == 'F';
			s.precision = fs.precision.has_value() ? static_cast<int>(fs.precision.value()) : -1;
			s.width = fs.width.has_value() ? static_cast<int>(fs.width.value()) : 0;

			switch (fs.align)
			{
			case etl::private_format::spec_align_t::START:
				s.align = '<';
				break;
			case etl::private_format::spec_align_t::CENTER:
				s.align = '^';
				break;
			default: // END / NONE：数字默认右对齐
				s.align = '>';
				break;
			}
			switch (fs.sign)
			{
			case etl::private_format::spec_sign_t::PLUS:
				s.sign = '+';
				break;
			case etl::private_format::spec_sign_t::SPACE:
				s.sign = ' ';
				break;
			default:
				s.sign = '-';
				break;
			}

			char buf[::EMBMartin::fixed_detail::buffer_size];
			const size_t written = ::EMBMartin::fixed_detail::render(buf, value.value, static_cast<unsigned>(N), s);

			OutputIt out = fmt_ctx.out();
			for (size_t i = 0; i < written; ++i)
			{
				*out = buf[i];
				++out;
			}
			fmt_ctx.advance_to(out);
			return out;
		}
	};

	namespace private_format
	{
		/**
		 * @brief 告诉 ETL：`fixed<N>` 只会以 `basic_format_arg::handle` 的形式出现
		 *
		 * ETL 的 `arg_type_mask` 主模板对「不认识的类型」保守地返回 `mask_all`，于是
		 * `format_visitor<OutputIt, mask_all>` 会把**所有**内置分支的 formatter 都实例化出来
		 * （含整条浮点链），代码体积会凭空涨十几 KB。自定义类型实际只走 `handle` 那条
		 * 非模板重载，与内置分支的掩码无关，因此这里把它的掩码收窄到 `mask_monostate`
		 *（掩码递归的下界，等价于「没有内置分支需要实例化」）。
		 *
		 * 这是 ETL 内部类型，但对**我们自己的类型**做特化是安全的：只影响本类型的调用。
		 */
		template <size_t N>
		struct arg_type_mask<::EMBMartin::fixed<N>, void>
		{
			static constexpr arg_mask_t value = mask_monostate;
		};
	} // namespace private_format
} // namespace etl

// 注：EMBMARTIN_FMT_INLINE_ETL / EMBMARTIN_FMT_INLINE_LEGACY 这两个内部宏**故意不 #undef**，
// 因为 fmt.cpp 需要用同样的内联性重新打开对应的命名空间
//（否则 clang 会报 -Winline-namespace-reopened-noninline）。

#endif // EMBMARTIN_FORMAT_STRING_H
