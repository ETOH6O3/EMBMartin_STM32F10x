/**
 ******************************************************************************
 * @file    tools.h
 * @author  孙鸣淼
 * @brief   这里存放一些嵌入式开发中常用的工具 （基于 STM32 标准外设库）
 ******************************************************************************
 * @attention
 * 0. 需要预定义宏 STM32_DEVICE_HEADER 为对应的 STM32 设备头文件名，如 stm32f10x.h （不得含引号）
 *    或者在 macro.h 中定义 STM32_DEVICE_HEADER 宏，这也是等价的
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#ifndef EMBMARTIN_TOOLS_H
#define EMBMARTIN_TOOLS_H

#include <cassert>
#include <type_traits>
#include <numeric>
#include <cmath>
#include <algorithm>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "meta.h"
#include "basic_tools.h"
#include "system.h"

#if EMBMARTIN_DEBUGING

#include "OLED.h"

#endif // EMBMARTIN_DEBUGING

EMBMARTIN_DEBUGING_EXTERN_OLED; // debug 控制台

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

// TODO: 时钟使能用 HAL 库重写


/**
 * @brief 读取按键状态并进行消抖处理
 * @param GPIOx GPIO 端口指针
 * @param pin GPIO 引脚号
 * @param trigger 按下状态，默认为 false, 低电平为按下状态；若设为 true，则高电平为按下状态
 * @return 按键按下并释放后返回 true，否则返回 false
 */
inline bool read_key(GPIO_TypeDef *GPIOx, uint16_t pin, bool trigger = false, intmax_t debounce_delay_ms = 20) noexcept
{
    if (GPIO_ReadInputDataBit(GPIOx, pin) == trigger) // 按键按下
    {
        Delay_ms(debounce_delay_ms); // 消抖动
        while (GPIO_ReadInputDataBit(GPIOx, pin) == trigger)
            ;                        // 等待按键释放
        Delay_ms(debounce_delay_ms); // 消抖动
        return true;
    }
    return false;
}
/**
 * @brief 读取按键状态并进行消抖处理
 * @param pin GPIO 引脚封装结构体
 * @param trigger 按下状态，默认为 false, 低电平为按下状态；若设为 true，则高电平为按下状态
 * @param debounce_delay_ms 消抖动延迟时间，单位为毫秒，默认为 20 毫秒
 * @return 按键按下并释放后返回 true，否则返回 false
 */
inline bool read_key(GPIOPin pin, bool trigger = false, intmax_t debounce_delay_ms = 20) noexcept
{
    return read_key(pin.port, pin.pin, trigger, debounce_delay_ms);
}

/**
 * @brief 将容器中的多个 GPIO 引脚合并为一个 GPIOPin 对象
 *
 * 此函数通过按位或操作将多个 GPIO 引脚的 pin 值合并成一个新的 GPIOPin 对象。
 * 合并后的对象保留第一个引脚的端口，并将所有引脚的 pin 值进行或运算组合。
 *
 * @tparam Container 容器类型，其元素类型必须为 GPIOPin
 * @param pins 包含多个 GPIOPin 对象的容器
 * @return GPIOPin 合并后的 GPIOPin 对象，其 port 为第一个引脚的 port，
 *         pin 为所有引脚 pin 值按位或运算的结果
 *
 * @note 使用 static_assert 确保容器元素类型为 GPIOPin
 */
template <typename Container>
inline GPIOPin merge_pins(const Container &pins) noexcept
{
    static_assert(std::is_same_v<typename Container::value_type, GPIOPin>,
                  "容器元素类型必须为 GPIOPin");
    assert(std::accumulate(pins.begin(), pins.end(), bool(1), [&](bool last_rslt, GPIOPin pin2)
                           { return last_rslt && (pins[0].port == pin2.port); }) &&
           "All pins must be on the same port");

    return {pins[0].port, std::accumulate(
                              pins.begin(),
                              pins.end(),
                              (uint16_t)0,
                              [](uint16_t last_rslt, GPIOPin pin2) -> uint16_t
                              { return last_rslt | pin2.pin; })};
}

/**
 * @brief 按键类，用于处理 STM32 GPIO 按键输入
 *
 * 该类封装了按键的 GPIO 配置和状态读取功能，支持按键消抖处理。
 * 可以配置按键的触发方式（高电平或低电平触发）。
 */
class Key
{
private:
    GPIOPin pin;  //!< GPIO 引脚信息
    bool trigger; //!< 按键触发状态，false 表示低电平触发，true 表示高电平触发

public:
    /**
     * @brief 构造一个按键对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param trigger 按键触发状态，默认为 false（低电平触发）
     * @note 会在构造时自动初始化 GPIO 为输入模式
     */
    inline Key(GPIOPin p, bool trigger = false) noexcept : pin(p), trigger(trigger)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = trigger ? GPIO_Mode_IPD : GPIO_Mode_IPU,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);
    }

    /**
     * @brief 构造一个按键对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param trigger 按键触发状态，默认为 false（低电平触发）
     */
    inline Key(GPIO_TypeDef *port, uint16_t pin, bool trigger = false) noexcept : Key{GPIOPin{port, pin}, trigger} {}

    /**
     * @brief 检测按键是否被按下
     * @return bool 按键按下并释放后返回 true，否则返回 false
     * @note 内部已实现按键消抖处理
     */
    inline bool is_pressed() const noexcept
    {
        return read_key(this->pin, this->trigger);
    }
};

