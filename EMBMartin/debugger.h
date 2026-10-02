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

#include <etl/unordered_map.h>

#include "stream.h"
#include "inspect.h"
#include "coordinate.h"

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
class MartinDebugger : public IOStream<buffer_size>
{
private:
    LogLevel log_level = LogLevel::INFO;
    const char *file_name = "";
    bool output_to_console = true;

    template <typename... Args>
    void send_log(LogLevel level, bool output_to_console, const char *file_name, Args &&...args) noexcept;

public:
    inline MartinDebugger() noexcept = default;
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

template <size_t buffer_size = 128>
using Debugger = MartinDebugger<buffer_size>;

template <uint8_t FONT_SIZE = 18, uint16_t SCREEN_X_PIXELS = 300, uint16_t SCREEN_Y_PIXELS = 200,
          uint8_t MAX_SLIDER_IDX = 32, typename SLIDER_PARAM_T = int8_t, typename JOYSTICK_PARAM_T = int8_t, size_t buffer_size = 128>
class JiangXieDebugger : public IOStream<buffer_size>
{
private:
    static_assert(FONT_SIZE >= 10, "FONT_SIZE must be not less than 10");
    constexpr static uint8_t __MAX_X_COORD = SCREEN_X_PIXELS * 5 / FONT_SIZE / 3;
    constexpr static uint8_t __MAX_Y_COORD = SCREEN_Y_PIXELS / FONT_SIZE;
    constexpr static double __FONT_WIDTH = FONT_SIZE * 0.6;
    constexpr static double __FONT_HEIGHT = FONT_SIZE * 1.0;

    Coordinate<uint8_t, 0, __MAX_X_COORD - 1, 0, __MAX_Y_COORD - 1> __coord = {0u, 0u};

    String<buffer_size> common_content = {};

    // SLIDER_PARAM_T __slider_params[MAX_SLIDER_IDX] = {[0 ... MAX_SLIDER_IDX - 1] = 100}; // 需开启 C99 扩展支持
    etl::unordered_map<String<32>, SLIDER_PARAM_T, MAX_SLIDER_IDX, MAX_SLIDER_IDX, std::hash<std::string_view>> __slider_params = {};

    using __JOYSTICK_PARAMS_T = std::pair<JOYSTICK_PARAM_T, JOYSTICK_PARAM_T>;
    std::pair<__JOYSTICK_PARAMS_T, __JOYSTICK_PARAMS_T> __joysticks_params;

    template <typename... Args>
    inline void _display(uint16_t x, uint16_t y, const std::string_view &str) noexcept
    {
        this->show("[display,");
        this->show(x);
        this->show(",");
        this->show(y);
        this->show(",");
        this->show(str);
        this->show(",");
        this->show(FONT_SIZE);
        this->show("]\n");
    }

    /**
     * @brief 解析数据包至类的成员
     *
     * @param pkg_content 数据包内的内容（不含 [] ）
     */
    String<32> __parse_pkg_content(const std::string_view &pkg_content) noexcept;

public:
    template <typename... Args>
    void display(Args &&...args) noexcept;

    inline void set_display_coord(uint8_t x, uint8_t y) noexcept
    {
        __coord.x = x;
        __coord.y = y;
    }

    inline void display_clear() noexcept
    {
        this->show("[display-clear]\n");
        __coord.x = 0;
        __coord.y = 0;
    }

    template <typename... Args>
    inline void plot(Args &&...values) noexcept
    {
        static_assert(sizeof...(values) >= 1, "plot() requires at least one value");
        static_assert(sizeof...(values) <= 10, "plot() supports up to 10 values");
        this->show("[plot,");
        this->template showsep<','>(std::forward<Args>(values)...);
        this->show("]\n");
    }

    inline void plot_clear() noexcept
    {
        this->show("[plot-clear]\n");
    }

