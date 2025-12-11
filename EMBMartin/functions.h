/**
 ******************************************************************************
 * @file    functions.h
 * @author  孙鸣淼
 * @brief   这里存放一些嵌入式开发中常用且不依赖于平台的函数
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#ifndef EMBMARTIN_FUNCTIONS_H
#define EMBMARTIN_FUNCTIONS_H

#include <limits>
#include <stdint.h>
#include <cmath>

#include "macro.h"

EMBMARTIN_NAMESPACE_BEGIN

// ------------------------------------------溢出处理--------------------------------------------------

template <class T>
struct add_result
{
	T sum;
	bool carry;
};

template <class T>
struct sub_result
{
	T difference;
	bool borrow;
};

template <class T>
struct mul_result
{
	T product_low;
	T product_high;
};

template <class T>
struct div_result
{
	T quotient;
	T remainder;
};

template <typename T>
std::enable_if_t<std::is_signed_v<T>, add_result<T>>
add(const T &a, const T &b) noexcept
{
	return {
		T(a + b),
		((b > 0) && (a > (std::numeric_limits<T>::max() - b))) ||
			((b < 0) && (a < (std::numeric_limits<T>::min() - b)))};
}
template <typename T>
std::enable_if_t<std::is_unsigned_v<T>, add_result<T>>
add(const T &a, const T &b) noexcept
{
	T rslt = a + b;
	return {rslt, rslt < a};
}

template <typename T>
std::enable_if_t<std::is_signed_v<T>, add_result<T>>
sub(const T &a, const T &b) noexcept
{
	return {
		T(a - b),
		((b > 0) && (a < (std::numeric_limits<T>::min() + b))) ||
			((b < 0) && (a > (std::numeric_limits<T>::max() + b)))};
}

template <typename T>
std::enable_if_t<std::is_unsigned_v<T>, add_result<T>>
sub(const T &a, const T &b) noexcept
{
	T rslt = a - b;
	return {rslt, rslt > a};
}

// ------------------------------------------数学运算--------------------------------------------------

/// 循环左移函数
/// 将整数类型的值向左循环移动指定位数
/// @param x 输入值，必须是整数类型
/// @param shamt 左移位数
/// @return 返回循环左移后的结果
/// @note 该函数只接受整数类型，否则会在编译时报错
template <typename T>
constexpr inline T rshl(const T &x, const int32_t &shamt) noexcept
{
	static_assert(std::is_integral<T>::value, "T must be an integral type");
	return (x << shamt) | (x >> (std::numeric_limits<T>::digits - shamt));
}

/// 循环右移函数
/// 将整数类型的值向右循环移动指定位数
/// @param x 输入值，必须是整数类型
/// @param shamt 右移位数
/// @return 返回循环右移后的结果
/// @note 该函数只接受整数类型，否则会在编译时报错
template <typename T>
constexpr inline T rshr(const T &x, const int32_t &shamt) noexcept
{
	static_assert(std::is_integral<T>::value, "T must be an integral type");
	return (x >> shamt) | (x << (std::numeric_limits<T>::digits - shamt));
}

/**
 * @brief 获取单个数值（模板重载版本1）
 *
 * @tparam T 输入值的类型
 * @param x 输入值
 * @return 返回输入值本身
 */
template <typename T>
constexpr inline T min(const T &x) noexcept
{
	return x;
}

/**
 * @brief 获取两个数值中的较小值（模板重载版本2）
 *
 * @tparam T 第一个值的类型
 * @tparam U 第二个值的类型
 * @param x 第一个值
 * @param y 第二个值
 * @return 返回两个值中的较小值
 */
template <typename T, typename U>
constexpr inline T min(const T &x, const U &y) noexcept
{
	return x < y ? x : y;
}

/**
 * @brief 获取多个数值中的最小值（模板重载版本3）
 *
 * @tparam T 第一个值的类型
 * @tparam U 第二个值的类型
 * @tparam Args 可变参数类型包
 * @param x 第一个值
 * @param y 第二个值
 * @param args 其他值
 * @return 返回所有值中的最小值
 */