/**
 * @brief
 * 安全的输出引脚类，与普通输出引脚类不同的是，该类初始化为 null_pin 时不会导致未定义行为
 *
 * 同时，该类会根据需求自动关闭 JTAG/SWD 功能以释放相应引脚
 *
 */
class OutPin
{
private:
    GPIOPin pin;        //!< GPIO 引脚信息
    bool current_level; //!< 目前输出
public:
    inline OutPin(GPIOPin p) noexcept : pin(p), current_level(0)
    {
        if (p == null_pin)
            return;

        if (p == PB4)
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
        else if ((p == PA15) || (p == PB3))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
        else if ((p == PA13) || (p == PA14))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->reset();
    }
    volatile inline void set() noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_SET);
        current_level = true;
    }
    volatile inline void reset() noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_RESET);
        current_level = false;
    }
    volatile inline void toggle() noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, current_level ? Bit_RESET : Bit_SET);
        current_level = !current_level;
    }
    volatile inline bool get_level() noexcept
    {
        return current_level;
    }
};

/**
 * @brief LED 类，用于控制 STM32 GPIO 连接的 LED
 *
 * 该类封装了 LED 的 GPIO 配置和控制功能，支持点亮、熄灭和翻转操作。
 */
class LED
{
private:
    GPIOPin pin;       //!< GPIO 引脚信息
    bool driven_level; //!< 驱动电平，false 表示低电平点亮，true 表示高电平点亮

public:
    /**
     * @brief 构造一个 LED 对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline LED(GPIOPin p, bool driven_level = false) noexcept : pin(p), driven_level(driven_level)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->off(); // 初始化时熄灭 LED
    }

    /**
     * @brief 构造一个 LED 对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     */
    inline LED(GPIO_TypeDef *port, uint16_t pin, bool driven_level = false) noexcept : LED{GPIOPin{port, pin}, driven_level} {}

    /**
     * @brief 点亮 LED
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相应电平以点亮 LED
     */
    inline void on() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_SET : Bit_RESET);
    }

    /**
     * @brief 熄灭 LED
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相反电平以熄灭 LED
     */
    inline void off() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_RESET : Bit_SET);
    }

    /**
     * @brief 切换 LED 状态
     *
     * 翻转当前 LED 的状态，如果当前是点亮则熄灭，如果当前是熄灭则点亮
     */
    inline void toggle() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin,
                      BitAction(!GPIO_ReadOutputDataBit(this->pin.port, this->pin.pin)));
    }
};

/**
 * @brief 蜂鸣器类，用于控制 STM32 GPIO 连接的蜂鸣器
 *
 * 该类封装了蜂鸣器的 GPIO 配置和控制功能，支持基本的开关控制以及蜂鸣操作。
 */
class Buzzer
{
private:
    GPIOPin pin;       //!< GPIO 引脚信息
    bool driven_level; //!< 驱动电平，false 表示低电平驱动，true 表示高电平驱动

public:
    /**
     * @brief 构造一个蜂鸣器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline Buzzer(GPIOPin p, bool driven_level = false) noexcept : pin(p), driven_level(driven_level)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->off(); // 初始化时关闭蜂鸣器
    }

    /**
     * @brief 构造一个蜂鸣器对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     */
    inline Buzzer(GPIO_TypeDef *port, uint16_t pin, bool driven_level = false) noexcept : Buzzer{GPIOPin{port, pin}, driven_level} {}

    /**
     * @brief 打开蜂鸣器
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相应电平以打开蜂鸣器
     */
    inline void on() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_SET : Bit_RESET);
    }

    /**
     * @brief 关闭蜂鸣器
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相反电平以关闭蜂鸣器
     */
    inline void off() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_RESET : Bit_SET);
    }

    /**
     * @brief 切换蜂鸣器状态
     *
     * 翻转当前蜂鸣器的状态，如果当前是开启则关闭，如果当前是关闭则开启
     */
    inline void toggle() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin,
                      BitAction(!GPIO_ReadOutputDataBit(this->pin.port, this->pin.pin)));
    }

    /**
     * @brief 发出一次蜂鸣声
     * @param duration_ms 蜂鸣持续时间（毫秒），默认为 100ms
     */
    inline void beep(intmax_t duration_ms = 100) const noexcept
    {
        this->on();
        Delay_ms(duration_ms);
        this->off();
    }

    /**
     * @brief 发出多次蜂鸣声
     * @param times 蜂鸣次数
     * @param duration_ms 每次蜂鸣持续时间（毫秒）
     * @param interval_ms 每次蜂鸣之间的间隔时间（毫秒），默认为 500ms
     */
    inline void beep(intmax_t times, intmax_t duration_ms, intmax_t interval_ms = 500) const noexcept
    {
        for (intmax_t i = 0; i < times - 1; i++)
        {
            this->beep(duration_ms);
            Delay_ms(interval_ms);
        }
        this->beep(duration_ms);
    }
};

/**
 * @brief 传感器类，用于读取 STM32 GPIO 连接的传感器状态
 *
 * 该类封装了传感器的 GPIO 配置和状态读取功能，适用于简单的数字传感器。
 */
