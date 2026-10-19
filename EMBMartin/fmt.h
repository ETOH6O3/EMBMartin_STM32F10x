/**
 * @file    fmt.h
 * @author  孙鸣淼
 * @brief   EMBMartin 格式化字符串组件（旧核心 / ETL 双后端）
 *
 * @details
 * 本组件提供一套 ETL 风格的格式化接口，底层有**两套可选实现**，由预定义宏
 * @c EMBMARTIN_FMT_USE_ETL 选择；两套后端都编译进同一份头文件，只是内联命名空间不同。
 *
 * @par 环境要求
 * - 语言标准：不低于 C++17；
 * - 编译器：ARM Compiler 6 及以上，或 MSVC 19.x 及以上，或 GCC / Clang 支持 C++17 的版本。
 *
 * @warning 本组件的程序存储器开销较大，请谨慎使用。用于嵌入式系统时建议把编译器优化
 *          调整到 @c -O3 或 @c -Oz；实测 @c -O0 下的代码体积可达 @c -Oz 的数倍。
 * @warning 本组件对浮点数的处理以「结果正确」为优先，性能甚至远不及 @c printf，
 *          请谨慎用于对实时性敏感的热点路径。
 *
 * @section fmt_backend_select 双后端与命名空间设计
 *
 * 两套底层实现由 @c EMBMARTIN_FMT_USE_ETL 选择哪一个是**内联命名空间**：
 *
 * | @c EMBMARTIN_FMT_USE_ETL | 内联命名空间 | 底层实现 |
 * | :--- | :--- | :--- |
 * | @c 1（默认） | @c EMBMartin::etl_fmt | 直转发第三方库 ETL 的 @c format |
 * | @c 0 | @c EMBMartin::legacy_fmt | 本库既有旧格式化核心（原样保留）+ ETL 风格接口适配 |
 *
 * 命名空间分层（@c embmartin 即 @c EMBMartin）：
 * @verbatim
   EMBMartin
   ├── etl_fmt        ETL 直转发后端；宏为 1 时它是内联命名空间
   ├── legacy_fmt     旧格式化核心 + ETL 风格接口适配；宏为 0 时它是内联命名空间
   └── pair_spec      两套后端共用的「广义 pair 说明符」切分工具
   @endverbatim
 *
 * @note @c EMBMARTIN_FMT_USE_ETL 只决定哪一个命名空间内联，**不取消**另一个的定义：
 *       两套实现始终都被编译，因此始终可以写出 @c EMBMartin::legacy_fmt::... 或
 *       @c EMBMartin::etl_fmt::... 来显式指定后端。任何时刻都恰好只有一个内联命名空间，
 *       故 @c EMBMartin::format_to(...) 这类写法不会产生二义。
 *
 * @note 两套后端对外提供**同一套 ETL 风格接口**，调用方无需用宏切换写法：
 *       @c format_to / @c format_to_n / @c vformat_to / @c formatted_size ，
 *       外加 ETL 类型别名 @c formatter / @c format_context / @c format_parse_context /
 *       @c format_string / @c format_args / @c basic_format_arg / @c make_format_args 。
 *
 * @section fmt_error_handling 错误处理
 *
 * 与 ETL 一致的**风格**：非法格式串、无法满足的格式说明符**不返回错误码**，而是断言。
 * 两套后端共用一个上报点 则 @c EMBMARTIN_FMT_ASSERT（默认 @c assert），
 * 因此对外语义完全相同。
 *
 * @note 为什么默认是 @c assert 而不是直接用 ETL 的 @c ETL_ASSERT ？
 *       本工程使用 @c TPL/my_etl_profile.h ，其中定义了 @c ETL_NO_EXCEPTIONS 且没有定义
 *       @c ETL_DEBUG / @c ETL_LOG_ERRORS / @c ETL_USE_ASSERT_FUNCTION ，于是 ETL 的
 *       @c ETL_ASSERT(b,e) 在 @c error_handler.h 里落到 @c static_cast<void>(sizeof(b)) ，
 *       ——**整条检查会被编译掉**。若把本组件的断言也改成 @c ETL_ASSERT ，两套后端在发布
 *       构建里都会静默放过非法格式串。为保持「新旧后端行为一致且可控」，这里默认用
 *       @c assert ；想改成走 ETL 的错误处理器，预定义
 *       @code
 *       EMBMARTIN_FMT_ASSERT(_EXPR) \
 *           ETL_ASSERT(_EXPR, ETL_ERROR(etl::bad_format_string_exception))
 *       @endcode
 *       即可。
 *
 * @warning @c assert 在定义了 @c NDEBUG 时同样会被移除。需要运行时保护请覆盖
 *          @c EMBMARTIN_FMT_ASSERT （例如改走自定义错误处理器），或使用旧接口的
 *          @ref EMBMartin::legacy_fmt::FormatError 错误码（仅 @c legacy_fmt 提供）。
 *
 * @section fmt_legacy_compat 旧接口兼容层
 *
 * @c legacy_fmt 内部**原样保留**旧实现及其旧接口，供仍需 @c FormatError 错误码的代码使用：
 * - @ref EMBMartin::legacy_fmt::FormatError
 * - @ref EMBMartin::legacy_fmt::FormatContext
 * - @ref EMBMartin::legacy_fmt::FormatParseContext
 * - @ref EMBMartin::legacy_fmt::FormatString
 * - @ref EMBMartin::legacy_fmt::FormatLex
 * - @ref EMBMartin::legacy_fmt::formatter （旧签名的自定义点，见下）
 * - 旧的 @ref EMBMartin::legacy_fmt::format_to "int format_to(char*, size_t, const FormatString<N>&, const Args&...)"
 *
 * 其中旧签名自定义点要求实现：
 * - @c int @c parse(FormatParseContext@c &)  —— 返回 0 表示成功，负数表示错误码；
 * - @c int @c format(const@c T@c &, FormatContext@c &)  —— 同上。
 *
 * @note 这不是新接口，不随 @c EMBMARTIN_FMT_USE_ETL 变化；使用它必须写
 *       @c EMBMartin::legacy_fmt:: 限定名。
 *
 * @section fmt_syntax_diff 语法差异
 *
 * ETL 后端完全遵循 ETL / C++20 @c std::format 语法：对齐只有 @c < @c > @c ^
 * （**没有** @c = ），支持 @c + @c - @c 空格 @c # @c 0 @c L 、@c {} 形式的嵌套动态宽度
 * 与精度、@c {0} 手动参数索引，表示类型为
 * @c s @c ? @c b @c B @c c @c d @c o @c x @c X @c a @c A @c e @c E @c f @c F @c g @c G
 * @c p @c P ；**不支持** @c z 、数值分组（@c , 与 @c _ ）、@c % 。
 * 旧后端的旧语法（@c z 、@c = 对齐、数值分组、@c % 等）原样保留。
 *
 * 两套后端都可用的公共子集（仓库内调用点只使用这个子集）：
 * @verbatim
   {}  {:d}  {:x}  {:X}  {:#x}  {:b}  {:o}  {:c}  {:s}
   {:N}  {:>N}  {:<N}  {:^N}  {:*^N}  {:0N}  {:+}  {: }
   @endverbatim
 *
 * 只有一侧支持的语法（用错后端会断言，不会静默出错）：
 * - **仅 ETL 后端**：@c {:?} 、@c {:p} 、@c B 与 @c A 等表示类型、@c {0} 手动索引、
 *   @c {} 形式的嵌套宽度与精度、字符串 precision、指针形参。
 * - **仅旧后端**：@c z 、@c = 对齐、数值分组（逗号与下划线）、@c % 、
 *   以及 pair 的 @c {::x规格;y规格} 与尖括号形式元素级说明符。
 *
 * @warning ETL 的说明符解析器遇到自己不认识的字符（如旧语法的 @c , 与 @c _ ）不会再报错，
 *          而是继续推进解析指针，导致**越界读取**。在 @c ETL_ASSERT 被编译掉的本工程
 *          profile 下，这种输入会崩溃或产生乱码，而不是干净地断言。
 *          详见 @ref fmt_error_handling 一节。
 *
 * @section fmt_fixed_precision 浮点小数位（重要）
 *
 * **ETL 后端无法控制浮点的小数位数**：ETL 20.49.0 的三个浮点格式化函数
 * （@c format_floating_default / @c format_floating_f / @c format_floating_e ）都把
 * 小数位数硬编码为 6，从不读取 @c spec.precision ；@c {:.2f} 、@c {:.0f} 、@c {:.4f}
 * 的输出因此完全相同。这是 ETL 上游**至今未实现**的功能（已核对 master 分支，代码里
 * 仍写着 @c "const@c size_t@c fractional_decimals@c =@c 6;@c //@c default" ），不是配置问题。
 *
 * 需要固定小数位时请使用 @ref EMBMartin::fixed "EMBMartin::fixed<N>" ：
 * @code
 * oled.println("Dist: {}m", EMBMartin::fixed<2>{distance});   // "Dist: 1.23m"
 * @endcode
 *
 * @c fixed<N> 用整数运算渲染，**两套后端输出逐字符一致**，也不受旧核心 @c long@c double
 * 舍入差异影响；说明符里的 @c .precision 会覆盖 @c N 。详见该类型的文档。
 *
 * @section fmt_known_limits 已知限制
 *
 * - 旧后端没有「只计数」通道，适配层用 @c EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE
 *   （默认 512）字节的栈缓冲承接一次完整结果；单次格式化结果超过该大小时断言失败。
 *   可调大该宏。
 * - 旧后端不支持 @c {0} 手动索引与 @c {} 嵌套动态宽度／精度（旧核心本身不支持）。
 * - ETL 20.49.0 的浮点格式化忽略 @c precision （见上一节）。不要在 ETL 后端依赖
 *   @c {:.Nf} ；要固定小数位就用 @ref EMBMartin::fixed 。
 * - 旧核心的浮点舍入建立在 @c long@c double 之上。当被丢弃的部分**恰好以 5 开头**
 *   （平局）时，结果取决于 @c long@c double 的有效位数：MinGW/x86-64 为 80 位
 *   （进位的概率更高），MSVC 与 armclang(ARM) 为 64 位。例如旧后端 @c {:.3e} 对
 *   @c 1234.5f 可能是 @c 1.235e+003 也可能是 @c 1.234e+003 。这是被原样保留的旧实现
 *   自身的性质，与本次迁移无关；@c fixed<N> 与 ETL 后端的其它路径不受影响。
 *
 * @see EMBMartin::fixed
 * @see EMBMartin::pair_spec
 * @see EMBMARTIN_FMT_ASSERT
 * @see FMT.md —— 面向使用者的接口文档
 */
