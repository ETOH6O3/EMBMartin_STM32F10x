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
 * | @c 1（默认） | @c EMBMartin::lang::etl_fmt | 直转发第三方库 ETL 的 @c format |
 * | @c 0 | @c EMBMartin::lang::legacy_fmt | 本库既有旧格式化核心（原样保留）+ ETL 风格接口适配 |
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
 *       两套实现始终都被编译，因此始终可以写出 @c EMBMartin::lang::legacy_fmt::... 或
 *       @c EMBMartin::lang::etl_fmt::... 来显式指定后端。任何时刻都恰好只有一个内联命名空间，
 *       故 @c EMBMartin::lang::format_to(...) 这类写法不会产生二义。
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
 *          @ref EMBMartin::lang::legacy_fmt::FormatError 错误码（仅 @c legacy_fmt 提供）。
 *
 * @section fmt_legacy_compat 旧接口兼容层
 *
 * @c legacy_fmt 内部**原样保留**旧实现及其旧接口，供仍需 @c FormatError 错误码的代码使用：
 * - @ref EMBMartin::lang::legacy_fmt::FormatError
 * - @ref EMBMartin::lang::legacy_fmt::FormatContext
 * - @ref EMBMartin::lang::legacy_fmt::FormatParseContext
 * - @ref EMBMartin::lang::legacy_fmt::FormatString
 * - @ref EMBMartin::lang::legacy_fmt::FormatLex
 * - @ref EMBMartin::lang::legacy_fmt::formatter （旧签名的自定义点，见下）
 * - 旧的 @ref EMBMartin::lang::legacy_fmt::format_to "int format_to(char*, size_t, const FormatString<N>&, const Args&...)"
 *
 * 其中旧签名自定义点要求实现：
 * - @c int @c parse(FormatParseContext@c &)  —— 返回 0 表示成功，负数表示错误码；
 * - @c int @c format(const@c T@c &, FormatContext@c &)  —— 同上。
 *
 * @note 这不是新接口，不随 @c EMBMARTIN_FMT_USE_ETL 变化；使用它必须写
 *       @c EMBMartin::lang::legacy_fmt:: 限定名。
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
 * 需要固定小数位时请使用 @ref EMBMartin::lang::fixed "EMBMartin::lang::fixed<N>" ：
 * @code
 * oled.println("Dist: {}m", EMBMartin::lang::fixed<2>{distance});   // "Dist: 1.23m"
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
 *   @c {:.Nf} ；要固定小数位就用 @ref EMBMartin::lang::fixed 。
 * - 旧核心的浮点舍入建立在 @c long@c double 之上。当被丢弃的部分**恰好以 5 开头**
 *   （平局）时，结果取决于 @c long@c double 的有效位数：MinGW/x86-64 为 80 位
 *   （进位的概率更高），MSVC 与 armclang(ARM) 为 64 位。例如旧后端 @c {:.3e} 对
 *   @c 1234.5f 可能是 @c 1.235e+003 也可能是 @c 1.234e+003 。这是被原样保留的旧实现
 *   自身的性质，与本次迁移无关；@c fixed<N> 与 ETL 后端的其它路径不受影响。
 *
 * @see EMBMartin::lang::fixed
 * @see EMBMartin::lang::pair_spec
 * @see EMBMARTIN_FMT_ASSERT
 * @see FMT.md —— 面向使用者的接口文档
 */
#pragma once

// 本文件为 fmt 组件的总入口，按原有顺序汇总各拆分文件：
//
//   fmt_config.h           头文件依赖、EMBMARTIN_FMT_ASSERT、后端选择宏、内联命名空间映射
//   fmt_helpers.h          String 前向声明、pair_spec、fixed_detail（两套后端共用）
//   fmt_legacy.h           后端一 legacy_fmt（旧核心 + ETL 风格适配）
//   fmt_etl.h              后端二 etl_fmt（直转发 ETL）
//   fmt_etl_formatters.h   etl::formatter 特化与 arg_type_mask 收窄
//
// 拆分只为减少单文件体积，对外接口与包含方式完全不变：包含 "fmt.h" 即可。

#include "EMBMartin/lang/fmt/fmt_config.h"
#include "EMBMartin/lang/fmt/fmt_helpers.h"
#include "EMBMartin/lang/fmt/fmt_legacy.h"
#include "EMBMartin/lang/fmt/fmt_etl.h"
#include "EMBMartin/lang/fmt/fmt_etl_formatters.h"
