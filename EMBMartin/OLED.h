#ifndef EMBMARTIN_OLED_H
#define EMBMARTIN_OLED_H

#include <stdint.h>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)

#include "tools.h"

OLED_BEGIN

/**
 * @brief OLED显示驱动类
 *
 * 该类实现了通过I2C接口控制OLED显示屏的功能，支持字符、字符串和数字显示
 * 显示区域被划分为3行，每行可显示16个字符
 * 支持多种显示模式和格式化输出功能
 */
class IO
{
private:
    GPIOPin OLED_SCL;  ///< I2C时钟引脚
    GPIOPin OLED_SDA;  ///< I2C数据引脚
    uint8_t current_X; ///< 当前显示位置的列坐标(1-16)
    uint8_t current_Y; ///< 当前显示位置的行坐标(1-4)

    /**
     * @brief 发送I2C起始信号
     */
    void I2C_Start(void) noexcept;

    /**
     * @brief 发送I2C停止信号
     */
    void I2C_Stop(void) noexcept;

    /**
     * @brief 通过I2C发送一个字节数据
     * @param Byte 要发送的字节
     */
    void I2C_SendByte(uint8_t Byte) noexcept;

    /**
     * @brief 向OLED写入命令
     * @param Command 要写入的命令字节
     */
    void WriteCommand(uint8_t Command) noexcept;

    /**
     * @brief 向OLED写入数据
     * @param Data 要写入的数据字节
     */
    void WriteData(uint8_t Data) noexcept;

    /**
     * @brief 计算X的Y次幂
     * @param X 底数
     * @param Y 指数
     * @return X的Y次幂结果
     */
    uint32_t Pow(uint32_t X, uint32_t Y) noexcept;

    /**
     * @brief 设置显示光标位置
     * @param Y 行坐标(0-2)
     * @param X 列坐标(0-15)
     */
    void SetCursor(uint8_t Y, uint8_t X) noexcept;

    /**
     * @brief 清屏操作，清除整个显示区域
     */
    void Clear(void) noexcept;

    /**
     * @brief 在指定位置显示单个字符
     * @param Line 行号(1-3)
     * @param Column 列号(1-16)
     * @param Char 要显示的字符
     */
    void ShowChar(uint8_t Line, uint8_t Column, char Char) noexcept;

    /**
     * @brief 在当前位置显示单个字符，并自动移动光标
     * @param c 要显示的字符
     */
    void ShowChar(char c) noexcept;

    /**
     * @brief 在指定位置显示字符串
     * @param Line 行号(1-3)
     * @param Column 列号(1-16)
     * @param String 要显示的字符串
     */
    inline void ShowString(uint8_t Line, uint8_t Column, const char String[]) noexcept
    {
        uint8_t i;
        for (i = 0; String[i] != '\0'; i++)
        {
            ShowChar(Line, Column + i, String[i]);
        }
    }

    /**
     * @brief 在当前位置显示字符串，并自动移动光标
     * @param String 要显示的字符串
     */
    inline void ShowString(const char String[]) noexcept
    {
        uint8_t i;
        for (i = 0; String[i] != '\0'; i++)
        {
            ShowChar(String[i]);
        }
    }

    /**
     * @brief 显示无符号整数
     * @param Number 要显示的无符号整数
     */
    void ShowNum(uintmax_t Number) noexcept;

    /**
     * @brief 显示有符号整数
     * @param Number 要显示的有符号整数
     */
    void ShowSignedNum(intmax_t Number) noexcept;

    /**
     * @brief 以十六进制格式显示数字
     * @param Number 要显示的32位无符号整数
     */
    void ShowHexNum(uint32_t Number) noexcept;

    /**
     * @brief 以二进制格式显示数字
     * @param Number 要显示的32位无符号整数
     */
    void ShowBinNum(uint32_t Number) noexcept;

    /**
     * @brief 显示字符
     * @param c 要显示的字符引用
     */
public:
    /**
     * @brief 构造函数，初始化OLED显示模块
     * @param SCL I2C时钟引脚配置
     * @param SDA I2C数据引脚配置
     */
    IO(const GPIOPin &SCL, const GPIOPin &SDA) noexcept;
    inline void show(const char &c) noexcept
    {
        ShowChar(c);
    }

    /**
     * @brief 显示字符数组
     * @param String 要显示的字符串引用
     */
    inline void show(const char String[]) noexcept
    {
        ShowString(String);
    }

    /**
     * @brief 显示数字，支持各种整数类型
     * @tparam T 整数类型
     * @param number 要显示的数字
     */
    template <typename T>
    inline void show(const T &number) noexcept
    {
        static_assert(std::is_integral_v<T>, "只支持整数类型显示");
        if constexpr (std::is_signed_v<T>)
            ShowSignedNum(static_cast<intmax_t>(number));
        else
            ShowNum(number);
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
    void show(double x) noexcept
    {
        char buff[16];
        sprintf(buff, "%g", x);
        ShowString(buff);
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
        ShowChar('\n');
    }

    /**
     * @brief 显示参数并补充空格至固定宽度，最后显示 \\r
     * @tparam Args 参数包类型
     * @param args 可变参数包，用于显示的内容
     *
     * 该函数首先调用show函数显示所有传入的参数，然后计算当前显示位置到固定宽度16之间的差值，
     * 用空格字符填充剩余位置，最后显示一个 \\r 。
     */
    template <typename... Args>
    void showlr(const Args &...args) noexcept
    {
        show(args...);
        if (current_X != 16)
            do
            {
                ShowChar(' ');
            } while (current_X != 16);
        ShowChar('\r');
    }
    template <typename... Args>
    void printf(const char c[], const Args &...args) noexcept
    {
        char buff[64];
        int len = snprintf(buff, sizeof(buff), c, args...);
        if (len > 0 && len < (int)sizeof(buff))
        {
            ShowString(buff);
        }
        else
        {
            // 处理缓冲区不足的情况，例如显示错误信息或截断字符串
            buff[sizeof(buff) - 1] = '\0';
            ShowString(buff);
        }
    }
    inline void set_coordinate(uint8_t x, uint8_t y) noexcept
    {
        current_X = x;
        current_Y = y;
    }
};

OLED_END

#endif // EMBMARTIN_OLED_H