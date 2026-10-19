#pragma once

// fmt 组件的公共配置：头文件依赖、错误上报宏、后端选择宏、内联命名空间映射。
// 由 fmt.h 及其拆分出的各子文件共同包含。

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