#pragma once

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
 * @def EMBMARTIN_FMT_ASSERT
 * @brief 新版（ETL 风格）接口的错误上报宏
 *
 * @details 与 ETL 的 @c ETL_ASSERT 同构：非法格式串、无法满足的格式说明符直接断言。
 * 两套后端都用这一个宏，故对外错误语义完全一致。
 *
 * @param _EXPR 断言表达式；为假时触发断言
 *
 * @note 默认用 @c assert 而非 @c ETL_ASSERT ：本工程的 ETL profile 让 @c ETL_ASSERT
 *       变成空操作（详见 @ref fmt_error_handling 一节）。想接入 ETL 的错误处理器，
 *       预定义
 *       @code
 *       EMBMARTIN_FMT_ASSERT(_EXPR) \
 *           ETL_ASSERT(_EXPR, ETL_ERROR(etl::bad_format_string_exception))
 *       @endcode
 */
#ifndef EMBMARTIN_FMT_ASSERT
#define EMBMARTIN_FMT_ASSERT(_EXPR) assert(_EXPR)
#endif // EMBMARTIN_FMT_ASSERT

/**
 * @def EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE
 * @brief 旧后端适配层承接结果的栈缓冲区大小（字节）
 *
 * @details 旧核心只能写入一整块连续缓冲区，且没有「只计数」模式，适配层因此需要一块栈缓冲。
 * 单次格式化结果超过此大小时断言失败（而不是静默给出错误结果）。
 *
 * @warning 增大该值会等量增加**调用栈**占用；在栈空间紧张的 MCU 上请谨慎调整。
 */
#ifndef EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE
#define EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE 512
#endif // EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE

// ------------------------------------------内联命名空间选择--------------------------------------------------

/**
 * @def EMBMARTIN_FMT_INLINE_ETL
 * @brief 把 @c EMBMARTIN_FMT_USE_ETL 映射成 @c etl_fmt 命名空间定义上的 @c inline 限定符
 *
 * @details 用宏而不是在命名空间定义之后写 @c "inline@c namespace@c xxx;" ，是为了让两套后端
 * 各自只被定义一次（避免为了实现「只有一个内联」而把两份实现正文重复两遍）。
 *
 * 取值：@c EMBMARTIN_FMT_USE_ETL 为真时为 @c inline ，否则为空。
 *
 * @note 本宏与 @c EMBMARTIN_FMT_INLINE_LEGACY **不**在文件末尾 @c \#undef ，
 *       因为 @c fmt.cpp 需要用同样的内联性重新打开对应命名空间。
 */

/**
 * @def EMBMARTIN_FMT_INLINE_LEGACY
 * @brief 把 @c EMBMARTIN_FMT_USE_ETL 映射成 @c legacy_fmt 命名空间定义上的 @c inline 限定符
 *
 * @details 与 @c EMBMARTIN_FMT_INLINE_ETL 互补，二者恰有一个展开为 @c inline 。
 *
 * 取值：@c EMBMARTIN_FMT_USE_ETL 为假时为 @c inline ，否则为空。
 *
 * @warning 两个宏的展开结果互斥；若同时为空或同时为 @c inline ，将分别导致「没有内联
 *          命名空间」或「两个内联命名空间」的语义错误。
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

// #################################################################################################
// #                                                                                               #
// #   后端一：legacy_fmt —— 旧格式化核心（原样保留） + ETL 风格接口适配                            #
// #                                                                                               #
// #################################################################################################

EMBMARTIN_FMT_INLINE_LEGACY namespace legacy_fmt
{

// ------------------------------------------前向声明--------------------------------------------------

/**
 * @brief 旧后端的自定义格式化器主模板
 *
 * @details 该主模板**刻意留空**：只有当用户为某个类型提供了 @c formatter<T> 的合法特化
 * （同时具备可用的 @c parse 与 @c format 成员）时，该类型才被认为可格式化。
 * 这一设计与 ETL 的 @c etl::formatter 一致。
 *
 * @tparam T     被格式化的类型
 * @tparam _Sfinae 预留的 SFINAE 槽位，供各特化用于条件约束
 *
 * @see is_formattable
 */
template <typename T, typename>
struct formatter;

// ------------------------------------------错误码定义--------------------------------------------------

/**
 * @brief 旧核心的错误码
 *
 * @details 旧接口（@c format_to(char*,@c size_t,@c const@c FormatString<N>&,@c const@c Args&...) 等）
 * 通过返回该枚举的负值来报告失败。新版 ETL 风格接口不使用它，而是走
 * @c EMBMARTIN_FMT_ASSERT 断言（详见 @ref fmt_error_handling 一节）。
 */
enum class FormatError
{
	Success = 0,			 ///< 成功
	BufferOverflow = -1,	 ///< 输出缓冲区容量不足（或写入了内嵌的 @c '\\0' ）
	InvalidFormatSpec = -2,	 ///< 格式说明符非法，或与实参类型不匹配
	TypeNotFormattable = -3, ///< 该类型没有可用的 @c formatter 特化
	EmptyFormatString = -4,	 ///< 格式串为空
	MismatchedBraces = -5,	 ///< 花括号不配对
	TooMuchContext = -6		 ///< 说明符后仍有多余字符（说明符过多）
};

// ------------------------------------------上下文管理--------------------------------------------------

/**
 * @brief 旧核心的轻量级格式上下文
 *
 * @details 持有「输出缓冲区 + 容量 + 当前写入位置」三元组，并提供带边界检查的写入操作。
 * 所有写入都**不会**越界；容量不足时返回 @c false ，由调用方转换成相应的
 * @ref FormatError 错误码。
 *
 * @note 本类的所有写操作都不追加结尾 @c '\\0' ；需要 C 字符串语义时由调用方自行补。
 * @warning 构造后 @p buffer 的所有权仍在调用方，本类不负责释放。
 */
