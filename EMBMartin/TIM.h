#ifndef EMBMARTIN_EXTI_H
#define EMBMARTIN_EXTI_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"


EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

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

EMBMARTIN_STM32F10X_NAMESPACE_END


#include "PWM.h" // 理论上属于

#endif // EMBMARTIN_EXTI_H