class Sensor
{
private:
    GPIOPin pin; //!< GPIO 引脚信息

public:
    /**
     * @brief 构造一个传感器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     */
    inline Sensor(GPIOPin p) noexcept : pin(p)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_IPU,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);
    }

    /**
     * @brief 构造一个传感器对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     */
    inline Sensor(GPIO_TypeDef *port, uint16_t pin) noexcept : Sensor{GPIOPin{port, pin}} {}

    /**
     * @brief 获取传感器状态
     * @return bool 传感器状态，高电平返回 true，低电平返回 false
     */
    inline bool get() const noexcept
    {
        return GPIO_ReadInputDataBit(this->pin.port, this->pin.pin) == Bit_SET;
    }
};

/**
 * @brief 计数传感器类，用于处理基于外部中断的计数传感器
 *
 * 该类封装了计数传感器的配置和中断处理功能，通过指定的 GPIO 引脚来检测信号边沿变化并进行计数。
 * 适用于如霍尔传感器、光电编码器等需要计数的传感器设备。
 */
class CounterSensor
{
private:
    bool edge;   //!< 触发方式，true 表示上升沿触发，false 表示下降沿触发
    GPIOPin pin; //!< GPIO 引脚信息

public:
    /**
     * @brief 构造一个计数传感器对象
     * @param _pin GPIOPin 结构体，包含端口和引脚信息
     * @param _edge 触发方式，true 表示上升沿触发，false 表示下降沿触发
     * @param PreemptionPriority 中断抢占优先级，默认为 0
     * @param SubPriority 中断子优先级，默认为 0
     * @param _GPIO_Speed GPIO 速度，默认为 GPIO_Speed_50MHz
     * @param _EXTI_Mode EXTI 模式，默认为 EXTI_Mode_Interrupt (中断模式)
     */
    CounterSensor(
        GPIOPin _pin, bool _edge,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        GPIOSpeed_TypeDef _GPIO_Speed = GPIO_Speed_50MHz,
        EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt) noexcept;
};

/**
 * @brief 旋转编码器类，用于处理正交编码器输入
 *
 * 该类封装了旋转编码器的配置和处理功能，通过两个相位信号 (A 相和 B 相) 来检测旋转方向和计数。
 * 适用于机械旋转编码器等需要旋转位置检测的设备。
 */
class EXTIRotaryEncoder
{
private:
    GPIOPin pin_a;     //!< A 相引脚
    GPIOPin pin_b;     //!< B 相引脚
    int16_t count = 0; //!< 当前计数值

public:
    /**
     * @brief 构造一个旋转编码器对象
     * @param pin_a A 相引脚 GPIOPin 结构体
     * @param pin_b B 相引脚 GPIOPin 结构体
     * @param PreemptionPriority 中断抢占优先级，默认为 0
     * @param SubPriority 中断子优先级，默认为 0
     * @param GPIO_Speed GPIO 速度，默认为 GPIO_Speed_50MHz
     * @param _EXTI_Mode EXTI 模式，默认为 EXTI_Mode_Interrupt (中断模式)
     */
    EXTIRotaryEncoder(
        GPIOPin pin_a, GPIOPin pin_b,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        GPIOSpeed_TypeDef GPIO_Speed = GPIO_Speed_50MHz,
        EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt) noexcept;

    /**
     * @brief 获取当前计数值
     * @return int16_t 当前计数值
     */
    inline int16_t get_count() const noexcept { return count; }

    /**
     * @brief A 相引脚中断处理函数
     *
     * 当 A 相引脚发生中断时调用此函数，根据 B 相引脚的电平状态判断旋转方向并更新计数
     */
    inline void pin_a_handler() noexcept
    {
        if (GPIO_ReadInputDataBit(pin_b.port, pin_b.pin) == 0)
        {
            count++;
        }
    }

    /**
     * @brief B 相引脚中断处理函数
     *
     * 当 B 相引脚发生中断时调用此函数，根据 A 相引脚的电平状态判断旋转方向并更新计数
     */
    inline void pin_b_handler() noexcept
    {
        if (GPIO_ReadInputDataBit(pin_a.port, pin_a.pin) == 0)
        {
            count--;
        }
    }
};

/**
 * @brief 使用 STM32 内部硬件的旋转编码器接口
 * @note 只能使用通用/高级定时器的 CH1 和 CH2
 */
class RotaryEncoder
{
private:
    TIM_TypeDef *__TIMX;

public:
    RotaryEncoder(TIM_TypeDef *TIMX, uint8_t TIMx_REMAP, bool reverse = false) noexcept;
    inline int16_t get() noexcept
    {
        auto temp = TIM_GetCounter(__TIMX);
        TIM_SetCounter(__TIMX, 0);
        return temp;
    }
};

/**
 * @brief 外部定时器类，用于处理 STM32 的外部定时器功能
 *
 * 该类封装了外部定时器的配置和使用，通过指定的定时器外设和回音引脚来实现定时功能。
 * 主要用于超声波传感器等需要外部触发的定时测量场景。
 */