class FormatContext
{
	char *buffer_;	  ///< 输出缓冲区起始地址
	size_t capacity_; ///< 缓冲区容量（字节）
	size_t position_; ///< 已写入的字节数

public:
	/**
	 * @brief 构造一个格式上下文
	 *
	 * @param buffer   输出缓冲区，至少 @p capacity 字节
	 * @param capacity 缓冲区容量（字节）
	 */
	FormatContext(char *buffer, size_t capacity)
		: buffer_(buffer), capacity_(capacity), position_(0)
	{
	}

	/**
	 * @brief 写入一段字符串视图（带溢出保护）
	 *
	 * @param str 待写入内容
	 *
	 * @return 是否写入成功
	 * @retval true  全部写入
	 * @retval false 剩余空间不足，**一个字节都没有写入**
	 */
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

	/**
	 * @brief 写入单个字符（带溢出保护）
	 *
	 * @param ch 待写入字符
	 *
	 * @return 是否写入成功
	 * @retval true  写入成功
	 * @retval false 已无剩余空间
	 *
	 * @note 若 @p ch 为 @c '\\0' ，则把它写到当前位置但**不推进** @ref position ，
	 *       用在需要就地补终止符的旧路径上。
	 */
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

	/**
	 * @brief 获取已写入内容的视图
	 *
	 * @return 指向已写入区域的 @c etl::string_view ，长度为 @ref position
	 */
	etl::string_view view() const
	{
		return etl::string_view(buffer_, position_);
	}

	/**
	 * @brief 获取当前写入位置
	 *
	 * @return 已写入的字节数
	 */
	size_t position() const { return position_; }

	/**
	 * @brief 设置当前写入位置（用于回退等操作）
	 *
	 * @param pos 新的写入位置；调用方需保证不超过 @ref position
	 */
	void set_position(size_t pos)
	{
		position_ = pos;
	}

	/**
	 * @brief 获取剩余可写空间
	 *
	 * @return @c capacity - position ，即还能写入的字节数
	 */
	size_t remaining() const { return capacity_ - position_; }
};

// ------------------------------------------解析上下文--------------------------------------------------

/**
 * @brief 旧核心的轻量级解析上下文
 *
 * @details 在一段说明符文本上维护读写位置，供 @ref FormatLex 及各 formatter 的 @c parse
 * 顺序读取字符。所有读取操作在越界时都安全（返回 @c '\\0' 或等价于「已结束」）。
 */
class FormatParseContext
{
	etl::string_view spec_; ///< 说明符原文
	size_t pos_;			///< 当前解析位置

public:
	/**
	 * @brief 构造一个解析上下文
	 *
	 * @param spec 说明符原文（不含前后花括号）
	 */
	FormatParseContext(const etl::string_view &spec)
		: spec_(spec), pos_(0)
	{
	}

	/**
	 * @brief 在说明符原文中查找子串（转发到 @c etl::string_view::find ）
	 *
	 * @tparam Args 转发给 @c find 的实参类型
	 * @param args  转发给 @c find 的实参
	 *
	 * @return @c find 的返回值（找到时为首个下标，否则为 @c npos ）
	 */
	template <typename... Args>
	auto find(Args... args) const
	{
		return spec_.find(args...);
	}

	/** @brief 获取指向**当前位置**的迭代器（可写重载） @return 起始迭代器 */
	auto begin() { return spec_.begin() + pos_; }
	/** @brief 获取指向末尾的迭代器（可写重载） @return 末尾迭代器 */
	auto end() { return spec_.end(); }
	/** @brief 获取指向**当前位置**的迭代器（只读重载） @return 起始迭代器 */
	auto begin() const { return spec_.begin() + pos_; }
	/** @brief 获取指向末尾的迭代器（只读重载） @return 末尾迭代器 */
	auto end() const { return spec_.end(); }

	/**
	 * @brief 查看下一个字符但不前进
	 *
	 * @return 当前位置的字符；已到末尾时返回 @c '\\0'
	 */
	char peek() const
	{
		return pos_ < spec_.size() ? spec_[pos_] : '\0';
	}

	/**
	 * @brief 取出当前字符并前进一位
	 *
	 * @return 取出的字符；已到末尾时返回 @c '\\0' 且位置不变
	 */
	char get()
	{
		return pos_ < spec_.size() ? spec_[pos_++] : '\0';
	}

	/**
	 * @brief 消耗一个字符但不返回其值
	 *
	 * @note 已到末尾时调用本函数是空操作。
	 */
	void consume()
	{
		if (pos_ < spec_.size())
			pos_++;
	}

	/**
	 * @brief 获取尚未解析的剩余说明符
	 *
	 * @return 从当前位置到末尾的视图
	 */
	etl::string_view remainder() const
	{
		return spec_.substr(pos_);
	}

	/**
	 * @brief 判断是否已解析完
	 *
	 * @return 是否已到末尾
	 * @retval true  位置已不小于长度
	 * @retval false 仍有待解析内容
	 */
	bool empty() const { return pos_ >= spec_.size(); }

	/**
	 * @brief 获取当前解析位置
	 *
	 * @return 已消费的字符数
	 */
	size_t position() const { return pos_; }
};

// ------------------------------------------格式字符串验证--------------------------------------------------

/**
 * @brief 编译期持有字符串字面量格式串的包装类型
 *
 * @details 把 @c const@c char[N] 字面量绑定为引用并保留其长度信息，从而让旧接口
 * @c format_to(char*,@c size_t,@c const@c FormatString<N>&,@c const@c Args&...) 能够拿到
 * **不含结尾 @c '\\0' ** 的完整视图。
 *
 * @tparam N 字面量数组的总长度（**必须**等于字符串长度加 1）
 *
 * @warning 模板参数 @c N 必须与传入字面量的实际数组长度一致。例如 @c "x={}" 长度为 4，
 *          加结尾 @c '\\0' 后数组长度为 5，必须写成 @c FormatString<5> 。
 *          @c N 写错会得到截断或越界的视图。
 */
template <size_t N>
class FormatString
{
	const char (&str_)[N]; ///< 绑定的字面量数组

public:
	/**
	 * @brief 从字符串字面量构造
	 *
	 * @tparam M 传入字面量的数组长度
	 * @param s  字符串字面量
	 */
	template <size_t M>
	constexpr FormatString(const char (&s)[M]) : str_(s)
	{
		static_assert(M > 0, "Format string cannot be empty");
		// static_assert(validate_basic_structure(s),
		//              "Invalid format string structure");
		// TODO: 改进
	}

	/**
	 * @brief 获取原始格式串的视图（不含结尾 @c '\\0' ）
	 *
	 * @return 长度为 @c N-1 的视图
	 */
	constexpr etl::string_view view() const
	{
		return etl::string_view(str_, N - 1);
	}

