#include "fmt.h"

// 旧格式化核心的浮点辅助实现（decimal_exponent / round_digits / increment_decimal /
// make_float_parts）。
//
// 这些函数是旧格式化核心的一部分，随旧核心一起搬进 EMBMartin::legacy_fmt，
// 因此这里只能用作用域限定名 EMBMartin::legacy_fmt::core_detail::X 引用。

namespace EMBMartin
{
	// 必须与 fmt.h 中 legacy_fmt 的**内联性**保持一致，否则 clang 会报
	// -Winline-namespace-reopened-noninline。EMBMARTIN_FMT_INLINE_LEGACY 由 fmt.h 提供。
	EMBMARTIN_FMT_INLINE_LEGACY namespace legacy_fmt
	{
		namespace core_detail
		{
int decimal_exponent(long double value) noexcept
{
    int exponent = 0;
    while (value >= 10.0L)
    {
        value /= 10.0L;
        ++exponent;
    }
    while (value > 0.0L && value < 1.0L)
    {
        value *= 10.0L;
        --exponent;
    }
    return exponent;
}

void round_digits(char *digits, int &length, int keep, char guard) noexcept
{
    if (guard < '5' || keep <= 0)
        return;
    int index = keep - 1;
    while (index >= 0 && digits[index] == '9')
        digits[index--] = '0';
    if (index >= 0)
        digits[index]++;
    else
    {
        for (int i = length; i > 0; --i)
            digits[i] = digits[i - 1];
        digits[0] = '1';
        ++length;
    }
}

void increment_decimal(char *digits) noexcept
{
    int length = static_cast<int>(std::strlen(digits));
    int index = length - 1;
    while (index >= 0 && digits[index] == '9')
        digits[index--] = '0';
    if (index >= 0)
        digits[index]++;
    else
    {
        for (int i = length; i > 0; --i)
            digits[i] = digits[i - 1];
        digits[0] = '1';
    }
}
void make_float_parts(
    long double value, char type, int precision, char *integer, char *fraction, char *exponent) noexcept
{
    if (value == 0.0L)
    {
        integer[0] = '0';
        integer[1] = '\0';
        fraction[0] = '\0';
        exponent[0] = '\0';
        return;
    }

    bool scientific = type == 'e' || type == 'E';
    int exponent_value = decimal_exponent(value);
    if (type == 'g' || type == 'G')
        scientific = exponent_value < -4 || exponent_value >= precision;

    int significant = scientific ? (type == 'g' || type == 'G' ? precision : precision + 1) : 0;
    if (precision > 18)
        precision = 18;
    if (significant > 19)
        significant = 19;

    if (scientific)
    {
        long double scale = std::pow(10.0L, exponent_value);
        long double normalized = value / scale;
        char digits[24]{};
        int length = significant;
        for (int i = 0; i < length; ++i)
        {
            int digit = static_cast<int>(normalized);
            digits[i] = char('0' + digit);
            normalized = (normalized - digit) * 10.0L;
        }
        int guard = static_cast<int>(normalized);
        round_digits(digits, length, significant, char('0' + guard));
        if (length > significant)
        {
            length = significant;
            ++exponent_value;
        }
        integer[0] = digits[0];
        integer[1] = '\0';
        int fraction_length = significant - 1;
        if (type == 'g' || type == 'G')
            while (fraction_length > 0 && digits[fraction_length] == '0')
                --fraction_length;
        for (int i = 0; i < fraction_length; ++i)
            fraction[i] = digits[i + 1];
        fraction[fraction_length] = '\0';
    }
    else
    {
        bool general_type = type == 'g' || type == 'G';
        if (precision <= 0 &&
            (type == 'e' || type == 'E' || general_type))
            precision = 1;
        long double integer_value = std::floor(value);
        int exponent = decimal_exponent(integer_value);
        if (general_type)
        {
            precision -= decimal_exponent(value) + 1;
            if (precision < 0)
                precision = 0;
        }
        long double scale = std::pow(10.0L, exponent);
        int integer_length = exponent + 1;
        if (integer_length < 1)
            integer_length = 1;
        for (int i = 0; i < integer_length && i < 127; ++i)
        {
            int digit = static_cast<int>(integer_value / scale);
            integer[i] = char('0' + digit);
            integer_value -= digit * scale;
            scale /= 10.0L;
        }
        integer[integer_length] = '\0';
        long double fractional = value - std::floor(value);
        char fraction_digits[64]{};
        for (int i = 0; i <= precision; ++i)
        {
            fractional *= 10.0L;
            int digit = static_cast<int>(fractional);
            fraction_digits[i] = char('0' + digit);
            fractional -= digit;
        }
        int guard = fraction_digits[precision] - '0';
        int length = precision;
        if (precision > 0)
        {
            for (int i = 0; i < precision; ++i)
                fraction[i] = fraction_digits[i];
            round_digits(fraction, length, precision, char('0' + guard));
            if (length > precision)
            {
                increment_decimal(integer);
                for (int i = 0; i < precision; ++i)
                    fraction[i] = '0';
            }
        }
        else if (guard >= 5)
        {
            increment_decimal(integer);
        }
        fraction[precision] = '\0';
        if (general_type)
        {
            int length = static_cast<int>(std::strlen(fraction));
            while (length > 0 && fraction[length - 1] == '0')
                fraction[--length] = '\0';
        }
    }

    if (scientific)
    {
        exponent[0] = (exponent_value < 0) ? '-' : '+';
        int absolute = exponent_value < 0 ? -exponent_value : exponent_value;
        exponent[1] = '0' + (absolute / 100) % 10;
        exponent[2] = '0' + (absolute / 10) % 10;
        exponent[3] = '0' + absolute % 10;
        exponent[4] = '\0';
    }
    else
        exponent[0] = '\0';
}
		} // namespace core_detail
	} // namespace legacy_fmt
} // namespace EMBMartin