class OuterTimer
{
public:
    /**
     * @brief 构造一个外部定时器对象
     * @param TIMX 定时器外设指针
     * @param echo_pin 回音引脚（只能是规定的复用引脚）
     * @param times 次数
     * @param PreemptionPriority 抢占优先级，默认为 0
     * @param SubPriority 子优先级，默认为 0
     */
    OuterTimer(TIM_TypeDef *TIMX, GPIOPin echo_pin, /* 只能是规定的复用引脚 */ uint16_t times, /* 次数 */
               uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0) noexcept;
};

/**
 * @brief PWM 类，用于生成 PWM 信号控制设备接口
 *
 * 该类封装了 STM32 定时器的 PWM 功能，可以方便地配置和控制 PWM 输出。
 * 支持设置 PWM 频率、占空比和分辨率，并可动态调整占空比。
 *
 * @note
 * 1. 输出引脚是根据定时器和通道自动确定的，无需手动指定
 * 2. Reso_permillage 参数建议使用能被 1000 整除的值，以保证计算精度
 * 3. 占空比范围为 0-100 的整数百分比
 *
 */
class PWM
{
private:
    int _Duty_permillage;
    int _Freq_HZ;
    int _Reso_permillage;
    TIM_TypeDef *_TIMX;
    int _CHn;

    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP, GPIOPin output_pin /* 只能是规定的复用引脚 */,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept;

public:
    /**
     * @brief PWM 构造函数
     * @param TIMX 定时器外设指针
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     * @param Duty_permillage 初始占空比（0-1000），默认 0%
     * @param Freq_HZ PWM 频率（Hz），默认 1kHz
     * @param Reso_permillage PWM 分辨率（千分之一），默认 10‰
     *
     * @note 有一些重映像会导致构造函数自动关闭调试端口，具体查阅参考手册
     * @note Reso_permillage 建议能被 1000 整除，以确保计算准确性
     * @note output_pin 根据定时器、通道和重映像自动选择
     */
    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, _GEN_TIM_CH_TO_GPIOPin_REMAP[TIMx_REMAP][Get_TIM_Index(TIMX)][CHn - 1],
              Duty_permillage, Freq_HZ, Reso_permillage) {};
    /**
     * @brief 设置 PWM 占空比
     * @param Duty_permillage 占空比（0-100）
     *
     * @note 占空比为千分比值，0 表示始终低电平，100 表示始终高电平
     * @note 该函数会根据设定的分辨率自动计算并设置比较寄存器值
     */
    void set_duty(int Duty_permillage) noexcept;
    inline auto get_duty() const noexcept
    {
        return _Duty_permillage;
    }
};

/**
 * @brief 呼吸灯类，用于实现呼吸灯效果
 *
 */
class BreathingLED : public PWM
{
private:
    bool _driven_level;
    int _period_s;

public:
    /**
     * @brief 构造一个呼吸灯对象
     *
     * 呼吸灯是基于 PWM 实现的，通过周期性地改变 LED 亮度来模拟 "呼吸" 效果。
     *
     * @param TIMX 定时器外设指针，用于产生 PWM 信号
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     *                   - 0b00: 无重映像
     *                   - 0b01: 部分重映像 01
     *                   - 0b10: 部分重映像 10
     *                   - 0b11: 完全重映像
     * @param driven_level 驱动电平，false 表示低电平点亮 LED，true 表示高电平点亮 LED
     * @param period_s 呼吸周期（秒），即从暗到亮再到暗的完整周期时间
     *
     * @note 呼吸灯初始化时会自动设置 PWM 频率为 1kHz，分辨率为 10‰
     */
    BreathingLED(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
                 bool driven_level = false, int period_s = 5) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, driven_level ? 0 : 1000 /* 初期关闭 */, 1000, 10),
          _driven_level(driven_level), _period_s(period_s) {};

    /**
     * @brief 执行呼吸灯效果
     *
     * 控制 LED 按照设定的周期执行呼吸效果，即亮度从暗逐渐变亮，
     * 再从亮逐渐变暗的过程。
     *
     * @param times 呼吸次数，默认为 1 次
     *
     * @note 该函数是阻塞式的，会持续执行完指定次数的呼吸效果
     * @note 呼吸周期由构造函数中设置的_period_s 决定
     */
    void breathe(int times = 1) noexcept;
};

class SteeringEngine : public PWM
{
private:
    const static int duty_min = 30;                          //!< 占空比最小值（左闭）
    const static int duty_max = 130;                         //!< 占空比最大值（右开）
    const static int delta_duty = duty_max - duty_min;       //!< 占空比上下界差值
    const static int degree_min = 0;                         //!< 百分度最小值（左闭）
    const static int degree_max = 200;                       //!< 百分度最大值（右开）
    const static int delta_degree = degree_max - degree_min; //!< 百分度上下界差值

    int16_t __degree_gon; //!< 角度（格恩 / 百分度 / 0.9 度）

    inline static int16_t truncate_degree_gon(int16_t degree_gon) noexcept
    {
        return std::abs(degree_gon) % degree_max + degree_min;
    }

public:
    inline SteeringEngine(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, duty_min, 50, 10), __degree_gon(0) {};

    inline void set_degree_gon(int16_t degree_gon) noexcept
    {
        this->__degree_gon = degree_gon;
        this->set_duty(duty_min + truncate_degree_gon(degree_gon) * delta_duty / delta_degree);
    }