	/**
	 * @brief 获取指向原始格式串的 C 字符串指针
	 *
	 * @return 绑定的字面量起始地址
	 */
	constexpr const char *c_str() const { return str_; }

private:
	/**
	 * @brief 检查花括号是否配对（当前未启用）
	 *
	 * @param s 待检查的 C 字符串
	 *
	 * @return 花括号是否配对
	 * @retval true  配对正确
	 * @retval false 存在多余或缺失的花括号
	 *
	 * @note 该检查目前并未接入构造流程（见构造函数中被注释掉的 @c static_assert ），
	 *       保留以备后续启用。
	 */
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

/**
 * @brief 旧后端的自定义格式化器主模板
 *
 * @details 主模板**刻意留空**，因此某类型只有在提供了合法特化（同时具备可用签名的
 * @c parse 与 @c format 成员）时才可被格式化。
 *
 * 一个合法的旧签名特化须提供：
 * - @c int @c parse(FormatParseContext@c &) —— 返回 0 表示成功，负数为 @ref FormatError ；
 * - @c int @c format(const@c T@c &, FormatContext@c &)@c const —— 同上。
 *
 * @tparam T    被格式化的类型
 * @tparam _Sf  SFINAE 约束槽位，供各特化通过 @c std::enable_if_t 使用
 *
 * @see is_formattable
 * @see formatter<T, std::enable_if_t<std::is_arithmetic_v<T>>> （算术类型特化）
 * @see formatter<T, std::enable_if_t<is_string_like_v<T> || is_character_array_v<T>>> （字符串特化）
 * @see formatter<_Pair, std::enable_if_t<is_generalized_pair_v<_Pair>>> （广义 pair 特化）
 */
template <typename T, typename = void>
struct formatter
{
};

/**
 * @brief 探测某类型在本后端下是否可格式化
 *
 * @details 当且仅当 @c formatter<T> 同时具备可用签名的 @c parse 与 @c format 成员时，
 * 本类型才继承自 @c std::true_type 。
 *
 * @tparam T     待探测的类型
 * @tparam _T1   内部 SFINAE 槽位，勿显式指定
 * @tparam _T2   内部 SFINAE 槽位，勿显式指定
 */
template <typename T, typename = void, typename = void>
struct is_formattable : std::false_type
{
};

/**
 * @brief @ref is_formattable 的特化：检测到可用的 @c parse 与 @c format 成员
 *
 * @tparam T 待探测的类型
 */
template <typename T>
struct is_formattable<
	T,
	std::void_t<decltype(std::declval<formatter<T>>().parse(std::declval<FormatParseContext &>()))>,
	std::void_t<decltype(std::declval<formatter<T>>().format(std::declval<T>(), std::declval<FormatContext &>()))>>
	: std::true_type
{
};

/**
 * @brief @c is_formattable 的变量模板形式
 *
 * @tparam T 待探测的类型
 */
template <typename T>
constexpr bool is_formattable_v = is_formattable<T>::value;

// ------------------------------------------基础类型特化声明--------------------------------------------------

/**
 * @brief 通用格式说明符的**词法解析器**（旧核心）
 *
 * @details 解析旧语法的迷你语言，把结果保存在本类的数据成员里，供各 @c formatter 的
 * @c format 阶段直接读取。说明符语法：
 * @verbatim
   format_spec:          [options][width_and_precision][type]
     options:            [[fill]align][sign]["z"]["#"]["0"]
       fill:             <any character>
       align:            "<" | ">" | "=" | "^"
       sign:             "+" | "-" | " "
     width_and_precision:[width_with_grouping][precision_with_grouping]
       width_with_grouping:     [width][grouping]
       precision_with_grouping: "." [precision][grouping] | "." grouping
         width:                  digit+
         precision:              digit+
         grouping:               "," | "_"
     type:               "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g"
                         | "G" | "o" | "s" | "x" | "X" | "%"
   @endverbatim
 *
 * @tparam T 被格式化的实参类型；用于推导 @c _type 的默认值
 *
 * @note 本类只属于旧后端；ETL 后端使用 ETL 自己的说明符解析器。
 * @warning 旧语法中的 @c z 、@c = 对齐、数值分组（@c , 与 @c _ ）、@c % 在 ETL 后端
 *          **不可用**，且在 @c ETL_ASSERT 被编译掉的配置下会导致越界读取，
 *          详见 @ref fmt_syntax_diff 。
 */
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
	/** @brief 对齐方式 */
	enum class Align : char
	{
		Left = '<',		///< 强制字段在可用空间内左对齐（大多数对象的默认值）
		Right = '>',	///< 强制字段在可用空间内右对齐（数字的默认值）
		Centered = '^', ///< 强制字段在可用空间内居中
		ZeroFill = '=', ///< 强制在符号（若有）之后、数字之前放置填充；仅适用于数字类型
		Invalid = char(0) ///< 不是合法的对齐字符
	};

	/** @brief 符号选项 */
	enum class Sign : char
	{
		All = '+',		  ///< 正数与负数都输出符号
		NegOnly = '-',	  ///< 仅负数输出符号（默认）
		SpaceOrSign = ' ', ///< 正数输出前导空格，负数输出负号
		Invalid = char(0)  ///< 不是合法的符号字符
	};

	/** @brief 数值分组分隔符 */
	enum class Grouping : char
	{
		Comma = ',',	  ///< 每隔若干数位插入逗号
		Underscore = '_', ///< 每隔若干数位插入下划线（十六进制等为每 4 位）
		None = '\0',	  ///< 不使用分隔符
		Invalid = char(0) ///< 不是合法的分组字符
	};

	/** @brief 表示类型 */
	enum class Type : char
	{
		// 可用的字符串表示类型
		String = 's', ///< 字符串格式；字符串的默认类型，可省略

		// 可用的整数表示类型
		Binary = 'b',	///< 二进制格式，输出以 2 为基数的数字
		Char = 'c',		///< 字符；打印前把整数转换为对应字符
		Dec = 'd',		///< 十进制整数，输出以 10 为基数的数字
		Octal = 'o',	///< 八进制格式，输出以 8 为基数的数字
		HexLower = 'x', ///< 十六进制格式，9 以上的数码用小写字母
		HexUpper = 'X', ///< 十六进制格式，9 以上的数码用大写字母

		// 浮点表示类型；整数可用，若使用则先转化为单精度浮点数
		eScientific = 'e', ///< 指数记数法，以小写 @c 'e' 引入指数部分
		EScientific = 'E', ///< 指数记数法，以大写 @c 'E' 引入指数部分
		fFixed = 'f',	   ///< 十进制浮点格式，小数点后恰好 @c prec 位
		FFixed = 'F',	   ///< 定点表示；与 @c 'f' 相似，但 nan/inf 输出为 NAN/INF
		gGeneral = 'g',	   ///< 常规格式，相当于 @c printf 的 @c %g
		GGeneral = 'G',	   ///< 常规格式，相当于 @c printf 的 @c %G
		Percentage = '%',  ///< 百分比；乘以 100 后按定点格式输出，并附加 @c '%'

		Invalid = char(0) ///< 不是合法的表示类型字符
	};

	/**
	 * @brief 把字符映射为 @ref Align
	 *
	 * @param c 待映射字符
	 * @return 对应的 @ref Align ；无法识别时返回 @ref Align::Invalid
	 */
	constexpr static Align align(char c) noexcept;

	/**
	 * @brief 把字符映射为 @ref Sign
	 *
	 * @param c 待映射字符，默认 @c '-'
	 * @return 对应的 @ref Sign ；无法识别时返回 @ref Sign::Invalid
	 */
	constexpr static Sign sign(char c = '-') noexcept;

	/**
	 * @brief 把字符映射为 @ref Grouping
	 *
	 * @param c 待映射字符，默认 @c '\\0'
	 * @return 对应的 @ref Grouping ；无法识别时返回 @ref Grouping::Invalid
	 */
	constexpr static Grouping grouping(char c = '\0') noexcept;

	/**
	 * @brief 把字符映射为 @ref Type
	 *
	 * @param c 待映射字符
	 * @return 对应的 @ref Type ；无法识别时返回 @ref Type::Invalid
	 */
	constexpr static Type type(char c) noexcept;

	/** @brief 未显式指定精度时使用的默认小数位数 */
	constexpr static int default_prec = 6;

	char _fill = ' ';			   ///< 填充字符，默认空格
	Align _align = Align::Invalid; ///< 对齐方式；字符类型究竟按字符还是整数输出需运行时决定，故不设默认参数
	Sign _sign = Sign::NegOnly;	   ///< 符号选项，默认仅负数显示符号

	bool _z = false;		 ///< 是否把 @c -0.0 强制转成 @c +0.0 （四舍五入后判断）
	bool _prefix = false;	 ///< 是否输出数值前缀（@c 0b / @c 0o / @c 0x ）；十进制任何时候都不使用前缀
	bool _zero_fill = false; ///< 是否启用「感知正负号」的零填充

	int _width = 0; ///< 最小字段宽度（含前缀、分隔符等全部字符）；0 表示不指定，由内容决定
	Grouping _int_part_grouping = Grouping::None; ///< 整数部分的分组分隔符

	/**
	 * @brief 小数位数
	 *
	 * @details 对表示类型 @c 'f' / @c 'F' 表示小数点后的位数；对 @c 'g' / @c 'G' 表示
	 * 有效位数；对字符串表示类型表示最大字段长度；整数表示类型不允许指定精度。
	 */
	int _prec = default_prec;

	Grouping _frac_part_grouping = Grouping::None; ///< 小数部分的分组分隔符

	/** @brief 表示类型；未指定时按实参类型推导默认值 */
	Type _type = is_string_like_v<T> ? Type::String
									 : (is_character_v<T> ? Type::Char
														  : (std::is_integral_v<T> ? Type::Dec
																				   : (std::is_floating_point_v<T> ? Type::gGeneral
																												  : Type::String)));

