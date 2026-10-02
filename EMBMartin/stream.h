/**
 * @file IO.h
 * @author 孙鸣淼
 * @brief 适用于单片机开发的轻量化流操作封装头文件
 * @version 0.1
 *
 */

#ifndef EMBMARTIN_STREAM_H
#define EMBMARTIN_STREAM_H

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <type_traits>

#include "macro.h"
#include "meta.h"
#include "fmt.h"
#include "mstring.h"

EMBMARTIN_NAMESPACE_BEGIN

template <size_t buffer_size = 128>
class OutStream
{
private:
    /**
     * @brief 内部格式化辅助函数
     * @tparam N 格式字符串长度
     * @tparam Args 参数类型包
     * @param fmt_str 格式字符串包装
     * @param args 参数包
     * @return 格式化是否成功
     */
    template <size_t N, typename... Args>
    bool format_to_buffer(const FormatString<N> &fmt_str, Args &&...args) noexcept;

    /**
     * @brief 将缓冲区内容逐字符输出到目标设备，并清空缓冲区
     *
     * 该函数由 OutStream 提供，内部循环调用 output_char，
     * 派生类无需重写。送出后缓冲区被清空，因此重复调用不会重复输出。
     */
    inline void flush() noexcept
    {
        char *out_p = this->buffer;
        while (*out_p != '\0')
        {
            this->output_char(*out_p++);
        }
        this->buffer[0] = '\0';
    }

protected:
    constexpr static size_t _buffer_size = buffer_size;
    char buffer[buffer_size];

    /**
     * @brief 将缓冲区中前 N 个字符输出到目标设备，并清空缓冲区。允许中间有 '\0' 字符
     *
     * @param N 待输出的字符数量
     */
    inline void flush(size_t N) noexcept
    {
        if (N > buffer_size)
        {
            sprintf(this->buffer, "[OutStream][flush]Buffer overflow: %zu > %zu", N, buffer_size);
            this->flush();
        }

        char *out_p = this->buffer;
        for (size_t i = 0; i < N; ++i)
        {
            this->output_char(*out_p++);
        }
        this->buffer[0] = '\0';
    }

    /**
     * @brief 纯虚函数，向目标设备输出一个字符
     *
     * 派生类只需要实现这一个接口即可；格式化、缓冲区管理与批量写出
     * 全部由 OutStream 提供，派生类不必再关心缓冲区。
     *
     * @param c 待输出的字符
     */
    virtual void output_char(char c) noexcept = 0;

public:
    /**
     * @brief C 风格格式化输出函数，将格式化字符串写入缓冲区并输出
     *
     * @tparam Args 可变参数包类型
     * @param c 格式化字符串
     * @param args 可变参数包
     */
    template <typename... Args>
    void printf(const char c[], Args... args) noexcept;

    /**
     * \defgroup show_funcs
     * python 风格 print 函数族
     * 提供简化的显示接口，支持多种数据类型的显示
     * @{
     */

    inline void show(const char &c) noexcept
    {
        this->buffer[0] = c;
        this->buffer[1] = '\0';
        this->flush();
    }

    inline void show(const char str[]) noexcept
    {
        strncpy(this->buffer, str, _buffer_size - 1);
        this->buffer[_buffer_size - 1] = '\0';
        this->flush();
    }

    inline void show(const std::string_view &str) noexcept
    {
        const size_t length = str.size() < _buffer_size - 1 ? str.size() : _buffer_size - 1;
        memcpy(this->buffer, str.data(), length);
        this->buffer[length] = '\0';
        this->flush();
    }