    inline void set_degree(double degree) noexcept
    {
        set_degree_gon(int16_t(degree / 0.9));
    }

    inline void clockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon - int16_t(degree / 0.9));
    }

    inline void anticlockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon + int16_t(degree / 0.9));
    }
};

class DCMotorDriver
{
EMBMARTIN_DEBUGING_SPECIFIER:
    PWM PWMA;
    PWM PWMB;
    OutPin STBY;
    OutPin AIN1;
    OutPin BIN1;
    OutPin AIN2;
    OutPin BIN2;

    int16_t current_left_speed_permillage = 0;
    int16_t current_right_speed_permillage = 0;

public:
    enum class Mode
    {
        STOP,
        BRAKE,
        FORWARD,
        REVERSE
    };
    inline DCMotorDriver(
        PWM _PWMA, OutPin _AIN1, OutPin _AIN2,
        OutPin STBY,
        PWM _PWMB, OutPin _BIN1, OutPin _BIN2) noexcept
        : PWMA{_PWMA}, AIN1{_AIN1}, AIN2{_AIN2},
          STBY{STBY},
          PWMB{_PWMB}, BIN1{_BIN1}, BIN2{_BIN2}
    {
        work();
        set_mode(Mode::FORWARD,0);
        set_mode(Mode::FORWARD,1);
    };

    inline DCMotorDriver(PWM _PWMA, OutPin _AIN1, OutPin _AIN2, OutPin STBY = null_pin /* 直接接到了 3V */) noexcept
        : DCMotorDriver(_PWMA, _AIN1, _AIN2, STBY,
                        PWM{nullptr, 0, 0}, OutPin{null_pin}, OutPin{null_pin}) {};

    inline void standby() noexcept
    {
        STBY.reset();
    }
    inline void work() noexcept
    {
        STBY.set();
    }
    void set_mode(Mode mode, uint8_t motor_index = 0) noexcept;
    void set_speed(int16_t speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept;
    inline void add_speed(int16_t delta_speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept
    {
        set_speed(
            get_speed(motor_index) + delta_speed_permillage,
            motor_index);
    }
    inline int16_t get_speed(uint8_t motor_index = 0) noexcept
    {
        return motor_index == 0 ? current_left_speed_permillage : current_right_speed_permillage;
    }

};

class I2C
{
private:
    GPIOPin _SCL; //!< I2C 时钟引脚
    GPIOPin _SDA; //!< I2C 数据引脚

    uint8_t _addr; //!< 从机地址
    inline void start() noexcept
    {
        /***************************************************************************************************
                 SCL
        XXXXXXXXXXXXXXXXXXXXXX
                             XX
                              X
                              XX
                               X
                               XX
                                X
        SDA                     XXXXXXXXXXXXXXXX
        XXXXXXXXXXXXXXXXXX
                         X
                         XX
                          X
                          X
                          XX
                           X
                           XXXXXXXXXXXXXXXXXXXX

        ****************************************************************************************************/
        _SDA.set();
        EMBMARTIN_KEEP_CODE_ORDER; // 防止与终止信号混淆
        _SCL.set();

        EMBMARTIN_KEEP_CODE_ORDER;
        _SDA.reset();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.reset();
    }

    inline void stop() noexcept
    {
        /***************************************************************************************************
                          XXXXXXXXXXXXXXXX
                         XX
                        XX
                        XX
         SCL           XX
        XXXXXXXXXXXXXXXXX


                                  XXXXXXXX
                                XXX
                               XXX
                              XX
        SDA                  XX
        XXXXXXXXXXXXXXXXXXXXXXX

        ****************************************************************************************************/
        // 契约：确保 SCL 必定已经是低电平
        _SDA.reset();

        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.set();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SDA.set();
    }

    void send_byte(uint8_t data) noexcept;
    inline void send_ack(bool data) noexcept
    {
        data ? _SDA.set() : _SDA.reset();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.set();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.reset();
    }

    uint8_t receive_byte() noexcept;
    inline void receive_ack() noexcept
    {
        bool rslt;

        _SDA.set(); // 释放 SDA
        EMBMARTIN_KEEP_CODE_ORDER;

        _SCL.set();
        EMBMARTIN_KEEP_CODE_ORDER;
        rslt = _SDA.read();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.reset();

        EMBMARTIN_ASSERT(!rslt,"I2C req exception", &oled);
    }

public:
    /**
     * @brief 自动完成需要的所有初始化
     * @param SCL I2C 时钟线引脚
     * @param SDA I2C 数据线引脚
     * @param addr 从机地址，第 0 位强制置 0
     */
    inline I2C(GPIOPin SCL, GPIOPin SDA, uint8_t addr) noexcept
        : _SCL(SCL), _SDA(SDA), _addr(addr & 0b11111110)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SCL.port), ENABLE);
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SDA.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = SCL.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_OD,
        };
        GPIO_Init(SCL.port, &GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = SDA.pin;
        GPIO_Init(SDA.port, &GPIO_InitStruct);

        GPIO_SetBits(SCL.port, SCL.pin);
        GPIO_SetBits(SDA.port, SDA.pin);
    }

    template <typename... Args>
    void write_reg(uint8_t first_reg_addr, Args... data) noexcept
    {
        start();
        send_byte(_addr);
        receive_ack();
        send_byte(first_reg_addr);
        receive_ack();

        ((send_byte(*reinterpret_cast<const uint8_t *>(&data) /* 按位映射为 8 位无符号数 */), receive_ack()), ...);

        stop();
    }

    template <typename _Container_OR_NUM>
    void write_reg(uint8_t first_reg_addr, const _Container_OR_NUM &data) noexcept
    {
        if constexpr (has_iterator_v<_Container_OR_NUM>)
        {
            start();
            send_byte(_addr);
            receive_ack();
            send_byte(first_reg_addr);
            receive_ack();

            for (auto byte : data)
            {
                send_byte(*reinterpret_cast<const uint8_t *>(&byte));
                receive_ack();
            }

            stop();
        }
        else if constexpr (std::is_arithmetic_v<_Container_OR_NUM>)
        {
            const auto &reg_addr = first_reg_addr;
            start();
            send_byte(_addr);
            receive_ack();
            send_byte(reg_addr);
            receive_ack();
            send_byte(*reinterpret_cast<const uint8_t *>(&data));
            receive_ack();
            stop();
        }
        else
        {
            static_assert(false, "必须传入算术或可迭代类型");
        }
    }

    uint8_t read_reg(uint8_t reg_addr) noexcept;

    template <size_t N>
    std::array<uint8_t, N> read_reg(uint8_t reg_addr) noexcept
    {
        std::array<uint8_t, N> result;

        // 指定寄存器地址
        start();
        send_byte(_addr);
        receive_ack();
        send_byte(reg_addr);
        receive_ack();

        // 进入读模式
        start();
        send_byte(_addr | 0x01);
        receive_ack();

        // 读取多个字节
        for (size_t i = 0; i < N; ++i)
        {
            bool send_nack = (i == N - 1);
            result[i] = receive_byte();
            send_ack(send_nack);
        }

        stop();
        return result;
    }
};

