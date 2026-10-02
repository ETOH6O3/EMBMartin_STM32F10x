#include "mstring.h"

EMBMARTIN_NAMESPACE_BEGIN

int atoi(const etl::string_view& sv) noexcept
{
    // 1. 跳过前导空白
    std::size_t i = 0;
    while (i < sv.size() &&
           (sv[i] == ' '  || sv[i] == '\t' || sv[i] == '\n' ||
            sv[i] == '\v' || sv[i] == '\f' || sv[i] == '\r'))
    {
        ++i;
    }

    // 2. 处理符号
    bool negative = false;
    if (i < sv.size() && (sv[i] == '+' || sv[i] == '-'))
    {
        negative = (sv[i] == '-');
        ++i;
    }

    etl::string_view digits = sv.substr(i);

    // 3. 用 unsigned 解析
    auto result = etl::to_arithmetic<unsigned int>(digits, etl::radix::decimal);

    if (!result.has_value())                    // ← 关键修改：has_error() → !has_value()
    {
        if (result.error() == etl::to_arithmetic_status::Overflow)
        {
            return negative ? INT_MIN : INT_MAX;
        }
        return 0;
    }

    // 4. 范围检查并返回
    unsigned int uval = result.value();

    if (negative)
    {
        if (uval >= static_cast<unsigned int>(INT_MAX) + 1u)
            return INT_MIN;
        return -static_cast<int>(uval);
    }
    else
    {
        if (uval > static_cast<unsigned int>(INT_MAX))
            return INT_MAX;
        return static_cast<int>(uval);
    }
}




EMBMARTIN_NAMESPACE_END