    /**
     * @brief 解析数据包，更新滑块和摇杆参数
     *
     * @return auto 标准库元组，普通值 + 当前滑块参数数组 + 当前摇杆参数对
     */
    auto parse() noexcept;
};

EMBMARTIN_NAMESPACE_END

template <std::size_t buffer_size>
template <class... Args>
void EMBMartin::MartinDebugger<buffer_size>::send_log(
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
void EMBMartin::MartinDebugger<buffer_size>::send_struct(
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

template <std::uint8_t FONT_SIZE, std::uint16_t SCREEN_X_PIXELS, std::uint16_t SCREEN_Y_PIXELS,
          uint8_t MAX_SLIDER_IDX, typename SLIDER_PARAM_T, typename JOYSTICK_PARAM_T, std::size_t buffer_size>
template <class... Args>
void EMBMartin::JiangXieDebugger<FONT_SIZE, SCREEN_X_PIXELS, SCREEN_Y_PIXELS,
                                 MAX_SLIDER_IDX, SLIDER_PARAM_T, JOYSTICK_PARAM_T, buffer_size>::display(Args &&...args) noexcept
{
    String<buffer_size> str;
    str.format(std::forward<decltype(args)>(args)...);

    auto begin_it = str.begin();
    auto coord_temp = __coord;

    for (auto it = str.begin(); it != str.end(); ++it)
    {
        if (*it == '\n' || *it == '\r' || *it == '\t')
        {
            if (begin_it != it)
            {
                this->_display(coord_temp.x * __FONT_WIDTH,
                               coord_temp.y * __FONT_HEIGHT,
                               std::string_view(begin_it, it - begin_it));
            }

            if (*it == '\n')
            {
                __coord.x = 0;
                __coord.assignment_add_y(1);
            }
            else if (*it == '\r')
            {
                __coord.x = 0;
            }
            else // '\t'
            {
                __coord += 4 - __coord.x % 4;
            }

            begin_it = it + 1;
            coord_temp = __coord;
        }
        else
        {
            __coord++; // 若需支持 UTF-8，这里要按码点/显示宽度累加
        }
    }

    if (begin_it != str.end())
    {
        this->_display(coord_temp.x * __FONT_WIDTH,
                       coord_temp.y * __FONT_HEIGHT,
                       std::string_view(begin_it, str.end() - begin_it));
    }
}

template <std::uint8_t FONT_SIZE, std::uint16_t SCREEN_X_PIXELS, std::uint16_t SCREEN_Y_PIXELS,
          uint8_t MAX_SLIDER_IDX, typename SLIDER_PARAM_T, typename JOYSTICK_PARAM_T, std::size_t buffer_size>
EMBMartin::String<32> EMBMartin::JiangXieDebugger<FONT_SIZE, SCREEN_X_PIXELS, SCREEN_Y_PIXELS,
                                                  MAX_SLIDER_IDX, SLIDER_PARAM_T, JOYSTICK_PARAM_T, buffer_size>::__parse_pkg_content(const std::string_view &pkg_content) noexcept
{
    const auto class_name_end_pos = pkg_content.find(',');
    if (class_name_end_pos == String<32>::npos)
    {
        return EMBMartin::String<8>("[") + pkg_content + ']'; // 退回
    }
    const auto class_name = pkg_content.substr(0, class_name_end_pos);

    if (class_name == "slider")
    {
        const auto slider_index_end_pos = pkg_content.find(',', class_name_end_pos + 1);
        const auto slider_index_str = pkg_content.substr(class_name_end_pos + 1, slider_index_end_pos - class_name_end_pos - 1);

        const auto slider_value_str = pkg_content.substr(slider_index_end_pos + 1);
        const auto slider_value = static_cast<SLIDER_PARAM_T>(atoi(slider_value_str));

        __slider_params[slider_index_str] = slider_value;

        return "";
    }
    else if (class_name == "joystick")
    {
        // 格式：[joystick, left_x, left_y, right_x, right_y]
        const auto left_x_end_pos = pkg_content.find(',', class_name_end_pos + 1);
        const auto left_x_str = pkg_content.substr(class_name_end_pos + 1, left_x_end_pos - class_name_end_pos - 1);
        const auto left_x = static_cast<JOYSTICK_PARAM_T>(atoi(left_x_str));
        const auto left_y_end_pos = pkg_content.find(',', left_x_end_pos + 1);
        const auto left_y_str = pkg_content.substr(left_x_end_pos + 1, left_y_end_pos - left_x_end_pos - 1);
        const auto left_y = static_cast<JOYSTICK_PARAM_T>(atoi(left_y_str));
        const auto right_x_end_pos = pkg_content.find(',', left_y_end_pos + 1);
        const auto right_x_str = pkg_content.substr(left_y_end_pos + 1, right_x_end_pos - left_y_end_pos - 1);
        const auto right_x = static_cast<JOYSTICK_PARAM_T>(atoi(right_x_str));
        const auto right_y_end_pos = pkg_content.find(',', right_x_end_pos + 1);
        const auto right_y_str = pkg_content.substr(right_x_end_pos + 1, right_y_end_pos - right_x_end_pos - 1);
        const auto right_y = static_cast<JOYSTICK_PARAM_T>(atoi(right_y_str));

        __joysticks_params.first.first = left_x;
        __joysticks_params.first.second = left_y;
        __joysticks_params.second.first = right_x;
        __joysticks_params.second.second = right_y;

        return "";
    }
    else
    {
        return EMBMartin::String<8>("[") + pkg_content + ']'; // 退回
    }
}

template <std::uint8_t FONT_SIZE, std::uint16_t SCREEN_X_PIXELS, std::uint16_t SCREEN_Y_PIXELS,
          uint8_t MAX_SLIDER_IDX, typename SLIDER_PARAM_T, typename JOYSTICK_PARAM_T, std::size_t buffer_size>
auto EMBMartin::JiangXieDebugger<FONT_SIZE, SCREEN_X_PIXELS, SCREEN_Y_PIXELS,
                                 MAX_SLIDER_IDX, SLIDER_PARAM_T, JOYSTICK_PARAM_T, buffer_size>::parse() noexcept
{
    const String<buffer_size> line = InStream<buffer_size>::getline();
    this->common_content.clear();

    for (size_t i = 0; i < line.size(); i++)
    {
        const char c = line[i];

        if (c != '[')
        {
            common_content += c;
            continue;
        }
        else
        {
            const auto pkg_end_pos = line.find(']', i);
            if (pkg_end_pos == String<buffer_size>::npos)
            {
                common_content = line.substr(i);
                break;
            }
            const auto pkg_content = line.substr(i + 1, pkg_end_pos - i - 1);
            common_content += this->__parse_pkg_content(pkg_content);
            i = pkg_end_pos;
        }
    }

    return std::forward_as_tuple(std::as_const(common_content), std::as_const(__slider_params), std::as_const(__joysticks_params));
}