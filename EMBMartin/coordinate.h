#ifndef EMBMARTIN_COORDINATE_H
#define EMBMARTIN_COORDINATE_H

#include <cstdint>
#include <utility>
#include <algorithm>

#include "macro.h"
#include "mmath.h"
#include "fmt.h"

EMBMARTIN_NAMESPACE_BEGIN

template <
	typename _Ty,
	_Ty XMin = std::numeric_limits<_Ty>::min(),
	_Ty XMax = std::numeric_limits<_Ty>::max(),
	_Ty YMin = std::numeric_limits<_Ty>::min(),
	_Ty YMax = std::numeric_limits<_Ty>::max()>
class Coordinate
{
	static_assert(std::is_integral_v<_Ty>, "Coordinate only supports integral types");
	static_assert(XMin <= XMax, "Invalid XRange in Coordinate");
	static_assert(YMin <= YMax, "Invalid YRange in Coordinate");

public:
	using ValueType = _Ty;		  // 个人风格
	using value_type = ValueType; // 与标准库一致

	static constexpr inline ValueType x_min = XMin;
	static constexpr inline ValueType x_max = XMax;
	static constexpr inline ValueType x_diff = x_max - x_min;
	static constexpr inline ValueType x_diff_plus1 = x_diff + 1;
	static constexpr inline ValueType y_min = YMin;
	static constexpr inline ValueType y_max = YMax;
	static constexpr inline ValueType y_diff = y_max - y_min;
	static constexpr inline ValueType y_diff_plus1 = y_diff + 1;

public:
	_Ty x = 0;
	_Ty y = 0;

public:
	constexpr inline Coordinate(const ValueType& x, const ValueType& y) noexcept
		: x(std::clamp(x, x_min, x_max)), y(std::clamp(y, y_min, y_max))
	{
	}

	constexpr inline add_result<Coordinate&> assignment_add(const ValueType& step) noexcept
	{
		if constexpr (std::is_unsigned_v<ValueType>)
		{
			const auto& [xvalue, xcarry] = add<ValueType>(x, step);
			x = (xvalue - x_min) % x_diff_plus1 + x_min;
			const auto& [yvalue, ycarry] = add<ValueType>(y, xcarry + (xvalue - x_min) / x_diff_plus1);
			y = (yvalue - y_min) % y_diff_plus1 + y_min;
			return { *this, ycarry };
		}
		else
		{
			static_assert(0, "Hasn't implemented for signed types yet");
		}
	}
	constexpr inline add_result<Coordinate&> assignment_add_y(const ValueType& step) noexcept
	{
		if constexpr (std::is_unsigned_v<ValueType>)
		{
			const auto& [yvalue, ycarry] = add<ValueType>(y, step);
			y = (yvalue - y_min) % y_diff_plus1 + y_min;
			return { *this, ycarry };
		}
		else
		{
			static_assert(0, "Hasn't implemented for signed types yet");
		}
	}
	constexpr inline Coordinate& operator+=(const ValueType& step) noexcept
	{
		return assignment_add(step).sum;
	}

	constexpr inline Coordinate& operator-=(const ValueType& step) noexcept;
	constexpr Coordinate& operator++() noexcept;
	constexpr inline Coordinate operator++(int) noexcept
	{
		Coordinate temp = *this;
		++(*this);
		return temp;
	}
	constexpr Coordinate& operator--() noexcept;
	constexpr inline Coordinate operator--(int) noexcept
	{
		Coordinate temp = *this;
		--(*this);
		return temp;
	}

	constexpr inline add_result<Coordinate&> increment() noexcept
	{
		bool carry = (x == x_max) && (y == y_max);
		++(*this);
		return { *this, carry };
	}
	constexpr inline add_result<Coordinate&> decrement() noexcept
	{
		bool borrow = (x == x_min) && (y == y_min);
		--(*this);
		return { *this, borrow };
	}
#if !(defined(_HAS_CXX23) && _HAS_CXX23)  // C++17
	template <size_t N>
	constexpr inline const auto& get() const& noexcept
	{
		if constexpr (N == 0)
			return x;
		else if constexpr (N == 1)
			return y;
		else
			static_assert(N < 2, "Index out of bounds in Coordinate::get<N>()");
	}
	template <size_t N>
	constexpr inline auto& get() & noexcept
	{
		if constexpr (N == 0)
			return x;
		else if constexpr (N == 1)
			return y;
		else
			static_assert(N < 2, "Index out of bounds in Coordinate::get<N>()");
	}
	template <size_t N>
	constexpr inline const auto&& get() const&& noexcept
	{
		if constexpr (N == 0)
			return std::move(x);
		else if constexpr (N == 1)
			return std::move(y);
		else
			static_assert(N < 2, "Index out of bounds in Coordinate::get<N>()");
	}
	template <size_t N>
	constexpr inline auto&& get() && noexcept
	{
		if constexpr (N == 0)
			return std::move(x);
		else if constexpr (N == 1)
			return std::move(y);
		else
			static_assert(N < 2, "Index out of bounds in Coordinate::get<N>()");
	}
#else 
	template <size_t N>
	constexpr inline decltype(auto) get(this auto&& self)
	{
		if constexpr (N == 0)
			return std::forward_like<decltype(self)>(self.x);
		else if constexpr (N == 1)
			return std::forward_like<decltype(self)>(self.y);
		else
			static_assert(N < 2, "Index out of bounds in Coordinate::get<N>()");
	}
#endif // _HAS_CXX23

};

template <typename _Ty, _Ty XMin, _Ty XMax, _Ty YMin, _Ty YMax>
constexpr Coordinate<_Ty, XMin, XMax, YMin, YMax>& Coordinate<_Ty, XMin, XMax, YMin, YMax>::operator++() noexcept
{
	++x;
	if (x > x_max)
	{
		x = x_min;
		++y;
		if (y > y_max)
		{
			y = y_min;
		}
	}
	return *this;
}

template <typename _Ty, _Ty XMin, _Ty XMax, _Ty YMin, _Ty YMax>
constexpr Coordinate<_Ty, XMin, XMax, YMin, YMax>& Coordinate<_Ty, XMin, XMax, YMin, YMax>::operator--() noexcept
{
	--x;
	if (x < x_min)
	{
		x = x_max;
		--y;
		if (y < y_min)
		{
			y = y_max;
		}
	}
	return *this;
}

EMBMARTIN_NAMESPACE_END

namespace std
{
	template <typename T, T XMin, T XMax, T YMin, T YMax>
	struct tuple_size<EMBMartin::Coordinate<T, XMin, XMax, YMin, YMax>>
		: integral_constant<size_t, 2>
	{
	};

	template <typename T, T XMin, T XMax, T YMin, T YMax>
	struct tuple_element<0, EMBMartin::Coordinate<T, XMin, XMax, YMin, YMax>>
	{
		using type = T;
	};
	template <typename T, T XMin, T XMax, T YMin, T YMax>
	struct tuple_element<1, EMBMartin::Coordinate<T, XMin, XMax, YMin, YMax>>
	{
		using type = T;
	};
} // namespace std

#include "meta.h"

EMBMARTIN_NAMESPACE_BEGIN

EMBMARTIN_NAMESPACE_END

#endif