	/**
	 * @brief 解析格式说明符（运行时）
	 *
	 * @param ctx 解析上下文，位置应指向 @c ':' 或已为空
	 *
	 * @return 错误码，0 表示成功
	 * @retval 0                        解析成功
	 * @retval FormatError::InvalidFormatSpec 说明符不符合语法
	 * @retval FormatError::TooMuchContext    说明符后仍有多余字符
	 *
	 * @note 说明符必须以 @c ':' 起头（旧核心不支持手动参数索引）；@p ctx 为空时视为
	 *       「使用全部默认值」并返回 0。
	 */
	int parse(FormatParseContext &ctx);
};

/** @brief 见 @ref FormatLex::align 的声明处说明 */
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
/** @brief 见 @ref FormatLex::sign 的声明处说明 */
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

/** @brief 见 @ref FormatLex::grouping 的声明处说明 */
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

/** @brief 见 @ref FormatLex::type 的声明处说明 */
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

/** @brief 见 @ref FormatLex::parse 的声明处说明 */
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

/**
 * @brief 算术类型（整数 / 字符 / 浮点）的旧核心格式化器
 *
 * @details 说明符解析继承自 @ref FormatLex ；渲染时按 @c _type 分派到整数路径或浮点路径。
 * 整数路径不做浮点转换，浮点路径统一用 @c long@c double 计算。
 *
 * @tparam T 算术类型
 *
 * @note 若 @c _type 为 @c Type::String （字符串表示类型），格式化会失败并返回
 *       @ref FormatError::InvalidFormatSpec 。
 */
template <typename T>
struct formatter<T, std::enable_if_t<std::is_arithmetic_v<T>>> : public FormatLex<T>
{
	using BaseType = FormatLex<T>; ///< 说明符词法解析器的基类别名

	/**
	 * @brief 按已解析的说明符格式化一个算术值
	 *
	 * @param value 被格式化的值
	 * @param ctx   输出上下文
	 *
	 * @return 错误码，0 表示成功
	 * @retval FormatError::Success           成功
	 * @retval FormatError::InvalidFormatSpec 说明符与本类型不匹配（如给整数用了 @c 's' ）
	 * @retval FormatError::BufferOverflow    输出缓冲区不足
	 */
	int format(T value, FormatContext &ctx) const;
};

/**
 * @namespace EMBMartin::legacy_fmt::core_detail
 * @brief 旧核心的内部实现细节
 *
 * @details 包含浮点拆分与舍入的辅助函数、参数包递归展开、以及占位符计数工具。
 * 这些符号不属于对外接口，仅由本文件与 @c fmt.cpp 使用。
 *
 * @warning 本命名空间的内容可能随实现调整而变化，不应在业务代码中直接调用。
 */
namespace core_detail
{

/**
 * @brief 计算一个十进制数的数量级（以 10 为底的指数）
 *
 * @param value 正数
 *
 * @return 使 @c value@c /@c 10^e 落在 @c [1,@c 10) 内的指数 @c e
 *
 * @note 仅供浮点格式化路径内部使用。
 */
int decimal_exponent(long double value) noexcept;

/**
 * @brief 把数字字符数组按给定精度做四舍五入
 *
 * @param digits   数字字符数组（就地修改）
 * @param length   [in,out] 数字个数；舍入导致进位时可能增加
 * @param keep     保留的数字个数
 * @param guard    被丢弃部分的首位字符，用于判定是否进位
 */
void round_digits(char *digits, int &length, int keep, char guard) noexcept;

/**
 * @brief 把十进制数字串整体加一（用于舍入进位）
 *
 * @param digits 以 @c '\\0' 结尾的十进制数字串，就地加一
 */
void increment_decimal(char *digits) noexcept;

/**
 * @brief 把浮点值拆成「整数部分 / 小数部分 / 指数部分」三个字符串
 *
 * @param value     待拆分的非负有限值
 * @param type      表示类型字符（@c 'e' @c 'E' @c 'f' @c 'F' @c 'g' @c 'G' @c '%' ）
 * @param precision 小数位数或有效位数
 * @param integer   [out] 接收整数部分数字串
 * @param fraction  [out] 接收小数部分数字串
 * @param exponent  [out] 接收指数部分（含符号），不需要时为空串
 *
 * @warning 各出参缓冲区的大小要求见 @c fmt.cpp 中的实现说明，调用方需自行保证足够。
 */
void make_float_parts(long double value, char type,
					  int precision, char *integer, char *fraction, char *exponent) noexcept;

} // namespace core_detail

/**
 * @brief 算术类型 formatter 的 @c format 实现
 *
 * @note 本实现依赖若干 @c goto 跳转的生成阶段（前缀 / 符号 / 数字），
 *       逻辑顺序为「前缀 → 符号 → 数字与分组 → 总拼接」。
 */
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

/**
 * @brief 字符串类类型的旧核心格式化器
 *
 * @details 判定条件是「能否由 @c T 构造出 @c etl::string_view 」（即 @c T 为字符串类对象、
 * 字符指针或字符数组），而**不是**「能否由 @c etl::string_view 构造出 @c T 」。
 * 后者对 @c const@c char* 恒为 @c false ，会导致 @c const@c char* 匹配不到本特化，
 * 从而回退到空的主模板而无法格式化。
 *
 * @tparam T 字符串类类型、字符指针或字符数组
 *
 * @note 本特化只支持 @c 's' 表示类型与对齐／宽度；其它任何说明符字段（分组、精度、
 *       前缀、@c z 、非默认符号、零填充、@c = 对齐）都会导致格式化失败。
 */
template <typename T>
struct formatter<T, std::enable_if_t<is_string_like_v<T> || is_character_array_v<T>>>
	: public FormatLex<T>
{
	using BaseType = FormatLex<T>; ///< 说明符词法解析器的基类别名

	/**
	 * @brief 按已解析的说明符格式化一个字符串
	 *
	 * @param value 被格式化的字符串
	 * @param ctx   输出上下文
	 *
	 * @return 错误码，0 表示成功
	 * @retval FormatError::Success           成功
	 * @retval FormatError::InvalidFormatSpec 使用了字符串不支持的说明符字段
	 * @retval FormatError::BufferOverflow    输出缓冲区不足
	 */
	int format(const T &value, FormatContext &ctx) const;
};

/** @brief 见 @c formatter::format 的声明处说明 */
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

/**
 * @brief 广义 pair 的旧核心格式化器
 *
 * @details 支持为两个元素分别指定子说明符，输出形如 @c "x, y" 并可加括号包裹。
 * 说明符语法：
 * @verbatim
   format_spec:  [:[left bracket][x_format_spec][sep[y_format_spec]][right bracket]]
     left bracket:   (  [  <  或省略
     sep:            ",," 或 ';'；不给时 x 与 y 共用 x_format_spec
     right bracket:  )  ]  >  或省略（必须与左括号配对）
     x_format:       任意适用于 _Tx 的合法说明符
     y_format:       任意适用于 _Ty 的合法说明符
   @endverbatim
 *
 * 示例：
 * @code
 * console.println("coord: {::#X;:#b}", Coordinate<int>(0xff,0b101010));  // coord: 0XFF; 0b101010
 * console.println("coord: {:(:#X)}", Coordinate<int>(0xff,0b101010));    // coord: (0XFF, 0X2A)
 * console.println("coord: {:(:#X,,:#b)}", Coordinate<int>(0xff,0b101010)); // coord: (0XFF, 0b101010)
 * console.println("aggr:  {:[]}", add_result<int>{20,false});            // aggr:  [20, 0]
 * @endcode
 *
 * @tparam _Pair 可结构化绑定出两个成员的广义 pair 类型
 *
 * @see EMBMartin::pair_spec
 * @note ETL 后端下本语法受限：@c <...> 括号会与 ETL 的对齐符 @c < 冲突，
 *       无括号形式也不可用（@c ':' 已被 ETL 消费）。详见 @ref fmt_syntax_diff 。
 */
template <typename _Pair>
struct formatter<_Pair, std::enable_if_t<is_generalized_pair_v<_Pair>>>
{
	using value_type = _Pair;							   ///< 被格式化的类型
	using _Tx = generalized_pair_element_t<0, _Pair>;	   ///< 左元素类型
	using _Ty = generalized_pair_element_t<1, _Pair>;	   ///< 右元素类型

	/** @brief 左括号种类 */
	enum class Prefix : char
	{
		SquareBracketBegin = '[',	///< @c '['
		ParenthesisBegin = '(',		///< @c '('
		AngleBracketBegin = '<',	///< @c '<'
		None = '\0',				///< 无括号
		Invalid = '\0'				///< 不合法的括号
	};

