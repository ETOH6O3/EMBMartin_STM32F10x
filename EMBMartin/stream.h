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

#include "macro.h"
#include "meta.h"
#include "fmt.h"

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
    bool format_to_buffer(const FormatString<N>& fmt_str, Args&&... args) noexcept;

protected:
    constexpr static size_t _buffer_size = buffer_size;
    char buffer[buffer_size];

public:
    /**
     * @brief 纯虚函数，用于将缓冲区内容输出到目标设备
     *
     */
    virtual void send() noexcept = 0;

    /**
     * @brief C 风格格式化输出函数，将格式化字符串写入缓冲区并调用 send 函数
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
        this->send();
    }

    inline void show(const char str[]) noexcept
    {
        strcpy(this->buffer, str);
        this->send();
    }

    inline void show(const std::string_view& str) noexcept
    {
        for(int i = 0; i < str.size(); i++)
        {
            this->buffer[i] = str[i];
        }
        this->send();
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
    inline void show(T number) noexcept
    {
        if constexpr (std::is_integral_v<T>)
        {
            sprintf(this->buffer, "%d", number);
            this->send();
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            sprintf(this->buffer, "%g", number);
            this->send();
        }
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
    template <typename T, typename... Args>
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
    void print(std::string_view fmt, Args&&... args) noexcept;

    /**
     * @brief C++23风格格式化输出，支持字符串字面量
     * @tparam N 字符串长度
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串字面量
     * @param args 参数包
     */
    template <size_t N, typename... Args>
    void print(const char (&fmt)[N], Args&&... args) noexcept;

    /**
     * @brief C++23风格格式化输出并换行，类似于std::println
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串
     * @param args 参数包
     */
    template <typename... Args>
    void println(std::string_view fmt, Args&&... args) noexcept;

    /**
     * @brief C++23风格格式化输出并换行，支持字符串字面量
     * @tparam N 字符串长度
     * @tparam Args 参数类型包
     * @param fmt 格式化字符串字面量
     * @param args 参数包
     */
    template <size_t N, typename... Args>
    void println(const char (&fmt)[N], Args&&... args) noexcept;

    /** @} */ // end of print_funcs

};


template <size_t buffer_size = 128>
class InStream
{
protected:
    constexpr static size_t _buffer_size = buffer_size;
    char buffer[buffer_size];

public:
    /**
     * @brief 纯虚函数，用于将目标设备内容读入到缓冲区
     *
     */
    virtual void read() noexcept = 0;

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
     * @brief 现代化的的 C 风格格式化输入函数
     *
     * @tparam Args 可变参数包类型
     * @param c 格式化字符串
     * @param args 可变万能引用参数包，支持左值变量和右值
     */
    template <typename... Args>
    void scanf(const char c[], Args &&...args) noexcept;

    /**
     * @brief 读取一行字符串，直到遇到换行符或缓冲区满
     * @param src 源字符串，格式化输入使用的格式字符串
     * @param trg 目标字符串，存储读取到的内容
     */
    void getline(char trg[]) noexcept;
};

template <size_t buffer_size = 128>
class IOStream : public OutStream<buffer_size>, public InStream<buffer_size>
{
protected:
    inline void update() noexcept
    {
        this->send();
        this->read();
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
        this->scanf("%s", trg);
        return;
    }
};

template <size_t buffer_size>
template <size_t N, typename... Args>
bool OutStream<buffer_size>::format_to_buffer(const FormatString<N>& fmt_str, Args&&... args) noexcept
{
    // 使用我们实现的format_to函数格式化到缓冲区
    int result = format_to(this->buffer, _buffer_size, fmt_str, std::forward<Args>(args)...);
    
    if (result >= 0) {
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
    const char* error_msg = "<format error>";
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
    this->send();
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
template <typename T, typename... Args>
void OutStream<buffer_size>::showsep(const T &first, const Args &...args) noexcept
{
    show(first);

    if constexpr (sizeof...(args) >= 1)
    {
        show(' ');
        showsep(args...);
    }
}


template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::print(std::string_view fmt, Args&&... args) noexcept
{
    // 创建临时字符串用于构造FormatString
    char temp[256];
    size_t copy_len = fmt.size() < sizeof(temp) - 1 ? fmt.size() : sizeof(temp) - 1;
    strncpy(temp, fmt.data(), copy_len);
    temp[copy_len] = '\0';
    
    // 使用字符串字面量重载
    if constexpr (sizeof...(args) == 0) {
        // 无参数，直接输出字符串
        strncpy(this->buffer, temp, _buffer_size - 1);
        this->buffer[_buffer_size - 1] = '\0';
    } else {
        // 有参数，使用格式化
        this->print(temp, std::forward<Args>(args)...);
    }
    this->send();
}

template <size_t buffer_size>
template <size_t N, typename... Args>
void OutStream<buffer_size>::print(const char (&fmt)[N], Args&&... args) noexcept
{
    if constexpr (sizeof...(args) == 0) {
        // 无参数，直接输出字符串
        strncpy(this->buffer, fmt, _buffer_size - 1);
        this->buffer[_buffer_size - 1] = '\0';
        this->send();
    } else {
        // 使用格式化
        FormatString<N> fmt_str(fmt);
        if (this->format_to_buffer(fmt_str, std::forward<Args>(args)...)) {
            this->send();
        } else {
            // 格式化失败，但仍然发送错误信息
            this->send();
        }
    }
}

template <size_t buffer_size>
template <typename... Args>
void OutStream<buffer_size>::println(std::string_view fmt, Args&&... args) noexcept
{
    this->print(fmt, std::forward<Args>(args)...);
    this->show('\n');
}

template <size_t buffer_size>
template <size_t N, typename... Args>
void OutStream<buffer_size>::println(const char (&fmt)[N], Args&&... args) noexcept
{
    this->print(fmt, std::forward<Args>(args)...);
    this->show('\n');
}


template <size_t buffer_size>
template <typename... Args>
void InStream<buffer_size>::scanf(const char c[], Args *...args) noexcept
{
    this->read();
    sscanf(this->buffer, c, args...);
}

template <size_t buffer_size>
template <typename... Args>
void InStream<buffer_size>::scanf(const char c[], Args &&...args) noexcept
{
    this->read();
    sscanf(this->buffer, c, &args...);
}

template <size_t buffer_size>
void InStream<buffer_size>::getline(char trg[]) noexcept
{
    this->read();
    char *buff_p = this->buffer;
    char *trg_p = trg;
    while ((*buff_p != '\r') && (*buff_p != '\n') && (*buff_p != '\0') && ((buff_p - this->buffer) < static_cast<ptrdiff_t>(_buffer_size - 1)))
    {
        *(trg_p++) = *(buff_p++);
    }
    *trg_p = '\0';
}

EMBMARTIN_NAMESPACE_END

// debug 控制台
EMBMARTIN_DEBUGING_EXTERN_CONSOLE;

#endif // EMBMARTIN_STREAM_H