template <typename T, typename U, typename... Args>
constexpr inline T min(const T &x, const U &y, const Args &...args) noexcept
{
	auto re = min(x, y);
	return min(re, args...);
}

/**
 * @brief 获取单个数值（模板重载版本1）
 *
 * @tparam T 输入值的类型
 * @param x 输入值
 * @return 返回输入值本身
 */
template <typename T>
constexpr inline auto max(const T &x) noexcept
{
	return x;
}

/**
 * @brief 获取两个数值中的较大值（模板重载版本2）
 *
 * @tparam T 第一个值的类型
 * @tparam U 第二个值的类型
 * @param x 第一个值
 * @param y 第二个值
 * @return 返回两个值中的较大值
 */
template <typename T, typename U>
constexpr inline auto max(const T &x, const U &y) noexcept
{
	return x > y ? x : y;
}

/**
 * @brief 获取多个数值中的最大值（模板重载版本3）
 *
 * @tparam T 第一个值的类型
 * @tparam U 第二个值的类型
 * @tparam Args 可变参数类型包
 * @param x 第一个值
 * @param y 第二个值
 * @param args 其他值
 * @return 返回所有值中的最大值
 */
template <typename T, typename U, typename... Args>
constexpr inline auto max(const T &x, const U &y, const Args &...args) noexcept
{
	auto re = max(x, y);
	return max(re, args...);
}

/*用于任意数目任意数型的gcd
为了支持约分函数需求，允许传入负数
特殊情况：
gcd(1, 0), gcd(0, 1), gcd(-4, -2),gcd(-4,2), gcd(2,-4)
1          1          -2          2          2
前5个符合约分函数要求，最后一个影响约分后流输出和打印结果（导致分母为负），但不影响分数比较
*/

/**
 * @brief 计算单个数的最大公约数（模板重载版本1）
 *
 * @tparam T 输入值的类型
 * @param a 输入值
 * @return 返回输入值本身
 */
template <typename T>
constexpr inline T gcd(const T &a) noexcept
{
	return a;
}

/**
 * @brief 计算两个数的最大公约数（模板重载版本2）
 *
 * @tparam T 第一个数的类型
 * @tparam U 第二个数的类型
 * @param a 第一个数
 * @param b 第二个数
 * @return 返回两个数的最大公约数
 *
 * 使用欧几里得算法计算最大公约数，如果第二个数为0则返回第一个数
 */
template <typename T, typename U>
constexpr inline auto gcd(const T &a, const U &b) noexcept
{
	return bool(b) ? gcd(b, a % b) : a;
}

/**
 * @brief 计算多个数的最大公约数（模板重载版本3）
 *
 * @tparam T 数值类型
 * @tparam Args 可变参数类型包
 * @param x 第一个数
 * @param y 第二个数
 * @param args 其他数
 * @return 返回所有数的最大公约数
 */
template <typename T, typename... Args>
constexpr inline T gcd(const T &x, const T &y, const Args &...args) noexcept
{
	T re = gcd(x, y);
	return gcd(re, args...);
}

/*用于任意数目任意数型的lcm
为了支持通分函数需求，允许传入负数
特殊情况：
lcm(1, 0), lcm(0, 1), lcm(-4, -2),lcm(-4,2), lcm(2,-4)
0          0          -4          -4         -4
通分函数不应当出现前两种情况，因为a/1不能与b/0（∞）通分
后3个基本不符合通分函数要求，会影响通分后流输出和打印结果（导致分母为负），但不影响分数比较
*/

/**
 * @brief 计算单个数的最小公倍数（模板重载版本1）
 *
 * @tparam T 输入值的类型
 * @param a 输入值
 * @return 返回输入值本身
 */
template <typename T>
constexpr inline T lcm(const T &a) noexcept
{
	return a;
}

/**
 * @brief 计算两个数的最小公倍数（模板重载版本2）
 *
 * @tparam T 第一个数的类型
 * @tparam U 第二个数的类型
 * @param x 第一个数
 * @param y 第二个数
 * @return 返回两个数的最小公倍数
 *
 * 使用公式 lcm(a,b) = a * b / gcd(a,b) 计算最小公倍数
 */
