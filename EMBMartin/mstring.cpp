#include "mstring.h"

EMBMARTIN_NAMESPACE_BEGIN

int atoi(const std::string_view& sv) noexcept
{
    // 1. 跳过前导空白（与 isspace 的六个字符一致）
    std::size_t i = 0;
    while (i < sv.size() &&
           (sv[i] == ' '  || sv[i] == '\t' || sv[i] == '\n' ||
            sv[i] == '\v' || sv[i] == '\f' || sv[i] == '\r'))
    {
        ++i;
    }

    // 2. 处理符号，记录正负后剥离符号，把无符号数字交给 from_chars
    bool negative = false;
    if (i < sv.size() && (sv[i] == '+' || sv[i] == '-'))
    {
        negative = (sv[i] == '-');
        ++i;
    }

    const char* first = sv.data() + i;
    const char* last  = sv.data() + sv.size();

    // 3. 用 unsigned 解析，避免 INT_MIN 的边界问题
    unsigned int uval = 0;
    auto [ptr, ec] = std::from_chars(first, last, uval, 10);

    if (ec == std::errc::invalid_argument)
    {
        return 0;   // 无有效数字，与 atoi 一致
    }

    if (ec == std::errc::result_out_of_range)
    {
        return negative ? INT_MIN : INT_MAX;   // 饱和
    }

    // 4. 范围检查并返回
    if (negative)
    {
        // 支持 INT_MIN == -(INT_MAX + 1)
        if (uval > static_cast<unsigned int>(INT_MAX) + 1u)
            return INT_MIN;
        if (uval == static_cast<unsigned int>(INT_MAX) + 1u)
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