/**
 * @file debugger.h
 * @author 孙鸣淼
 * @brief 增强版流式通讯，提供参数包支持 （见 SERIAL_TOOL.md）
 * @version 0.1
 * @date 2026-09-25
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "stream.h"
#include "inspect.h"

EMBMARTIN_NAMESPACE_BEGIN

enum class LogLevel
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

template <size_t buffer_size = 128>
class Debugger : public IOStream<buffer_size>
{
private:
    LogLevel log_level = LogLevel::INFO;
    const char *file_name = "";
    bool output_to_console = true;

    template <typename... Args>
    void send_log(LogLevel level, bool output_to_console, const char *file_name, Args &&...args) noexcept;

public:
    inline Debugger() noexcept = default;
    inline void set_params(LogLevel level, const char *file_name = "", bool output_to_console = true) noexcept
    {
        this->log_level = level;
        this->output_to_console = output_to_console;
        this->file_name = file_name;
    }
    inline auto get_params() noexcept
    {
        return std::make_tuple(this->log_level, this->output_to_console, this->file_name);
    }

    template <typename... Args>
    inline void send_to_console(Args &&...args) noexcept
    {
        this->show("\x1B\x1E[Console][");
        this->print(std::forward<decltype(args)>(args)...);
        this->show("]\n");
    }

    template <typename... Args>
    inline void send_log(LogLevel level, Args &&...args) noexcept
    {
        return this->send_log(level, this->output_to_console, this->file_name, std::forward<decltype(args)>(args)...);
    }

    template <typename __Struct>
    void send_struct(const __Struct &data, const char *var_name) noexcept;
};

EMBMARTIN_NAMESPACE_END

template <std::size_t buffer_size>
template <class... Args>
void EMBMartin::Debugger<buffer_size>::send_log(
    EMBMartin::LogLevel level, bool output_to_console, const char *file_name, Args &&...args) noexcept
{
    if (level >= log_level)
    {
        this->show("\x1B\x1E[Log][");
        this->show(enum_name<0, 6>(level));
        this->show("][");
        this->show(output_to_console ? "1" : "0");
        this->show("][");
        this->show(file_name);
        this->show("][");
        this->print(std::forward<decltype(args)>(args)...);
        this->show("]\n");
    }
}
template <std::size_t buffer_size>
template <class __Struct>
void EMBMartin::Debugger<buffer_size>::send_struct(
    const __Struct &data, const char *var_name) noexcept
{
    this->show("\x1B\x1E[Struct][");
    this->show(type_name<__Struct>());
    this->show("][");
    this->show(var_name);
    this->show("][");
    std::string_view sv(reinterpret_cast<const char *>(&data), sizeof(data));
    this->print("{:s}", sv);
    this->show("]\n");
}