template <typename T, typename U>
constexpr inline auto lcm(const T &x, const U &y) noexcept
{
	return x / gcd<T, U>(x, y) * y;
}

/**
 * @brief 计算多个数的最小公倍数（模板重载版本3）
 *
 * @tparam T 数值类型
 * @tparam Args 可变参数类型包
 * @param x 第一个数
 * @param y 第二个数
 * @param args 其他数
 * @return 返回所有数的最小公倍数
 */
template <typename T, typename... Args>
constexpr inline T lcm(const T &x, const T &y, const Args &...args) noexcept
{
	T re = lcm(x, y);
	return lcm(re, args...);
}

/**
 * @brief 按指定精度对长双精度浮点数进行四舍五入
 *
 * @param x 输入的长双精度浮点数
 * @param prec 要保留的小数位数
 * @return 返回按指定精度四舍五入后的结果
 *
 * 通过将数值乘以10的prec次方，使用std::round进行四舍五入，
 * 再除以10的prec次方得到指定精度的结果
 */
inline long double round(long double x, int prec) noexcept
{
	x *= std::pow(10.0, prec);
	x = std::round(x);
	return x / std::pow(10.0, prec);
}

// ------------------------------------------浮点操作--------------------------------------------------

// 单精度浮点数
/*
 * IEE754标准单精度浮点数
 * N=(−1)^s×2 ^(e−127)×1.f
 **数符s为1位，表示浮点数的正负
 **尾数编码f为23位(采用原码表示)
 **阶码编码e为8位（含1位阶符，采用移码表示，偏移量127）
 * 31   [30:23]   [22,0]
 * s    e         f
 */
struct split_float
{
	bool sign : 1;
	uint8_t E : 8;
	uint32_t f : 23;
};

/**
 * @brief 将IEEE 754单精度浮点数分解为符号位、指数和尾数部分
 *
 * @param x 输入的单精度浮点数
 * @return split_float 包含符号位(sign)、指数(E)和尾数(f)的结构体
 *
 * 该函数使用联合体(union)将浮点数的二进制表示直接解释为32位整数，
 * 然后通过位运算提取符号位、指数和尾数部分，符合IEEE 754标准
 */
constexpr inline split_float split_float_func(float x) noexcept
{
	union
	{
		float f;
		uint32_t i;
	} u = {x}; // 使用联合体访问float的位表示
	return split_float{
		static_cast<bool>(u.i >> 31),	 // 得到第32位
		static_cast<uint8_t>(u.i >> 23), // 得到第32-24位，收缩转换成8位
		u.i								 // 收缩转换成23位
	};
}

// 双精度浮点数
/*
 * IEE754标准双精度浮点数
 * N=(−1)^s×2 ^(e−1023)×1.f
 **数符s为1位，表示浮点数的正负
 **尾数编码f为52位(采用原码表示)
 **阶码编码e为11位（含1位阶符，采用移码表示，偏移量1023）
 * 63   [62:52]   [51,0]
 * s    e         f
 */
struct split_double
{
	bool sign : 1;
	uint16_t E : 11;
	uint64_t f : 52;
};

/**
 * @brief 将IEEE 754双精度浮点数分解为符号位、指数和尾数部分
 *
 * @param x 输入的双精度浮点数
 * @return split_double 包含符号位(sign)、指数(E)和尾数(f)的结构体
 *
 * 该函数使用联合体(union)将双精度浮点数的二进制表示直接解释为64位整数，
 * 然后通过位运算提取符号位、指数和尾数部分，符合IEEE 754标准
 */
constexpr inline split_double split_double_func(double x) noexcept
{
	union
	{
		double d;
		uint64_t i;
	} u = {x};
	return split_double{
		static_cast<bool>(u.i >> 63),
		static_cast<uint16_t>(u.i >> 52),
		u.i};
}

EMBMARTIN_NAMESPACE_END


#endif // EMBMARTIN_FUNCTIONS_H