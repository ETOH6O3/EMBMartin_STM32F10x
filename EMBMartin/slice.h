/**
 ******************************************************************************
 * @file    slice.h
 * @brief   Python 风格半开区间切片描述器 Slice<T>（含随机访问迭代器）
 ******************************************************************************
 */

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "macro.h"
#include "meta.h"
#include "stream.h"

EMBMARTIN_NAMESPACE_BEGIN

//------------------------------------------python 风格 slice 和 range 对象 --------------------------------------------------

/**
 * @brief Python 风格半开区间切片描述器。
 *
 * `Slice` 允许以类似 Python 的语法描述一个区间：
 *
 * - `Slice(0, 10, 2)` 表示 [0, 10) 且步长为 2；
 *
 * - 允许正向或反向步进；
 *
 * - 提供带随机访问标签的迭代器接口，可直接被范围 for 使用。
 *
 * 该类型适用于对索引区间进行结构化描述，而不是直接存储元素本身。
 * 其 `start`/`stop`/`step` 语义与 Python 的 slicing 设计相近。索引本身是无符号值，
 * 负步长表示反向遍历，不提供 Python 的负索引归一化。
 *
 * 构造时会用 `EMBMARTIN_NON_BLOCKING_ASSERT` 做非阻塞检查：若断言未终止程序，
 * `step == 0` 或方向非法的区间仍会被接受，此时 `size()` 返回 `0`，
 * 不应再依赖 `operator[]` 或迭代器遍历的结果。
 *
 * @note 内部 `Iterator` 只提供随机访问标签与基本算术（`+`、`-`、`+=`、`-=`、
 * 前后置 `++`/`--` 及比较），可用于范围 for；但不保证完整满足 C++20
 * `std::random_access_iterator` 概念（缺少 `operator[]`、`n + it`、`it - it` 等）。
 */
template <typename T>
struct Slice
{
	static_assert(std::is_integral_v<T>);

private:
	using index_type = std::make_unsigned_t<T>;
	using step_type = std::make_signed_t<T>;

public:
	index_type start;
	index_type stop;
	step_type step;

	class Iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = index_type;
		using difference_type = std::ptrdiff_t;
		using pointer = void;
		using reference = index_type;

	private:
		const Slice &_slice;
		index_type __pos;

	public:
		constexpr inline Iterator(const Slice &slice, const index_type &pos = 0) noexcept : _slice{slice}, __pos{pos} {}
		constexpr inline auto operator*() const noexcept { return _slice[__pos]; }
		constexpr inline Iterator operator+(difference_type offset) const noexcept
		{
			const auto new_pos = offset >= 0
									 ? __pos + static_cast<index_type>(offset)
									 : __pos - static_cast<index_type>(-(offset + 1)) - 1;
			return Iterator(_slice, new_pos);
		}
		constexpr inline Iterator operator-(difference_type offset) const noexcept
		{
			const auto new_pos = offset >= 0
									 ? __pos - static_cast<index_type>(offset)
									 : __pos + static_cast<index_type>(-(offset + 1)) + 1;
			return Iterator(_slice, new_pos);
		}
		constexpr inline Iterator &operator+=(difference_type offset) noexcept
		{
			if (offset >= 0)
			{
				__pos += static_cast<index_type>(offset);
			}
			else
			{
				__pos -= static_cast<index_type>(-(offset + 1)) + 1;
			}
			return *this;
		}
		constexpr inline Iterator &operator-=(difference_type offset) noexcept
		{
			if (offset >= 0)
			{
				__pos -= static_cast<index_type>(offset);
			}
			else
			{
				__pos += static_cast<index_type>(-(offset + 1)) + 1;
			}
			return *this;
		}
		constexpr inline Iterator &operator++() noexcept
		{
			++__pos;
			return *this;
		}
		constexpr inline Iterator &operator--() noexcept
		{
			--__pos;
			return *this;
		}
		constexpr inline Iterator operator++(int) noexcept
		{
			Iterator temp = *this;
			++__pos;
			return temp;
		}
		constexpr inline Iterator operator--(int) noexcept
		{
			Iterator temp = *this;
			--__pos;
			return temp;
		}
		constexpr inline bool operator==(const Iterator &other) const noexcept
		{
			return __pos == other.__pos;
		}
		constexpr inline bool operator!=(const Iterator &other) const noexcept
		{
			return __pos != other.__pos;
		}
		constexpr inline bool operator<(const Iterator &other) const noexcept
		{
			return __pos < other.__pos;
		}
		constexpr inline bool operator<=(const Iterator &other) const noexcept
		{
			return __pos <= other.__pos;
		}
		constexpr inline bool operator>(const Iterator &other) const noexcept
		{
			return __pos > other.__pos;
		}
		constexpr inline bool operator>=(const Iterator &other) const noexcept
		{
			return __pos >= other.__pos;
		}
	};

public:
	constexpr inline Slice(const index_type &start, const index_type &stop, const step_type &step = 1)
		: start(start), stop(stop), step(step)
	{
		EMBMARTIN_NON_BLOCKING_ASSERT(step != 0, "Slice step must not be zero", &console);
		EMBMARTIN_NON_BLOCKING_ASSERT((step > 0 && start <= stop) || (step < 0 && start >= stop), "Slice range invalid", &console);
	}
	explicit constexpr inline Slice(const index_type &stop)
		: Slice(0, stop, 1)
	{
	}

	constexpr inline index_type operator[](const index_type &index) const
	{
		const auto magnitude = static_cast<index_type>(step > 0 ? step : -(step + 1)) + (step < 0 ? 1 : 0);
		const auto offset = index * magnitude;
		return step > 0 ? start + offset : start - offset;
	}

	constexpr inline Iterator begin() const noexcept
	{
		return Iterator(*this, 0);
	}
	constexpr inline Iterator end() const noexcept
	{
		return Iterator(*this, size());
	}
	constexpr inline Iterator cbegin() const noexcept
	{
		return Iterator(*this, 0);
	}
	constexpr inline Iterator cend() const noexcept
	{
		return Iterator(*this, size());
	}
	constexpr inline bool empty() const noexcept
	{
		return size() == 0;
	}
	constexpr inline index_type size() const noexcept
	{
		if (step == 0 || (step > 0 && start > stop) || (step < 0 && start < stop))
		{
			return 0;
		}
		const index_type distance = step > 0 ? stop - start : start - stop;
		const index_type magnitude = static_cast<index_type>(step > 0 ? step : -(step + 1)) + (step < 0 ? 1 : 0);
		return distance / magnitude + (distance % magnitude != 0);
	}
};


EMBMARTIN_NAMESPACE_END