	/** @brief 右括号种类 */
	enum class Suffix : char
	{
		SquareBracketClose = ']',	///< @c ']'
		ParenthesisClose = ')',		///< @c ')'
		AngleBracketClose = '>',	///< @c '>'
		None = '\0',				///< 无括号
		Invalid = '\0'				///< 不合法的括号
	};

	/** @brief 元素分隔符种类 */
	enum class Sep : char
	{
		Comma = ',',   ///< @c ','
		Semicolon = ';' ///< @c ';'
	};

	formatter<_Tx> _formatter_x;  ///< 左元素的格式化器
	formatter<_Ty> _formatter_y;  ///< 右元素的格式化器
	Prefix _prefix = Prefix::None; ///< 左括号
	Suffix _suffix = Suffix::None; ///< 右括号
	Sep _sep = Sep::Comma;		   ///< 元素分隔符

	/**
	 * @brief 把一对括号字符映射成 @ref Prefix 与 @ref Suffix
	 *
	 * @param pre 左括号字符
	 * @param suf 右括号字符
	 *
	 * @return 由 @ref Prefix 与 @ref Suffix 组成的 pair；不配对时两者均为 @c Invalid
	 */
	static constexpr auto bracket(char pre, char suf) noexcept;

	/**
	 * @brief 解析 pair 的说明符
	 *
	 * @param ctx 解析上下文
	 *
	 * @return 错误码，0 表示成功
	 * @retval 0                              解析成功
	 * @retval FormatError::InvalidFormatSpec 说明符不符合语法，或某个元素的子说明符非法
	 */
	auto parse(FormatParseContext &ctx);

	/**
	 * @brief 按已解析的说明符格式化一个广义 pair
	 *
	 * @param value 被格式化的值
	 * @param ctx   输出上下文
	 *
	 * @return 错误码，0 表示成功；任一元素失败时返回该元素的错误码
	 */
	auto format(const _Pair &value, FormatContext &ctx) const;
};

/** @brief 见 @c formatter::bracket 的声明处说明 */
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

/**
 * @brief 广义 pair formatter 的 @c parse 实现
 *
 * @note 这里统一用「下标偏移 + @c etl::string_view::substr 」切分子说明符，而**不要**拿
 *       @c string_view 的迭代器去构造 @c string_view ：各标准库中 @c string_view::iterator
 *       的具体类型不同（MSVC 上并非裸指针，且 @c _HAS_CXX23 在 C++17 模式下也会被定义为 0 ，
 *       使原先的 @c \#if 分支判断失效）。
 */
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

/** @brief 见 @c formatter::format 的声明处说明 */
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
 * @details 说明符的解析复用旧核心的 @ref FormatLex （fill / align / sign / @c 0 / width /
 * @c .precision ），但**渲染走 @ref EMBMartin::fixed_detail::render 的整数运算**，与 ETL 后端
 * 共用同一份实现，因此两套后端输出逐字符一致，也不会碰到旧核心「平局舍入随
 * @c long@c double 位数变化」的坑。
 *
 * @tparam N 默认小数位数
 *
 * @note @c _prec 在调用 @ref FormatLex::parse 之前被置为 @c -1 ，用来区分「说明符里没写
 *       precision」（保持 @c -1 ，用 @c fixed<N> 的 @c N ）与「写了 precision」（大于等于 0 ，
 *       覆盖 @c N ）。旧核心自身的算术 formatter 仍然使用 @c default_prec ，不受影响。
 */
template <size_t N>
struct formatter<::EMBMartin::fixed<N>, void> : public FormatLex<::EMBMartin::fixed<N>>
{
	using BaseType = FormatLex<::EMBMartin::fixed<N>>; ///< 说明符词法解析器的基类别名

	/**
	 * @brief 解析 @c fixed<N> 的说明符
	 *
	 * @param ctx 解析上下文
	 * @return 错误码，0 表示成功
	 *
	 * @note 解析前先把 @c _type 固定为 @c fFixed 、把 @c _prec 置为 @c -1 哨兵；
	 *       说明符里若给出别的表示类型，会在 @ref format 中被拒绝。
	 */
	int parse(FormatParseContext &ctx)
	{
		// fixed<N> 默认按定点小数处理；说明符里给了别的表示类型会在 format 里被拒绝
		this->_type = BaseType::Type::fFixed;
		this->_prec = -1; // 哨兵：区分「未给 precision」
		return BaseType::parse(ctx);
	}

	/**
	 * @brief 渲染 @c fixed<N> 的值
	 *
	 * @param value 被格式化的值
	 * @param ctx   输出上下文
	 *
	 * @return 错误码，0 表示成功
	 * @retval FormatError::Success           成功
	 * @retval FormatError::InvalidFormatSpec 使用了定点不支持的表示类型或选项
	 * @retval FormatError::BufferOverflow    输出缓冲区不足
	 */
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

/**
 * @brief 统计格式串中占位符的个数
 *
 * @param fmt 格式串
 *
 * @return 占位符个数（@c "{{" 与 @c "}}" 转义不计入）
 *
 * @note 结果 **不** 加校验，纯计数；当前仅用于将来的编译期实参个数检查，
 *       尚未接入 @c format_to(char*,@c size_t,@c const@c FormatString<N>&,@c const@c Args&...) 。
 */
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

/**
 * @brief 格式化单个实参：解析说明符后写入上下文
 *
 * @tparam Arg 实参类型
 * @param ctx      输出上下文
 * @param fmt_spec 该实参的说明符原文（不含花括号）
 * @param arg      实参
 *
 * @return 错误码，0 表示成功；解析失败时返回解析阶段的错误码，否则返回格式化阶段的错误码
 */
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

/**
 * @brief 递归展开实参包、逐个替换占位符的实现（主模板）
 *
 * @tparam Args 实参类型包
 *
 * @note 本模板只有两个特化：空实参的 @c FormatImpl<> 与至少一个实参的
 *       @c FormatImpl<Arg, Rest...> 。
 */
template <typename... Args>
struct FormatImpl;

/**
 * @brief 实参包为空时的递归终止情况
 *
 * @details 把剩余格式串**原样**写出（不做 @c "{{" / @c "}}" 转义），并补一个结尾 @c '\\0' 。
 */
template <>
struct FormatImpl<>
{
	/**
	 * @brief 把剩余格式串原样写入上下文
	 *
	 * @param ctx 输出上下文
	 * @param fmt 剩余格式串
	 *
	 * @return 错误码，0 表示成功
	 * @retval FormatError::Success        成功
	 * @retval FormatError::BufferOverflow 写入格式串或结尾 @c '\\0' 时空间不足
	 */
	static int format(FormatContext &ctx, etl::string_view fmt)
	{
		// 直接输出剩余的格式字符串
		ctx.write_safe(fmt);
		return ctx.write_safe('\0') ? int(FormatError::Success) : static_cast<int>(FormatError::BufferOverflow);
	}
};

/**
 * @brief 至少有一个实参时的递归情况
 *
 * @tparam Arg  当前实参类型
 * @tparam Rest 其余实参类型
 */
template <typename Arg, typename... Rest>
struct FormatImpl<Arg, Rest...>
{
	/**
	 * @brief 处理第一个占位符，然后递归处理其余实参
	 *
	 * @param ctx  输出上下文
	 * @param fmt  剩余格式串
	 * @param arg  当前实参
	 * @param rest 其余实参
	 *
	 * @return 错误码，0 表示成功
	 * @retval FormatError::MismatchedBraces 占位符缺少匹配的 @c '}'
	 * @retval FormatError::BufferOverflow   输出缓冲区不足
	 * @retval FormatError::Success          成功（无剩余占位符时返回剩余长度）
	 *
	 * @note 找不到占位符时直接写出剩余文本并结束，不会因为「实参多于占位符」而报错。
	 */
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

/**
 * @brief 旧接口：按格式串把实参写入调用方提供的缓冲区
 *
 * @tparam N    格式串字面量数组长度（等于字符串长度加 1）
 * @tparam Args 实参类型包
 *
 * @param buffer   输出缓冲区
 * @param capacity 缓冲区容量（字节）
 * @param fmt_str  格式串包装，见 @c FormatString
 * @param args     待格式化的实参
 *
 * @return 错误码，**不是**写入长度
 * @retval 0 成功且格式串含占位符
 * @retval 正数 成功但格式串不含占位符（值无实际意义，仅表示非负）
 * @retval 负数 失败，取值为 @c FormatError 的对应项
 *
 * @warning 返回值**不能**当作「写入字符数」使用：成功时它可能是 0 或剩余串长度。
 *          需要写入长度请改用 @c formatted_size 或 ETL 风格接口。
 * @note 成功时缓冲区中会带结尾 @c '\\0' （由 @c FormatImpl<> 追加）。
 * @note 这是旧接口，不受 @c EMBMARTIN_FMT_USE_ETL 影响；使用时请写
 *       @c EMBMartin::legacy_fmt:: 限定名。
 * @see FormatError
 */
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
	// # ETL 风格接口适配（建于旧核心之上）
	//
	// 目的：让 EMBMARTIN_FMT_USE_ETL 为 0 时，仓库里所有使用 ETL 风格接口的调用点无需改动即可
	//       编译并正确运行。类型别名与函数名和 etl_fmt 完全一致。
	//
	// 错误处理：与 ETL 一致，非法格式串走 EMBMARTIN_FMT_ASSERT，不返回错误码。需要错误码的
	//           旧代码请直接使用上面那套旧接口（FormatError / int format_to）。
	//
	// 自定义点：legacy_fmt::formatter<T> 仍是**旧签名**的（parse(FormatParseContext&) -> int、
	//           format(const T&, FormatContext&) -> int）。这与 ETL 的 formatter 签名不同，
	//           属于两套后端固有的差异（旧实现无法支持 ETL 那套 parse/format 签名）。
	// ################################################################################################

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

