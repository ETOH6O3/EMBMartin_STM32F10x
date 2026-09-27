#ifndef EMBMARTIN_OLED_H
#define EMBMARTIN_OLED_H

#include <cstdio>
#include <stdint.h>
#include <algorithm>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)

#include "basic_tools.h"
#include "stream.h"
#include "I2C.h"
#include "OLEDutility.h"

//------------------------------------------ 版本控制 --------------------------------------------------
#ifndef EMBMARTIN_USING_OLD_OLED_VERSION // 一般在 macro.h 定义
#define EMBMARTIN_USING_OLD_OLED_VERSION 0
#endif

#if EMBMARTIN_USING_OLD_OLED_VERSION
#define EMBMARTIN_OLED_OLD_VERSION_INLINE inline
#define EMBMARTIN_OLED_NEW_VERSION_INLINE
#else
#define EMBMARTIN_OLED_OLD_VERSION_INLINE
#define EMBMARTIN_OLED_NEW_VERSION_INLINE inline
#endif

#define EMBMARTIN_OLED_OLD_VERSION_NAMESPACE_BEGIN                \
    EMBMARTIN_OLED_OLD_VERSION_INLINE namespace EMBMartin_OLED_v1 \
    {
#define EMBMARTIN_OLED_OLD_VERSION_NAMESPACE_END }
#define EMBMARTIN_OLED_NEW_VERSION_NAMESPACE_BEGIN                \
    EMBMARTIN_OLED_NEW_VERSION_INLINE namespace EMBMartin_OLED_v2 \
    {
#define EMBMARTIN_OLED_NEW_VERSION_NAMESPACE_END }

EMBMARTIN_OLED_NAMESPACE_BEGIN

//------------------------------------------ 旧版本代码 --------------------------------------------------
EMBMARTIN_OLED_OLD_VERSION_NAMESPACE_BEGIN
/**
 * @brief OLED 显示驱动类
 *
 * 该类实现了通过 I2C 接口控制 OLED 显示屏的功能，支持字符、字符串和数字显示
 * 显示区域被划分为 3 行，每行可显示 16 个字符
 * 支持多种显示模式和格式化输出功能
 */
class IO
{
private:
    GPIOPin OLED_SCL;  ///< I2C 时钟引脚
    GPIOPin OLED_SDA;  ///< I2C 数据引脚
    uint8_t current_X; ///< 当前显示位置的列坐标 (1-16)
    uint8_t current_Y; ///< 当前显示位置的行坐标 (1-4)

    /**
     * @brief 发送 I2C 起始信号
     */
    void I2C_Start(void) noexcept;

    /**
     * @brief 发送 I2C 停止信号
     */
    void I2C_Stop(void) noexcept;

    /**
     * @brief 通过 I2C 发送一个字节数据
     * @param Byte 要发送的字节
     */
    void I2C_SendByte(uint8_t Byte) noexcept;

    /**
     * @brief 向 OLED 写入命令
     * @param Command 要写入的命令字节
     */
    void WriteCommand(uint8_t Command) noexcept;

    /**
     * @brief 向 OLED 写入数据
     * @param Data 要写入的数据字节
     */
    void WriteData(uint8_t Data) noexcept;

    /**
     * @brief 计算 X 的 Y 次幂
     * @param X 底数
     * @param Y 指数
     * @return X 的 Y 次幂结果
     */
    uint32_t Pow(uint32_t X, uint32_t Y) noexcept;

    /**
     * @brief 设置显示光标位置
     * @param Y 行坐标 (0-2)
     * @param X 列坐标 (0-15)
     */
    void SetCursor(uint8_t Y, uint8_t X) noexcept;

    /**
     * @brief 清屏操作，清除整个显示区域
     */
    void Clear(void) noexcept;

    /**
     * @brief 在指定位置显示单个字符
     * @param Line 行号 (1-3)
     * @param Column 列号 (1-16)
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
     * @param Line 行号 (1-3)
     * @param Column 列号 (1-16)
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
     * @param Number 要显示的 32 位无符号整数
     */
    void ShowHexNum(uint32_t Number) noexcept;

    /**
     * @brief 以二进制格式显示数字
     * @param Number 要显示的 32 位无符号整数
     */
    void ShowBinNum(uint32_t Number) noexcept;

    /**
     * @brief 显示字符
     * @param c 要显示的字符引用
     */
public:
    /**
     * @brief 构造函数，初始化 OLED 显示模块
     * @param SCL I2C 时钟引脚配置
     * @param SDA I2C 数据引脚配置
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
     * @brief 空参数的 show 函数，用于适配可变参数模板
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
     * 该函数首先调用 show 函数显示所有传入的参数，然后计算当前显示位置到固定宽度 16 之间的差值，
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

EMBMARTIN_OLED_OLD_VERSION_NAMESPACE_END

//------------------------------------------ 新版本代码 --------------------------------------------------

EMBMARTIN_OLED_NEW_VERSION_NAMESPACE_BEGIN

class OLEDBase : public I2C
{
protected:
    struct ControlBytes
    {
        enum
        {
            SINGLE_CMD = 0X00,
            SINGLE_DATA = 0X40,
            CONTINUEOUS_CMD = 0X80,
            CONTINUEOUS_DATA = 0XC0,
        };
    };
    struct Cmds
    {
        enum CmdsEnum : uint8_t
        {
            // ========================= 一、页寻址模式 - 列地址设置（需按位或指定低 4 位） =========================
            SetColStartL = 0b0000'0000, // 页寻址模式下设置列起始地址低 4 位，用法：SetColStartL | 0x00~0x0F（低 4 位地址）
            SetColStartH = 0b0001'0000, // 页寻址模式下设置列起始地址高 4 位，用法：SetColStartH | 0x00~0x0F（高 4 位地址）

            // ========================= 二、显存寻址模式设置（多字节：主命令 + 后续参数） =========================
            SetMemoryAddrMode = 0x20, // 设置显存寻址模式（主命令，多字节 1/2），后续需发寻址模式参数
                                      // SetMemoryAddrMode 的后续参数（可选范围仅 3 种，视为指令加入）
            HoriAddrMode = 0b00,      // 水平寻址模式（SetMemoryAddrMode 后续参数 1）
            VertiAddrMode = 0b01,     // 垂直寻址模式（SetMemoryAddrMode 后续参数 2）
            PageAddrMode = 0b10,      // 页寻址模式（SetMemoryAddrMode 后续参数 3，复位默认）

            // ========================= 三、列地址范围设置（多字节：主命令 + 后续参数） =========================
            SetColAddr = 0x21,       // 设置显存列地址范围（主命令，多字节 1/3），后续需发 2 字节：ColStart + ColEnd
                                     // SetColAddr 的常用后续参数（可选范围固定为 0~127，取典型值视为指令）
            ColStart_Default = 0x00, // 列起始地址默认值（SetColAddr 后续参数 1，复位默认）
            ColEnd_Default = 0x7F,   // 列终止地址默认值（SetColAddr 后续参数 2，复位默认，对应 127 列）

            // ========================= 四、页地址范围设置（多字节：主命令 + 后续参数） =========================
            SetPageAddrRange = 0x22,  // 设置显存页地址范围（主命令，多字节 1/3），后续需发 2 字节：PageStart + PageEnd
                                      // SetPageAddr 的常用后续参数（可选范围仅 0~7，取典型值视为指令）
            PageStart_Default = 0x00, // 页起始地址默认值（SetPageAddr 后续参数 1，复位默认，对应 PAGE0）
            PageEnd_Default = 0x07,   // 页终止地址默认值（SetPageAddr 后续参数 2，复位默认，对应 PAGE7）

            // ========================= 五、页寻址模式 - 页起始地址（需按位或指定低 3 位） =========================
            SetPageStartForPageAddrMode = 0b1011'0000, // 页寻址模式下设置页起始地址，用法：| 0x00~0x07（低 3 位 = PAGE0~PAGE7）
                                                       // SetPageStartForPageAddrMode 的常用后续位（可选范围 0~7，视为指令）
            PageStart_Page0 = 0b000,                   // 页起始地址 PAGE0（SetPageStartForPageAddrMode 低 3 位参数）
            PageStart_Page7 = 0b111,                   // 页起始地址 PAGE7（SetPageStartForPageAddrMode 低 3 位参数）

            // ========================= 六、显示起始行设置（需按位或指定低 6 位） =========================
            SetDisplayStartLine = 0b0100'0000,   // 设置显示起始行，用法：| 0x00 ~ 0x3F（低 6 位 = 行号 0 ~ 63）
                                                 // SetDisplayStartLine 的常用后续位（可选范围 0~63，取默认值视为指令）
            DisplayStartLine_Default = 0b000000, // 显示起始行默认值（SetDisplayStartLine 低 6 位参数，复位默认 0 行）

            // ========================= 七、对比度控制（多字节：主命令 + 后续参数） =========================
            SetContrastControl = 0x81, // 设置 BANK0 对比度（主命令，多字节 1/2），后续需发 1 字节对比度值
                                       // SetContrastControl 的后续参数（可选范围 0~255，取典型值视为指令）
            Contrast_Default = 0x7F,   // 对比度默认值（SetContrastControl 后续参数，复位默认，中等亮度）
            Contrast_Max = 0xFF,       // 最大对比度（SetContrastControl 后续参数，最亮）
            Contrast_Min = 0x00,       // 最小对比度（SetContrastControl 后续参数，最暗）

            // ========================= 八、段驱动映射（普通单字节指令） =========================
            SetSegmentReMap_A0 = 0xA0, // 段映射：列地址 0→SEG0（复位默认，普通单字节）
            SetSegmentReMap_A1 = 0xA1, // 段映射：列地址 127→SEG0（普通单字节）

            // ========================= 九、全屏显示模式（普通单字节指令） =========================
            EntireDisplayOn_A4 = 0xA4, // 显示显存内容（复位默认，普通单字节）
            EntireDisplayOn_A5 = 0xA5, // 强制全屏点亮（忽略显存内容，普通单字节）

            // ========================= 十、显示正反相（普通单字节指令） =========================
            SetNormalDisplay = 0xA6,  // 正常显示：显存 1 = 亮 / 0 = 灭（复位默认，普通单字节）
            SetInverseDisplay = 0xA7, // 反相显示：显存 1 = 灭 / 0 = 亮（普通单字节）

            // ========================= 十一、COM 多路复用比（多字节：主命令 + 后续参数） =========================
            SetMultiplexRatio = 0xA8,      // 设置 COM 多路复用比（主命令，多字节 1/2），后续需发 1 字节：N（复用比 = N+1）
                                           // SetMultiplexRatio 的后续参数（可选范围 0x0F~0x3F，取典型值视为指令）
            MultiplexRatio_Default = 0x3F, // 复用比默认值（SetMultiplexRatio 后续参数，复位默认 0x3F=64MUX）
            MultiplexRatio_16 = 0x0F,      // 16 路复用比（SetMultiplexRatio 后续参数，对应 17MUX）
            MultiplexRatio_32 = 0x1F,      // 32 路复用比（SetMultiplexRatio 后续参数，对应 33MUX）

            // ========================= 十二、COM 扫描方向（普通单字节指令） =========================
            SetCOMScansDir_C0 = 0xC0, // COM 扫描：COM0→COM [N-1]（正常模式，复位默认，普通单字节）
            SetCOMScansDir_C8 = 0xC8, // COM 扫描：COM [N-1]→COM0（翻转模式，普通单字节）

            // ========================= 十三、显示垂直偏移（多字节：主命令 + 后续参数） =========================
            SetDisplayOffset = 0xD3,      // 设置显示垂直偏移（主命令，多字节 1/2），后续需发 1 字节偏移值
                                          // SetDisplayOffset 的后续参数（可选范围 0~63，取默认值视为指令）
            DisplayOffset_Default = 0x00, // 垂直偏移默认值（SetDisplayOffset 后续参数，复位默认 0 行偏移）

            // ========================= 十四、显示时钟与振荡器（多字节：主命令 + 后续参数） =========================
            SetDisplayClockOscFreq = 0xD5,  // 设显示时钟分频 + 振荡器频率（主命令，多字节 1/2），后续需发 1 字节：[Freq (高 4 位) | Div (低 4 位)]
                                            // SetDisplayClockOscFreq 的后续参数（可选范围固定，视为指令）
            DisplayClockOsc_Default = 0x80, // 默认配置（SetDisplayClockOscFreq 后续参数：高 4 位 1000b = 默认频率，低 4 位 0000b = 分频比 1）
                                            // 下面三个通过按位或合并使用
            DisplayClockDiv_1 = 0x00,       // 显示时钟分频比 1（SetDisplayClockOscFreq 低 4 位参数）
            DisplayClockDiv_16 = 0x0F,      // 显示时钟分频比 16（SetDisplayClockOscFreq 低 4 位参数）
            OscFreq_Default = 0x80,         // 振荡器默认频率（SetDisplayClockOscFreq 高 4 位参数，1000b）

            // ========================= 十五、预充电周期（多字节：主命令 + 后续参数） =========================
            SetPreChargePeriod = 0xD9,      // 设置预充电周期（主命令，多字节 1/2），后续需发 1 字节：[Phase2 (高 4 位) | Phase1 (低 4 位)]
                                            // SetPreChargePeriod 的后续参数（可选范围 1~15，取默认值视为指令）
            PreChargePeriod_Default = 0x22, // 预充电周期默认值（SetPreChargePeriod 后续参数：Phase1=2，Phase2=2，复位默认）

            // ========================= 十六、COM 引脚配置（多字节：主命令 + 后续参数） =========================
            SetCOMPinsConfig = 0xDA,      // 设置 COM 引脚硬件配置（主命令，多字节 1/2），后续需发 1 字节：[Map (bit5) | 00 | Seq (bit4) | 0000]
                                          // SetCOMPinsConfig 的后续参数（可选范围仅 4 种组合，视为指令）
            COMPinsConfig_Default = 0x12, // 默认配置（SetCOMPinsConfig 后续参数：bit4=1 = 交替 COM，bit5=0 = 禁用左右映射，复位默认）
            COMPinsConfig_Seq = 0x02,     // 连续 COM 配置（SetCOMPinsConfig 后续参数：bit4=0 = 连续 COM，bit5=0 = 禁用映射）
            COMPinsConfig_Map = 0x32,     // 交替 + 映射配置（SetCOMPinsConfig 后续参数：bit4=1 = 交替 COM，bit5=1 = 启用映射）
            COMPinsConfig_SeqMap = 0x22,  // 连续 + 映射配置（SetCOMPinsConfig 后续参数：bit4=0 = 连续 COM，bit5=1 = 启用映射）

            // ========================= 十七、VCOMH 非选中电平（多字节：主命令 + 后续参数） =========================
            SetVCOMHLevel = 0xDB, // 设置 VCOMH 非选中电平（主命令，多字节 1/2），后续需发 1 字节电平值
                                  // SetVCOMHLevel 的后续参数（可选范围仅 3 种，视为指令）
            VCOMH_0_65VCC = 0x00, // VCOMH=~0.65×VCC（SetVCOMHLevel 后续参数 1）
            VCOMH_0_77VCC = 0x20, // VCOMH=~0.77×VCC（SetVCOMHLevel 后续参数 2，复位默认）
            VCOMH_0_83VCC = 0x30, // VCOMH=~0.83×VCC（SetVCOMHLevel 后续参数 3）

            // ========================= 十八、显示开关（普通单字节指令） =========================
            SetDisplayOff = 0xAE, // 显示关闭（休眠模式，复位默认，普通单字节）
            SetDisplayOn = 0xAF,  // 显示开启（正常模式，普通单字节）

            // ========================= 十九、无操作命令（普通单字节指令） =========================
            NOP = 0xE3, // 无操作（占位 / 测试，普通单字节）

            // ========================= 二十、水平滚动配置（多字节：主命令 + 后续参数） =========================
            SetHoriScroll_Right = 0x26,      // 配置向右水平滚动（主命令，多字节 1/6），后续需发 5 字节：0x00 + PageStart + Interval + PageEnd + 0x00 + 0xFF
            SetHoriScroll_Left = 0x27,       // 配置向左水平滚动（主命令，多字节 1/6），后续参数格式同 SetHoriScroll_Right
                                             // 滚动间隔参数（可选范围仅 8 种，视为指令）
            ScrollInterval_2Frames = 0x07,   // 滚动间隔：2 帧（SetHoriScroll_xxx 后续间隔参数 1）
            ScrollInterval_3Frames = 0x04,   // 滚动间隔：3 帧（SetHoriScroll_xxx 后续间隔参数 2）
            ScrollInterval_4Frames = 0x05,   // 滚动间隔：4 帧（SetHoriScroll_xxx 后续间隔参数 3）
            ScrollInterval_5Frames = 0x00,   // 滚动间隔：5 帧（SetHoriScroll_xxx 后续间隔参数 4，默认）
            ScrollInterval_25Frames = 0x06,  // 滚动间隔：25 帧（SetHoriScroll_xxx 后续间隔参数 5）
            ScrollInterval_64Frames = 0x01,  // 滚动间隔：64 帧（SetHoriScroll_xxx 后续间隔参数 6）
            ScrollInterval_128Frames = 0x02, // 滚动间隔：128 帧（SetHoriScroll_xxx 后续间隔参数 7）
            ScrollInterval_256Frames = 0x03, // 滚动间隔：256 帧（SetHoriScroll_xxx 后续间隔参数 8）

            // ========================= 二十一、垂直 + 水平滚动配置（多字节：主命令 + 后续参数） =========================
            SetVertHoriScroll_Right = 0x29,  // 配置垂直 + 向右滚动（主命令，多字节 1/7），后续需发 6 字节：0x00 + PageStart + Interval + PageEnd + VertOffset + 0x00
            SetVertHoriScroll_Left = 0x2A,   // 配置垂直 + 向左滚动（主命令，多字节 1/7），后续参数格式同 SetVertHoriScroll_Right
                                             // 垂直偏移参数（取默认值视为指令）
            VertScrollOffset_Default = 0x00, // 垂直偏移默认值（SetVertHoriScroll_xxx 后续参数，复位默认 0 行）

            // ========================= 二十二、滚动启停（普通单字节指令） =========================
            DeactivateScroll = 0x2E, // 停止所有滚动（执行后需重写显存，普通单字节）
            ActivateScroll = 0x2F,   // 启动已配置滚动（需先执行 26h/27h/29h/2Ah，普通单字节）

            // ========================= 二十三、垂直滚动区域（多字节：主命令 + 后续参数） =========================
            SetVerticalScrollArea = 0xA3,      // 设置垂直滚动区域（主命令，多字节 1/3），后续需发 2 字节：TopFixedRows + ScrollRows
                                               // SetVerticalScrollArea 的后续参数（取默认值视为指令）
            VertScrollTopFixed_Default = 0x00, // 顶部固定行数默认值（SetVerticalScrollArea 后续参数 1，复位默认 0 行）
            VertScrollAreaRows_Default = 0x40, // 滚动区域行数默认值（SetVerticalScrollArea 后续参数 2，复位默认 64 行）

            // ------------------------------------------充电泵--------------------------------------------------
            SetChargePump = 0x8D, // 设置充电泵（多字节1/2，用于开启OLED电源泵，后续需发1字节参数）
            ChargePumpEnable = 0x14,
        };
    };

    void write_cmd(uint8_t cmd) noexcept;
    void write_data(uint8_t data) noexcept;
    template <typename Container>
    std::enable_if_t<has_iterator_v<Container>> write_data(const Container &data) noexcept;

    inline void set_coordinate(uint8_t x, uint8_t page /*0 - 7*/) noexcept
    {
        // 写入列地址
        write_cmd(Cmds::SetColStartL | (x & 0x0F));
        write_cmd(Cmds::SetColStartH | ((x & 0xF0) >> 4));
        // 页地址
        write_cmd(Cmds::SetPageStartForPageAddrMode | (page & Cmds::PageStart_Page7));
    }

public:
    OLEDBase(GPIOPin SCL, GPIOPin SDA, bool SA0 = 0) noexcept;
    inline void clear() noexcept
    {
        for (uint8_t page = 0; page < 8; page++)
        {
            set_coordinate(0, page);
            for (uint8_t col = 0; col < 128; col++)
            {
                write_data(0x00);
            }
        }
    }
};

template <typename Container>
std::enable_if_t<has_iterator_v<Container>> OLEDBase::write_data(const Container &datas) noexcept
{
    start();
    EMBMARTIN_KEEP_CODE_ORDER;
    send_byte(this->_addr); // 寻址
    EMBMARTIN_KEEP_CODE_ORDER;
    receive_ack();
    EMBMARTIN_KEEP_CODE_ORDER;
    send_byte(ControlBytes::SINGLE_DATA); // 控制位: 非连续写数据
    EMBMARTIN_KEEP_CODE_ORDER;
    receive_ack();
    EMBMARTIN_KEEP_CODE_ORDER;
    for (const auto &data : datas)
    {
        send_byte(data);
        EMBMARTIN_KEEP_CODE_ORDER;
        receive_ack();
    }

    EMBMARTIN_KEEP_CODE_ORDER;
    stop();
}

template <OLEDFontSize font_size = OLEDFontSize::F8x16, typename BaseOStream = EMBMartin::OutStream<128>>
class OLEDConsole : private OLEDBase, public BaseOStream
{
private:
    static constexpr uint8_t font_size_x = static_cast<uint8_t>(font_size);
    static constexpr uint8_t font_size_y = static_cast<uint8_t>(font_size == OLEDFontSize::F6x8 ? 8 : 16);
    static constexpr uint8_t col_num = 128 / font_size_x;
    static constexpr uint8_t col_max = col_num - 1;
    static constexpr uint8_t row_num = 64 / font_size_y;
    static constexpr uint8_t row_max = row_num - 1;

    Coordinate<uint8_t, 0, col_max, 0, row_max> current_position{0, 0};

    inline void show_char(uint8_t x, uint8_t page, char c)
    {
        if constexpr (font_size == OLEDFontSize::F6x8)
        {
            OLEDBase::set_coordinate(x, page);
            for (uint8_t i = 0; i < 6; i++)
                write_data(OLED_F6x8[c - ' '][i]);
        }
        else if constexpr (font_size == OLEDFontSize::F8x16)
        {
            OLEDBase::set_coordinate(x, page);
            for (uint8_t i = 0; i < 8; i++)
                write_data(OLED_F8x16[c - ' '][i]);
            OLEDBase::set_coordinate(x, page + 1);
            for (uint8_t i = 0; i < 8; i++)
                write_data(OLED_F8x16[c - ' '][i + 8]);
        }
    }

    void show_char(char c) noexcept;

public:
    using OLEDBase::OLEDBase;

    /**
     * @brief 输出一个字符到 OLED
     * 缓冲区由 OutStream 管理，这里只需要把字符交给显示驱动
     */
    inline void output_char(char c) noexcept override
    {
        show_char(c);
    };

    inline void set_coordinate(uint8_t x, uint8_t y) noexcept
    {
        this->current_position.x = x;
        this->current_position.y = y;
    }

    inline void clear() noexcept
    {
        OLEDBase::clear();
        this->current_position.x = 0;
        this->current_position.y = 0;
    }
};

template <OLEDFontSize font_size, typename BaseOStream>
void OLEDConsole<font_size, BaseOStream>::show_char(char c) noexcept
{
    if (c == '\n')
    {
        this->current_position.assignment_add_y(1);
        this->current_position.x = 0;
        return;
    }
    if (c == '\r')
    {
        this->current_position.x = 0;
        return;
    }
    if (c == '\t')
    {
        this->current_position += 4 - this->current_position.x % 4;
        return;
    }
    if (/* 非可显示 ASCII 码 */ c < 32 || c > 126)
    {
        char str_hex[3];
        // 转为 hex
        sprintf(str_hex, "%02X", static_cast<uint8_t>(c));

        show_char(this->current_position.x * font_size_x, this->current_position.y * font_size_y / 8, '\\');
        this->current_position++;
        show_char(this->current_position.x * font_size_x, this->current_position.y * font_size_y / 8, 'x');
        this->current_position++;
        show_char(this->current_position.x * font_size_x, this->current_position.y * font_size_y / 8, str_hex[0]);
        this->current_position++;
        show_char(this->current_position.x * font_size_x, this->current_position.y * font_size_y / 8, str_hex[1]);
        this->current_position++;
        return;
    }
    
    show_char(this->current_position.x * font_size_x, this->current_position.y * font_size_y / 8, c);
    this->current_position++;
}


class OLEDPlayerBase : private OLEDBase
{
protected:
    std::array<std::array<IterableUInt8, 128>, 8> video_mem{};

    constexpr static uint8_t col_num = 128;
    constexpr static uint8_t row_num = 64;
    using OLEDCoordinate = Coordinate<uint8_t, 0, row_num - 1, 0, col_num - 1>;

public:
    using OLEDBase::OLEDBase;
    void update() noexcept;
    inline auto operator[](const OLEDCoordinate &index) noexcept
    {
        const auto &[row_index, col_index] = index;

        const uint8_t page = row_index / 8;
        const uint8_t bit_pos = row_index % 8;
        return video_mem[page][col_index][bit_pos];
    }
    inline void clear(bool auto_update = true) noexcept
    {
        for (auto &page : video_mem)
        {
            std::fill(page.begin(), page.end(), 0);
        }
        if (auto_update)
            update();
    }

    template <uint8_t X, uint8_t Y>
    void show_pic(const OLEDBitMap<X, Y> &pic, const OLEDCoordinate &start, bool auto_update = true) noexcept
    {
        constexpr static auto pic_height = OLEDBitMap<X, Y>::row_num;
        constexpr static auto pic_width = OLEDBitMap<X, Y>::col_num;

        const auto &[start_row, start_col] = start;
        for (uint8_t r = 0; (r < pic_height) && (start_row + r < row_num); r++)
        {
            for (uint8_t c = 0; (c < pic_width) && (start_col + c < col_num); c++)
            {
                const auto &pix = pic[{r, c}];
                switch (pix)
                {
                case BWPixel::Transparent:
                    break;
                default:
                    (*this)[{static_cast<uint8_t>(start_row + r), static_cast<uint8_t>(start_col + c)}] = bool(pix);
                    break;
                }
            }
        }

        if (auto_update)
            update();
    }

    template <typename OLEDPIC>
    void show_pic(const OLEDPIC &pic, const OLEDCoordinate &start, bool auto_update = true) noexcept
    {
        constexpr static auto pic_height = OLEDPIC::row_num;
        constexpr static auto pic_width = OLEDPIC::col_num;

        return show_pic(OLEDBitMap<pic_height, pic_width>(pic), start, auto_update);
    }
};
EMBMARTIN_OLED_NEW_VERSION_NAMESPACE_END

EMBMARTIN_OLED_NAMESPACE_END

#endif // EMBMARTIN_OLED_H