class MPU6050 : public I2C
{
private:
public:
    /**
     * @brief 仿枚举类，内含MPU6050所有寄存器对应编号
     *
     */
    struct REGS
    {
        enum Values : uint8_t
        {
            // 辅助 I2C 电源选择
            AUX_VDDIO = 0x01,

            // 采样率分频器
            SMPLRT_DIV = 0x19,

            // 配置寄存器
            CONFIG = 0x1A,

            // 陀螺仪配置
            GYRO_CONFIG = 0x1B,

            // 加速度计配置
            ACCEL_CONFIG = 0x1C,

            // 自由落体检测
            FF_THR = 0x1D, // 自由落体加速度阈值
            FF_DUR = 0x1E, // 自由落体持续时间

            // 运动检测
            MOT_THR = 0x1F, // 运动检测阈值
            MOT_DUR = 0x20, // 运动检测持续时间

            // 零运动检测
            ZRMOT_THR = 0x21, // 零运动检测阈值
            ZRMOT_DUR = 0x22, // 零运动检测持续时间

            // FIFO 使能
            FIFO_EN = 0x23,

            // I2C 主控制
            I2C_MST_CTRL = 0x24,

            // I2C 从设备 0 控制
            I2C_SLV0_ADDR = 0x25,
            I2C_SLV0_REG = 0x26,
            I2C_SLV0_CTRL = 0x27,

            // I2C 从设备 1 控制
            I2C_SLV1_ADDR = 0x28,
            I2C_SLV1_REG = 0x29,
            I2C_SLV1_CTRL = 0x2A,

            // I2C 从设备 2 控制
            I2C_SLV2_ADDR = 0x2B,
            I2C_SLV2_REG = 0x2C,
            I2C_SLV2_CTRL = 0x2D,

            // I2C 从设备 3 控制
            I2C_SLV3_ADDR = 0x2E,
            I2C_SLV3_REG = 0x2F,
            I2C_SLV3_CTRL = 0x30,

            // I2C 从设备 4 控制
            I2C_SLV4_ADDR = 0x31,
            I2C_SLV4_REG = 0x32,
            I2C_SLV4_DO = 0x33,
            I2C_SLV4_CTRL = 0x34,
            I2C_SLV4_DI = 0x35,

            // I2C 主状态
            I2C_MST_STATUS = 0x36,

            // 中断引脚 / 旁路使能配置
            INT_PIN_CFG = 0x37,

            // 中断使能
            INT_ENABLE = 0x38,

            // 中断状态
            INT_STATUS = 0x3A,

            // 加速度计测量值
            ACCEL_XOUT_H = 0x3B,
            ACCEL_XOUT_L = 0x3C,
            ACCEL_YOUT_H = 0x3D,
            ACCEL_YOUT_L = 0x3E,
            ACCEL_ZOUT_H = 0x3F,
            ACCEL_ZOUT_L = 0x40,

            // 温度测量值
            TEMP_OUT_H = 0x41,
            TEMP_OUT_L = 0x42,

            // 陀螺仪测量值
            GYRO_XOUT_H = 0x43,
            GYRO_XOUT_L = 0x44,
            GYRO_YOUT_H = 0x45,
            GYRO_YOUT_L = 0x46,
            GYRO_ZOUT_H = 0x47,
            GYRO_ZOUT_L = 0x48,

