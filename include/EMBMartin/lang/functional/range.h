/**
 ******************************************************************************
 * @file    range.h
 * @brief   Range<T> 别名与 range() 便捷工厂；Range 与 Slice 语义完全一致
 ******************************************************************************
 */

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "EMBMartin/lang/functional/slice.h"

EMBMARTIN_NAMESPACE_BEGIN

template <typename T>
using Range = Slice<T>;

/**
 * @brief 便捷构造 `Range<size_t>`。
 *
 * @param start 起始索引（含）。
 * @param stop 结束索引（不含）。
 * @param step 步长，默认为 `1`；负值表示反向步进。
 * @return 返回对应的 `Range<size_t>`。
 *
 * @note 形参类型是 `int`，内部会 `static_cast` 到 `size_t`；请只传非负索引。
 * 负值会转换成很大的无符号数，可能触发 `Slice` 的方向断言，或得到不符合直觉的区间。
 */
constexpr inline Range<size_t> range(int start, int stop, int step = 1)
{
	return Range<size_t>(static_cast<size_t>(start), static_cast<size_t>(stop), static_cast<std::make_signed_t<size_t>>(step));
}

/** @brief 便捷构造从 `0` 开始、步长为 `1` 的 `Range<size_t>`；`stop` 的取值要求同上。 */
constexpr inline Range<size_t> range(int stop)
{
	return Range<size_t>(static_cast<size_t>(stop));
}

EMBMARTIN_NAMESPACE_END