	// `formatter` / `is_formattable` / `is_formattable_v` 沿用上面旧核心定义的那一套（旧签名）

	// ------------------------------------------ETL 风格入口--------------------------------------------------

	/**
	 * @brief 把整串原样逐字符写出（最多 @p n 个）
	 *
	 * @tparam OutputIt 输出迭代器类型
	 * @param out  输出迭代器
	 * @param text 待写出的文本
	 * @param n    最多写出的字符数
	 *
	 * @return 已推进的输出迭代器
	 *
	 * @details 旧核心在**无实参**时走 @c FormatImpl<> ，语义就是「把剩余的格式串原样写出、
	 * 不做 @c "{{" / @c "}}" 转义」。但那条路径是 **all-or-nothing** 的：整串一次
	 * @c write_safe 装不下就一个字符也不写，而且忽略失败、仍返回成功。适配层因此对无实参
	 * 情形直接做逐字符拷贝——与旧语义完全等价，同时保留了「按容量截断」的契约
	 * （旧实现的 @c String::_format_impl 也是这么兜的）。
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
	 * @tparam OutputIt 输出迭代器类型（不能是 @c etl::istring 的派生类）
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 已推进的输出迭代器
	 *
	 * @warning 旧核心没有「只计数」通道，本函数用 @c EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE 字节
	 *          栈缓冲承接一次完整结果；超出该大小时断言失败（而不是静默给出错误结果）。
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

	/**
	 * @brief 运行期格式串 → 输出迭代器（@c vformat_to 的别名，便于与 ETL 名字对齐）
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
		return legacy_fmt::vformat_to(out, fmt_str, args...);
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
	 * @return 已推进的输出迭代器
	 *
	 * @note 旧后端不做编译期格式串检查，字面量重载与运行期版本行为一致。
	 */
	template <typename OutputIt, size_t N, typename... Args,
			  typename = std::enable_if_t<!std::is_base_of_v<etl::istring, std::remove_reference_t<OutputIt>>>>
	OutputIt format_to(OutputIt out, const char (&fmt_str)[N], const Args &...args)
	{
		return legacy_fmt::vformat_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/**
	 * @brief 最多写 @p n 个字符；返回已推进的输出迭代器
	 *
	 * @tparam OutputIt 输出迭代器类型（不能是 @c etl::istring 的派生类）
	 * @tparam Args     实参类型包
	 * @param out     输出迭代器
	 * @param n       最多写入的字符数
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 已推进的输出迭代器（即实际写入 @c min(n, len) 个字符后的位置）
	 *
	 * @note 返回类型与 ETL 的 @c etl::format_to_n 一致（裸迭代器），不是
	 *       @c format_to_n_result 。需要「本应写入多少字符」请用 @c formatted_size 。
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
		return legacy_fmt::format_to_n(out, n, etl::string_view(fmt_str, N - 1), args...);
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
		etl::istring::iterator result = legacy_fmt::format_to_n(out.begin(), out.max_size(), fmt_str, args...);
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
		return legacy_fmt::format_to(out, etl::string_view(fmt_str, N - 1), args...);
	}

	/**
	 * @brief 只计算所需字符数
	 *
	 * @tparam Args 实参类型包
	 * @param fmt_str 运行期格式串
	 * @param args    待格式化的实参
	 *
	 * @return 完整格式化本应产生的字符数
	 *
	 * @warning 旧核心没有「只计数」通道，本函数用 @c EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE 字节
	 *          栈缓冲跑完一次完整格式化；结果超过该大小时断言失败。
	 */
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
		return legacy_fmt::formatted_size(etl::string_view(fmt_str, N - 1), args...);
	}