            // 外部传感器数据 (Slave 0-3)
            EXT_SENS_DATA_00 = 0x49,
            EXT_SENS_DATA_01 = 0x4A,
            EXT_SENS_DATA_02 = 0x4B,
            EXT_SENS_DATA_03 = 0x4C,
            EXT_SENS_DATA_04 = 0x4D,
            EXT_SENS_DATA_05 = 0x4E,
            EXT_SENS_DATA_06 = 0x4F,
            EXT_SENS_DATA_07 = 0x50,
            EXT_SENS_DATA_08 = 0x51,
            EXT_SENS_DATA_09 = 0x52,
            EXT_SENS_DATA_10 = 0x53,
            EXT_SENS_DATA_11 = 0x54,
            EXT_SENS_DATA_12 = 0x55,
            EXT_SENS_DATA_13 = 0x56,
            EXT_SENS_DATA_14 = 0x57,
            EXT_SENS_DATA_15 = 0x58,
            EXT_SENS_DATA_16 = 0x59,
            EXT_SENS_DATA_17 = 0x5A,
            EXT_SENS_DATA_18 = 0x5B,
            EXT_SENS_DATA_19 = 0x5C,
            EXT_SENS_DATA_20 = 0x5D,
            EXT_SENS_DATA_21 = 0x5E,
            EXT_SENS_DATA_22 = 0x5F,
            EXT_SENS_DATA_23 = 0x60,

            // 运动检测状态
            MOT_DETECT_STATUS = 0x61,

            // I2C 从设备数据输出
            I2C_SLV0_DO = 0x63,
            I2C_SLV1_DO = 0x64,
            I2C_SLV2_DO = 0x65,
            I2C_SLV3_DO = 0x66,

            // I2C 主延迟控制
            I2C_MST_DELAY_CTRL = 0x67,

            // 信号路径复位
            SIGNAL_PATH_RESET = 0x68,

            // 运动检测控制
            MOT_DETECT_CTRL = 0x69,

            // 用户控制
            USER_CTRL = 0x6A,

            // 电源管理
            PWR_MGMT_1 = 0x6B,
            PWR_MGMT_2 = 0x6C,

            // FIFO 计数
            FIFO_COUNTH = 0x72,
            FIFO_COUNTL = 0x73,

            // FIFO 读写
            FIFO_R_W = 0x74,

            // 设备 ID
            WHO_AM_I = 0x75
        };
    };