    template <typename T,
              typename = std::enable_if_t<
                  std::is_integral_v<T> || std::is_floating_point_v<T>>>
    inline void show(T number) noexcept
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            // 与 C 习惯一致：输出 0 / 1
            sprintf(this->buffer, "%d", static_cast<int>(number));
        }
        else if constexpr (std::is_same_v<T, signed char>)
        {
            // %hhd 期望 int，signed char 会默认提升为 int
            sprintf(this->buffer, "%hhd", number);
        }
        else if constexpr (std::is_same_v<T, unsigned char>)
        {
            sprintf(this->buffer, "%hhu", number);
        }
        else if constexpr (std::is_same_v<T, short>)
        {
            sprintf(this->buffer, "%hd", number);
        }
        else if constexpr (std::is_same_v<T, unsigned short>)
        {
            sprintf(this->buffer, "%hu", number);
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            sprintf(this->buffer, "%d", number);
        }
        else if constexpr (std::is_same_v<T, unsigned int>)
        {
            sprintf(this->buffer, "%u", number);
        }
        else if constexpr (std::is_same_v<T, long>)
        {
            sprintf(this->buffer, "%ld", number);
        }
        else if constexpr (std::is_same_v<T, unsigned long>)
        {
            sprintf(this->buffer, "%lu", number);
        }
        else if constexpr (std::is_same_v<T, long long>)
        {
            sprintf(this->buffer, "%lld", number);
        }
        else if constexpr (std::is_same_v<T, unsigned long long>)
        {
            sprintf(this->buffer, "%llu", number);
        }
        else if constexpr (std::is_same_v<T, wchar_t> ||
                           std::is_same_v<T, char16_t> ||
                           std::is_same_v<T, char32_t>
#if defined(__cpp_char8_t)
                           || std::is_same_v<T, char8_t>
#endif
        )
        {
            // 按底层整数输出，避免误当成字符处理
            if constexpr (std::is_signed_v<T>)
            {
                sprintf(this->buffer, "%lld", static_cast<long long>(number));
            }
            else
            {
                sprintf(this->buffer, "%llu", static_cast<unsigned long long>(number));
            }
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            if constexpr (std::is_same_v<T, long double>)
            {
                sprintf(this->buffer, "%Lg", number);
            }
            else
            {
                // float 会默认提升为 double，%g 期望 double
                sprintf(this->buffer, "%g", number);
            }
        }
        else
        {
            static_assert(sizeof(T) == 0, "show(): unsupported type");
        }

        this->flush();
    }

    template <typename T, typename = std::enable_if_t<has_iterator_v<T>>>
    inline void show(const T &container) noexcept
    {
        for (const auto &item : container)
        {
            this->show(item);
        }
    }

    /**
     * @brief 空参数的show函数，用于适配可变参数模板
     */
    inline void show() noexcept {}

    /**
     * @brief 链式显示多个数据
     * @tparam T 第一个参数的类型
     * @tparam Args 剩余参数的类型包
     * @param first 第一个参数
     * @param args 剩余参数
     */
    template <typename T, typename... Args>
    void show(const T &first, const Args &...args) noexcept;

    /**
     * @brief 显示参数，最后显示 \\n
     * @tparam Args 参数包类型
     * @param args 可变参数包，用于显示的内容
     */
    template <typename... Args>
    void showln(const Args &...args) noexcept;

    /**
     * @brief 显示参数，最后显示 \\r
     * @tparam Args 参数包类型
     * @param args 可变参数包，用于显示的内容
     *
     */
    template <typename... Args>
    void showlr(const Args &...args) noexcept;

    /**
     * @brief 显示参数，参数间以空格分隔
     * @tparam T 第一个参数的类型
     * @tparam Args 剩余参数的类型包
     * @param first 第一个参数
     * @param args 剩余参数
     */
    template <char sep = ' ', typename T, typename... Args>
    void showsep(const T &first, const Args &...args) noexcept;

    /** @} */ // end of show_funcs

    /**
     * \defgroup print_funcs
     * C++23 风格 print 函数族
     * 提供安全高效的格式化输出方案
     * @{
     */

    /**
     * @brief C++23风格格式化输出，类似于std::print
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串
     * @param args 参数包
     */
    template <typename... Args>
    void print(std::string_view fmt, Args &&...args) noexcept;

    /**
     * @brief C++23风格格式化输出，支持字符串字面量
     * @tparam N 字符串长度
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串字面量
     * @param args 参数包
     */
    template <size_t N, typename... Args>
    void print(const char (&fmt)[N], Args &&...args) noexcept;

    /**
     * @brief C++23风格格式化输出并换行，类似于std::println
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串
     * @param args 参数包
     */
    template <typename... Args>
    void println(std::string_view fmt, Args &&...args) noexcept;

    /**
     * @brief C++23风格格式化输出并换行，支持字符串字面量
     * @tparam N 字符串长度
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串字面量
     * @param args 参数包
     */
    template <size_t N, typename... Args>
    void println(const char (&fmt)[N], Args &&...args) noexcept;

    /** @} */ // end of print_funcs
};

template <size_t buffer_size = 128>
class InStream
{
protected:
    constexpr static size_t _buffer_size = buffer_size;
    char buffer[buffer_size];

    template <typename T>
    static T *scan_argument(T &value) noexcept
    {
        return &value;
    }

