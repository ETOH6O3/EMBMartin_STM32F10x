/**
 * @file IO.h
 * @author 孙鸣淼
 * @brief 适用于单片机开发的轻量化流操作封装头文件
 * @version 0.1
 *
 */

#ifndef EMBMARTIN_IO_H
#define EMBMARTIN_IO_H

#include <cstddef>
#include <cstdio>
#include <cstring>

#include "macro.h"
#include "meta.h"

EMBMARTIN_NAMESPACE_BEGIN

template <size_t buffer_size = 128>
class OutStream
{
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
    void printf(const char c[], Args... args) noexcept
    {
        sprintf(this->buffer, c, args...);
        this->send();
    }

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

    template <typename T>
    inline void show(const T &number) noexcept
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
        else if constexpr (has_iterator_v<T>)
        {
            for (const auto &item : number)
            {
                this->show(item);
            }
        }
        else
        {
            static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
                          "Only integral and floating point types are supported");
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
    void show(const T &first, const Args &...args) noexcept
    {
        show(first);
        show(args...);
    }

    /**
     * @brief 显示参数，最后显示 \\n
     * @tparam Args 参数包类型
     * @param args 可变参数包，用于显示的内容
     */
    template <typename... Args>
    void showln(const Args &...args) noexcept
    {
        show(args...);
        show('\n');
    }

    /**
     * @brief 显示参数，最后显示 \\r
     * @tparam Args 参数包类型
     * @param args 可变参数包，用于显示的内容
     *
     */
    template <typename... Args>
    void showlr(const Args &...args) noexcept
    {
        show(args...);
        show('\r');
    }
    /**
     * @brief 显示参数，参数间以空格分隔
     * @tparam T 第一个参数的类型
     * @tparam Args 剩余参数的类型包
     * @param first 第一个参数
     * @param args 剩余参数
     */
    template <typename T, typename... Args>
    void showsep(const T &first, const Args &...args) noexcept
    {
        show(first);

        if constexpr (sizeof...(args) > 1)
        {
            show(' ');
            showsep(args...);
        }
    }

    /** @} */ // end of show_funcs
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
    void scanf(const char c[], Args *...args) noexcept
    {
        this->read();
        sscanf(this->buffer, c, args...);
    }

    /**
     * @brief 现代化的的 C 风格格式化输入函数
     *
     * @tparam Args 可变参数包类型
     * @param c 格式化字符串
     * @param args 可变万能引用参数包，支持左值变量和右值
     */
    template <typename... Args>
    void scanf(const char c[], Args &&...args) noexcept
    {
        this->read();
        sscanf(this->buffer, c, &args...);
    }

    /**
     * @brief 读取一行字符串，直到遇到换行符或缓冲区满
     * @param src 源字符串，格式化输入使用的格式字符串
     * @param trg 目标字符串，存储读取到的内容
     */
    void getline(char trg[]) noexcept
    {
        this->read();
        char *buff_p = this->buffer;
        char *trg_p = trg;
        while ((*buff_p != '\n') && (*buff_p != '\0') && ((buff_p - this->buffer) < static_cast<ptrdiff_t>(_buffer_size - 1)))
        {
            *(trg_p++) = *(buff_p++);
        }
        *trg_p = '\0';
    }
};

template <size_t buffer_size = 128>
class IOStream : public OutStream<buffer_size>, public InStream<buffer_size>
{
public:
    inline void update() noexcept
    {
        this->send();
        this->read();
    }
};

EMBMARTIN_NAMESPACE_END

#endif // EMBMARTIN_IO_H