    struct Data
    {
        int16_t acc_x, acc_y, acc_z;
        int16_t gyro_x, gyro_y, gyro_z;
    };
    inline MPU6050(
        GPIOPin SCL, GPIOPin SDA, bool AD0 = 0, /* 是否更改地址 */
        bool gyroscope_enable = 1, uint8_t SMPRT_DIV = 8 /* 分频数 */,
        uint8_t ACCEL_AFS_SCL = 0b01 /* 加速度满量程选择 0-3*/,
        uint8_t GYRO_FS_SEL = 0b10 /* 角速度满量程选择 0-3*/) noexcept
        : I2C{SCL, SDA, uint8_t(0xD0 + (AD0 << 1))}
    {
        write_reg(REGS::PWR_MGMT_1, gyroscope_enable);     // 解除休眠，是否启用陀螺仪作为时钟源
        write_reg(REGS::PWR_MGMT_2, 0x00);                 // 六轴都工作
        write_reg(REGS::SMPLRT_DIV, SMPRT_DIV - 1);        // 设置采样率
        write_reg(REGS::CONFIG, 0x00);                     // 平滑地滤波
        write_reg(REGS::ACCEL_CONFIG, ACCEL_AFS_SCL << 3); // 4g 满量程
        write_reg(REGS::GYRO_CONFIG, GYRO_FS_SEL << 3);    // 1000°/s 满量程
    };
    inline void awake() noexcept
    {
        write_reg(REGS::PWR_MGMT_1, read_reg(REGS::PWR_MGMT_1) & 0b1011'1111);
    }

    inline void sleep() noexcept
    {
        write_reg(REGS::PWR_MGMT_1, read_reg(REGS::PWR_MGMT_1) | 0b0100'0000);
    }

    inline auto get_id() noexcept
    {
        return read_reg(REGS::WHO_AM_I);
    }

    inline Data get_data() noexcept
    {
        Data merge_rslt;

        auto data_acc = read_reg<6>(REGS::ACCEL_XOUT_H);
        auto data_gyro = read_reg<6>(REGS::GYRO_XOUT_H);

        merge_rslt.acc_x = (uint16_t(std::get<0>(data_acc)) << 8) | std::get<1>(data_acc);
        merge_rslt.acc_y = (uint16_t(std::get<2>(data_acc)) << 8) | std::get<3>(data_acc);
        merge_rslt.acc_z = (uint16_t(std::get<4>(data_acc)) << 8) | std::get<5>(data_acc);

        merge_rslt.gyro_x = (uint16_t(std::get<0>(data_gyro)) << 8) | std::get<1>(data_gyro);
        merge_rslt.gyro_y = (uint16_t(std::get<2>(data_gyro)) << 8) | std::get<3>(data_gyro);
        merge_rslt.gyro_z = (uint16_t(std::get<4>(data_gyro)) << 8) | std::get<5>(data_gyro);

        return merge_rslt;
    }
};

class AngleComplementaryFilter
{
private:
    const double alpha;            //!< 互补滤波系数
    const double dt;               //!< 采样时间间隔（秒）
    const MPU6050::Data &MPU_data; //!< MPU6050 数据引用

    double current_pitch = 0.0; //!< 当前俯仰角（度）
    double current_roll = 0.0;  //!< 当前横滚角（度）
    double current_yaw = 0.0;   //!< 当前偏航角（度）
public:
    struct Status
    {
        double pitch; //!< 俯仰角（度）
        double roll;  //!< 横滚角（度）
        double yaw;   //!< 偏航角（度）
    };
    AngleComplementaryFilter(
        MPU6050::Data &_MPU_data,
        double _alpha = 0.001, double _dt = 0.001) noexcept
        : alpha(_alpha), dt(_dt), MPU_data(_MPU_data) {};

    void update() noexcept; // 在中断函数或循环中定期调用以更新角度数据
    inline Status get_status() const noexcept { return {current_pitch, current_roll, current_yaw}; }
};

class SingleLoopPID
{
private:
    const double kp;             //!< 比例系数
    const double ki;             //!< 积分系数
    const double kd;             //!< 微分系数
    const double integral_limit; //!< 积分限幅值, 小于=0表示不限制
    const double output_limit;   //!< 输出限幅值，小于=0表示不限制
    const double alpha;          //!< 误差滤波系数

    const double &current; //!< 当前值引用
    const double dt;       //!< 采样时间间隔（秒）

    const int16_t* d_current; // 可选：直接使用现成的微分值

    double previous_error = 0.0; //!< 上一次误差值
    double integral = 0.0;       //!< 积分值

public:
    inline SingleLoopPID(
        double _kp, double _ki, double _kd, double &_current, double _dt,
        double _integral_limit, double _output_limit, double _alpha = 0.0 /*默认不滤波*/, const int16_t* _d_current = nullptr) noexcept
        : kp(_kp), ki(_ki), kd(_kd), current(_current), dt(_dt),
          integral_limit(_integral_limit), output_limit(_output_limit), alpha(std::clamp(_alpha, 0.0, 1.0)), d_current(_d_current) {};

    double compute(double target) noexcept; // 中断函数中调用

    inline void reinit()noexcept
    {
        this->previous_error = 0;
        this->integral = 0;
    }
};

struct PIDParams
{
    double kp;
    double ki;
    double kd;
    double integral_limit;
    double output_limit;
    double alpha = 0.0; // 误差滤波系数，默认不滤波
};

class BalancedCarPID
{
private:
    // 数据获取
    const MPU6050::Data &MPU_data; //!< MPU6050 数据引用
    const int16_t &pace_left;      //!< 左轮编码器数据引用
    const int16_t &pace_right;     //!< 右轮编码器数据引用
    // 互补滤波器
    AngleComplementaryFilter angle_filter;         //!< 角度互补滤波器
    AngleComplementaryFilter::Status angle_status; //!< 当前角度状态,每次compute更新
    const double pitch_med_angle;                 //!< 平衡车静止时的俯仰角（度）
    // PID 控制器
    SingleLoopPID vertical; //!< 垂直方向 PID 控制器
    SingleLoopPID velocity; //!< 速度方向 PID 控制器
    SingleLoopPID turn;     //!< 转向方向 PID 控制器
    // 缓存值（每次compute更新）
    double velocity_pid_current; //!< 速度 PID current 成员
    double turn_pid_current;     //!< 转向 PID current 成员

public:
    struct Output
    {
        double duty_left_permillage;  //!< 左轮占空比千分比
        double duty_right_permillage; //!< 右轮占空比千分比
    };
    inline BalancedCarPID(MPU6050::Data &_MPU_data,
                          int16_t &_pace_left, int16_t &_pace_right,
                          PIDParams vertical_params, PIDParams velocity_params, PIDParams turn_params,
                          double dt /* 采样时间间隔（秒） */, double _pitch_med_angle) noexcept
        : MPU_data(_MPU_data),
          pace_left(_pace_left), pace_right(_pace_right),
          angle_filter(_MPU_data, 0.001, dt),
          vertical(vertical_params.kp, vertical_params.ki, vertical_params.kd,
                   this->angle_status.pitch, dt,
                   vertical_params.integral_limit, vertical_params.output_limit, vertical_params.alpha,
                &this->MPU_data.gyro_y),
          velocity(velocity_params.kp, velocity_params.ki, velocity_params.kd,
                   this->velocity_pid_current, dt,
                   velocity_params.integral_limit, velocity_params.output_limit, velocity_params.alpha),
          turn(turn_params.kp, turn_params.ki, turn_params.kd,
               this->turn_pid_current, dt,
               turn_params.integral_limit, turn_params.output_limit, turn_params.alpha),
                pitch_med_angle(_pitch_med_angle) {};

    Output compute(double turn_trg, double velocity_trg) noexcept; // 中断函数中调用

    
};

EMBMARTIN_STM32F10X_NAMESPACE_END




#endif // EMBMARTIN_TOOLS_H