    template <typename T, size_t N>
    static T *scan_argument(T (&value)[N]) noexcept
    {
        return value;
    }
    /**
     * @brief 纯虚函数，用于将目标设备内容读入到缓冲区
     *
     */
    virtual void read() noexcept = 0;

    /**
     * @brief 允许试探性读取的 read 。
     * 
     * @param tentative 若为真，下次调用时不会刷新 buffer
     */
    void __read(bool tentative = false)noexcept
    {
        static bool __last_used = true;
        if (__last_used)
        {
            this->read();
        }
        __last_used = !tentative;
        
    }

public:
    /**
     * @brief C 风格格式化输入函数
     *
     * @tparam Args 可变参数包类型
     * @param c 格式化字符串
     * @param args 可变参数指针包
     */
    template <typename... Args>
    void scanf(const char c[], Args *...args) noexcept;

    /**
     * @brief 现代化的 C 风格格式化输入函数
     *
     * @tparam Args 可变参数包类型
     * @param c 格式化字符串
     * @param args 可变万能引用参数包，支持左值变量和右值
     */
    template <typename... Args>
    void scanf(const char c[], Args &&...args) noexcept;

    /**
     * @brief 读取输入并返回编译期确定类型的元组
     *
     * 示例：auto [x, y] = console.scan<int, int>("%d %d");
     * @tparam Args 元组元素类型
     * @tparam N 格式字符串长度
     * @param c sscanf 格式字符串
     * @return 包含读取结果的 std::tuple<Args...>
     */
    template <typename... Args, size_t N>
    std::tuple<Args...> scan(const char (&c)[N]) noexcept;

    /**
     * @brief 读取一行字符串，直到遇到换行符或缓冲区满
     * @param src 源字符串，格式化输入使用的格式字符串
     * @param trg 目标字符串，存储读取到的内容
     */
    void getline(char trg[]) noexcept;

    /**
     * @brief 读取一行字符串，直到遇到换行符或缓冲区满，并返回一个 String 对象
     *
     */
    String<buffer_size> getline() noexcept;
};

template <size_t buffer_size = 128>
class IOStream : public OutStream<buffer_size>, public InStream<buffer_size>
{
protected:
    inline void update() noexcept
    {
        this->flush();
        this->__read();
    }

public:
    /**
     * @brief 从输入流中获取用户输入
     * @param prompt 提示信息字符串，显示给用户
     * @param trg 用于存储输入数据的目标字符数组
     * @return 无返回值
     */
    inline void input(const char *prompt, const char *trg) noexcept
    {
        this->printf("%s", prompt);
        this->getline(trg);
        return;
    }

    /**
     * @brief 从输入流中获取用户输入，以定长字符串返回
     * @param prompt 提示信息字符串，显示给用户
     * @return 定长字符串对象，包含用户输入的数据
     */
    inline String<buffer_size> input(const char *prompt) noexcept
    {
        this->printf("%s", prompt);
        return this->getline();
    }
};

template <size_t buffer_size>
template <size_t N, typename... Args>
bool OutStream<buffer_size>::format_to_buffer(const FormatString<N> &fmt_str, Args &&...args) noexcept
{
    // 使用我们实现的format_to函数格式化到缓冲区
    int result = format_to(this->buffer, _buffer_size, fmt_str, std::forward<Args>(args)...);

    if (result >= 0)
    {
        // // 确保以null结尾
        // size_t length = static_cast<size_t>(result);
        // if (length < _buffer_size) {
        //     this->buffer[length] = '\0';
        // } else if (_buffer_size > 0) {
        //     this->buffer[_buffer_size - 1] = '\0';
        // }
        return true;
    }

    // 格式化失败，使用回退方案
    const char *error_msg = "<format error>";
    size_t error_len = strlen(error_msg);
    size_t copy_len = error_len < _buffer_size ? error_len : _buffer_size - 1;

    strncpy(this->buffer, error_msg, copy_len);
    this->buffer[copy_len] = '\0';

    return false;
}

template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::printf(const char c[], Args... args) noexcept
{
    if constexpr (sizeof...(args) == 0)
    {
        // 无参数：把格式字符串按普通字符串写入，避免 format-security
        std::sprintf(this->buffer, "%s", c);
    }
    else
    {
        // 有参数：安全格式化到固定缓冲区
        std::sprintf(this->buffer, c, args...);
    }
    this->flush();
}