	// `is_formattable` / `is_formattable_v` 由上面的旧核心提供（旧签名自定义点的探测），此处不重复定义。
} // namespace legacy_fmt

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

// #################################################################################################
// # 自定义格式化器迁移：广义 pair 的 etl::formatter<T> 特化
//
// 为什么必须写在**全局** etl 命名空间里：
//   - ETL 后端：ETL 的格式化核心只通过 etl::formatter<T> 识别自定义类型；
//   - 若写在 EMBMartin 内部，etl 会被 EMBMartin 作用域内的名字遮挡，特化落到错误的命名空间。
//
// 旧后端的同名自定义点仍然是 legacy_fmt::formatter<T>（旧签名），由上面的旧核心提供，不受影响。
//
// ETL 的 vformat_to 流程是「先由 ETL 的 parse_format_spec 吃掉认识的标准片段，再把剩余部分
// 交给自定义 formatter 的 parse」。本特化因此：
//   1. parse：把 ETL 吃剩的文本当成广义 pair 说明符解释，并整体消费到收尾 '}'；
//   2. 每个元素的子说明符复用 ETL 自己的 parse_format_spec，不重写任何说明符解析；
//   3. format：分别调用元素类型自己的 etl::formatter<Tx/Ty>，把结果拼成 "[x, y]"。
//
// 语法差异（ETL 后端）："<...>" 括号形式会与 ETL 的对齐符 '<' 冲突，故 ETL 后端下只能用
// "(...)" 与 "[...]" 两种括号；无括号形式的 "{::#X;:#b}" 也不可用（':' 已被 ETL 消费）。
// 需要完整旧语法时请用 EMBMARTIN_FMT_USE_ETL=0，旧后端的 pair 语法与旧实现完全一致。
// #################################################################################################

EMBMARTIN_NAMESPACE_END

namespace etl
{
	/**
	 * @brief 广义 pair 的 ETL 自定义格式化器
	 *
	 * @details 当 @c _Pair 是可解包的广义 pair 时，本特化选中并把两个元素按各自的子说明符
	 * 分别格式化，拼成 @c "[x,@c y]" 形式（括号与分隔符由说明符决定）。
	 *
	 * @tparam _Pair 广义 pair 类型
	 *
	 * @note 关于特化的写法：@c etl::formatter 的第二个模板参数是**字符类型**（默认 @c char ），
	 *       不是 @c std::enable_if 的落点。实测（armclang 6.24）：
	 *       - @c "formatter<T,@c char>"             → 能匹配，但无法约束；
	 *       - @c "formatter<T,@c enable_if_t<...>>" → 落点是 @c void ，**永远匹配不上**（ETL 只用 @c char ）；
	 *       - @c "formatter<T,@c conditional_t<cond,@c char,@c 不完整类型>>" → 既能匹配又能约束。
	 *       故这里用第三种：条件成立时槽位是 @c char ，否则替换失败（SFINAE）。
	 *
	 * @see EMBMartin::pair_spec
	 */
	template <typename _Pair>
	struct formatter<_Pair,
					 etl::conditional_t<::EMBMartin::pair_spec::is_decomposable_pair<_Pair>::value,
										char,
										::EMBMartin::pair_spec::not_a_pair>>
	{
		using _Tx = ::EMBMartin::generalized_pair_element_t<0, _Pair>; ///< 左元素类型
		using _Ty = ::EMBMartin::generalized_pair_element_t<1, _Pair>; ///< 右元素类型

		using _Spec = etl::private_format::format_spec_t; ///< ETL 的说明符类型

		::EMBMartin::pair_spec::parts _parts{}; ///< 切分后的 pair 说明符
		_Spec _spec_x{};						///< 左元素的说明符（仅在 @c _has_element_spec 为真时有效）
		_Spec _spec_y{};						///< 右元素的说明符（仅在 @c _has_element_spec 为真时有效）
		bool _has_element_spec = false;			///< 是否显式给出了元素级子说明符

		/**
		 * @brief 解析 pair 的说明符
		 *
		 * @param parse_ctx ETL 的解析上下文，指向 @c ':' 之后、@c '}' 之前
		 * @return 指向收尾 @c '}' 的迭代器
		 */
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

		/**
		 * @brief 格式化一对值
		 *
		 * @tparam OutputIt 输出迭代器类型
		 * @param value   被格式化的 pair
		 * @param fmt_ctx ETL 的格式上下文
		 * @return 已推进的输出迭代器
		 *
		 * @note 处理完两个元素后会恢复外层说明符（@c fmt_ctx.format_spec = outer ），
		 *       以免影响后续替换字段。
		 */
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
		etl::formatter<_Tx> _formatter_x{}; ///< 左元素的格式化器
		etl::formatter<_Ty> _formatter_y{}; ///< 右元素的格式化器

		/**
		 * @brief 解析一个元素子说明符
		 *
		 * @tparam Formatter 该元素的 formatter 类型
		 * @param spec              子说明符文本
		 * @param out               [out] 解析出的 ETL 说明符
		 * @param element_formatter 该元素的 formatter，解析后供 @c format 使用
		 *
		 * @note 完全照搬 ETL 的顺序：先用 ETL 自己的 @c parse_format_spec 吃掉标准片段，
		 *       再把剩余部分交给元素自己的 formatter（内置 formatter 的 @c parse 是空操作）。
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
	 * @brief  @ref EMBMartin::fixed "fixed<N>" 在 ETL 后端下的自定义格式化器
	 *
	 * @details
	 * ETL 已把它认识的说明符片段解析进 `fmt_ctx.format_spec`（fill / align / sign /
	 * `0` / width / precision / type），本特化只把这些字段翻译成
	 * 渲染交给与旧后端共用的整数运算实现 @c EMBMartin::fixed_detail::spec 。
	 * 因此 ETL 那个「浮点 precision 被忽略」的缺陷不会影响 `fixed<N>`。
	 *
	 * 支持的表示类型只有 `f` 与 `F`；给出 `e` / `g` / `a` 等其它表示类型会触发
	 * @c EMBMARTIN_FMT_ASSERT。
	 *
	 * @tparam N 默认小数位数，见 @ref EMBMartin::fixed
	 */
	template <size_t N>
	struct formatter<::EMBMartin::fixed<N>, char>
	{
		/**
		 * @brief 消费本替换字段的说明符
		 *
		 * @param parse_ctx 解析上下文，指向 `:` 之后、`}` 之前
		 * @return 指向收尾 `}` 的迭代器
		 *
		 * @note `fixed<N>` 没有自己的额外说明符语法，故把 ETL 吃剩的部分整体消费到 `}`
		 *       即可；若用户给出定点以外的表示类型，统一在 @ref format 中断言拒绝。
		 */
		etl::format_parse_context::iterator parse(etl::format_parse_context &parse_ctx)
		{
			auto it = parse_ctx.begin();
			while (it != parse_ctx.end() && *it != '}')
			{
				++it;
			}
			return it;
		}

		/**
		 * @brief 渲染 `fixed<N>` 的值
		 *
		 * @tparam OutputIt 输出迭代器类型
		 * @param value   被格式化的值
		 * @param fmt_ctx 格式上下文，其中 `format_spec` 已由 ETL 解析好
		 * @return 已推进的输出迭代器
		 */
		template <typename OutputIt>
		typename format_context<OutputIt>::iterator format(const ::EMBMartin::fixed<N> &value,
														   format_context<OutputIt> &fmt_ctx)
		{
			using spec_t = etl::private_format::format_spec_t;

			const spec_t &fs = fmt_ctx.format_spec;

			if (fs.type.has_value() && fs.type.value() != 'f' && fs.type.value() != 'F')
			{
				// 只支持 f / F：e / g / a 等定点以外的表示类型没有意义
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
		 * @brief  为「自定义可格式化类型」收窄 ETL 的编译期参数掩码
		 *
		 * @details
		 * **问题**：ETL 的 `basic_format_arg` 用一个含 21 个候选的 variant 存放实参，
		 * `etl::visit` 会遍历其中每一个候选。为免实例化用不到的 formatter（尤其整条浮点
		 * 链），ETL 用 `arg_type_mask<T>` 在编译期算出「本次调用可能命中哪些候选」，再由
		 * `format_visitor<OutputIt, Mask>` 把不可达候选的 `dispatch` 编成空函数。
		 *
		 * 然而 `arg_type_mask` 只为 21 个内建类型及 `etl::ibasic_string` 的派生类提供了
		 * 特化；对**其它任何类型**，主模板保守地返回 `mask_all`：
		 * @code
		 * template <typename T, typename Enable = void>
		 * struct arg_type_mask { static constexpr arg_mask_t value = mask_all; };
		 * @endcode
		 * 于是只要一次格式化调用中出现自定义类型（本仓库的 `Coordinate`、`add_result`、
		 * 广义 pair 等），该次调用的 Mask 便退化为 `mask_all`，ETL 会把 **21 个内建分支的
		 * formatter 全部实例化**——包括 `format_floating_*` 整条链及其拖入的
		 * `pow` / `log` / `log10` 等数学库函数。这是本组件接入 ETL 后代码体积明显增大的
		 * 真正原因（并非两套后端被同时实例化）。
		 *
		 * **为何收窄是安全的**：通过 `etl::formatter<T>` 特化接入的自定义类型，在
		 * `basic_format_arg` 中存放于 **`handle` 槽**（见 ETL 的 converting constructor），
		 * 格式化时走 `format_visitor::operator()(handle)` 这一**非模板重载**，完全不经过
		 * 内建分支的 `dispatch`。即：对该类型而言 21 个内建分支一个都用不到，`mask_all`
		 * 中除 `mask_monostate`（掩码递归的下界、恒置位）之外全是浪费。
		 *
		 * 因此这里把「所有提供了 `etl::formatter<T>` 特化的类型」统一收窄到
		 * `mask_monostate`。它与 ETL 自带的 `ibasic_string` 特化互斥：后者只对
		 * `etl::ibasic_string<char>` 的派生类（`etl::string` / `etl::istring` 等）生效，
		 * 而那些类型在 `basic_format_arg` 中存放于 `string_view` 槽、走内建分支；本特化
		 * 只接管真正走 `handle` 槽的类型，二者不会同时对同一类型成立。
		 *
		 * @par 实测效果
		 * Keil armclang V6.24，本仓库 `martin_test/main.cpp` 当前版本：
		 * - 未收窄：`Code = 59396` B
		 * - 收窄后：`Code = 37252` B（节省 22144 B）
		 * - 对照：同一份代码的旧后端为 `Code = 42520` B
		 *
		 * @note 本特化只让编译器不生成用不到的代码，**不改变任何格式化结果**。
		 * @warning 这里特化的是 ETL 的内部扩展点。若将来 ETL 改为「自定义类型也走内建槽」，
		 *          本特化需要重新评估。
		 */
		template <typename T>
		struct arg_type_mask<T, etl::enable_if_t<is_formattable<T>::value>>
		{
			/// @brief 仅保留 `mask_monostate` 位，裁掉全部内建分支
			static constexpr arg_mask_t value = mask_monostate;
		};
	} // namespace private_format
} // namespace etl

// 注：EMBMARTIN_FMT_INLINE_ETL / EMBMARTIN_FMT_INLINE_LEGACY 这两个内部宏**故意不 #undef**，
// 因为 fmt.cpp 需要用同样的内联性重新打开对应的命名空间
//（否则 clang 会报 -Winline-namespace-reopened-noninline）。