template <size_t buffer_size>
template <typename T, typename... Args>
void OutStream<buffer_size>::show(const T &first, const Args &...args) noexcept
{
    show(first);
    show(args...);
}

template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::showln(const Args &...args) noexcept
{
    show(args...);
    show('\n');
}

template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::showlr(const Args &...args) noexcept
{
    show(args...);
    show('\r');
}

template <size_t buffer_size>
template <char sep, typename T, typename... Args>
void OutStream<buffer_size>::showsep(const T &first, const Args &...args) noexcept
{
    show(first);

    if constexpr (sizeof...(args) >= 1)
    {
        show(sep);
        showsep(args...);
    }
}

template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::print(std::string_view fmt, Args &&...args) noexcept
{
    // 创建临时字符串用于构造FormatString
    char temp[256];
    size_t copy_len = fmt.size() < sizeof(temp) - 1 ? fmt.size() : sizeof(temp) - 1;
    strncpy(temp, fmt.data(), copy_len);
    temp[copy_len] = '\0';

    // 使用字符串字面量重载
    if constexpr (sizeof...(args) == 0)
    {
        // 无参数，直接输出字符串
        strncpy(this->buffer, temp, _buffer_size - 1);
        this->buffer[_buffer_size - 1] = '\0';
    }
    else
    {
        // 有参数，使用格式化
        this->print(temp, std::forward<Args>(args)...);
    }
    this->flush();
}

template <size_t buffer_size>
template <size_t N, typename... Args>
void OutStream<buffer_size>::print(const char (&fmt)[N], Args &&...args) noexcept
{
    if constexpr (sizeof...(args) == 0)
    {
        // 无参数，直接输出字符串
        strncpy(this->buffer, fmt, _buffer_size - 1);
        this->buffer[_buffer_size - 1] = '\0';
        this->flush();
    }
    else
    {
        // 使用格式化
        FormatString<N> fmt_str(fmt);
        if (this->format_to_buffer(fmt_str, std::forward<Args>(args)...))
        {
            this->flush();
        }
        else
        {
            // 格式化失败，但仍然发送错误信息
            this->flush();
        }
    }
}

template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::println(std::string_view fmt, Args &&...args) noexcept
{
    this->print(fmt, std::forward<Args>(args)...);
    this->show('\n');
}

template <size_t buffer_size>
template <size_t N, typename... Args>
void OutStream<buffer_size>::println(const char (&fmt)[N], Args &&...args) noexcept
{
    this->print(fmt, std::forward<Args>(args)...);
    this->show('\n');
}

template <size_t buffer_size>
template <typename... Args>
void InStream<buffer_size>::scanf(const char c[], Args *...args) noexcept
{
    this->__read();
    sscanf(this->buffer, c, args...);
}

template <size_t buffer_size>
template <typename... Args>
void InStream<buffer_size>::scanf(const char c[], Args &&...args) noexcept
{
    this->__read();
    sscanf(this->buffer, c, &args...);
}

template <size_t buffer_size>
template <typename... Args, size_t N>
std::tuple<Args...> InStream<buffer_size>::scan(const char (&c)[N]) noexcept
{
    this->__read();
    std::tuple<Args...> result{};

    std::apply(
        [this, &c](auto &...args)
        {
            sscanf(this->buffer, c, scan_argument(args)...);
        },
        result);

    return result;
}

template <size_t buffer_size>
void InStream<buffer_size>::getline(char trg[]) noexcept
{
    this->__read();
    char *buff_p = this->buffer;
    char *trg_p = trg;
    while ((*buff_p != '\r') && (*buff_p != '\n') && (*buff_p != '\0') && ((buff_p - this->buffer) < static_cast<ptrdiff_t>(_buffer_size - 1)))
    {
        *(trg_p++) = *(buff_p++);
    }
    *trg_p = '\0';
}

template <size_t buffer_size>
String<buffer_size> InStream<buffer_size>::getline() noexcept
{
    this->__read();

    String<buffer_size> result;
    char *dest = result.data_mutable();

    size_t length = 0;
    while (length < _buffer_size - 1 &&
           this->buffer[length] != '\0' &&
           this->buffer[length] != '\r' &&
           this->buffer[length] != '\n')
    {
        dest[length] = this->buffer[length];
        ++length;
    }

    result.commit(length);
    return result;
}

EMBMARTIN_NAMESPACE_END

// debug 控制台
EMBMARTIN_DEBUGING_EXTERN_CONSOLE;

#endif // EMBMARTIN_STREAM_H