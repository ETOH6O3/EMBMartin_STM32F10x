/**
 ******************************************************************************
 * @file    rotary_encoder.h
 * @author  孙鸣淼
 * @brief   旋转编码器驱动：外部中断型与硬件定时器型
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 *
 * 本文件由 hal/peripheral/EXTI.h 与 hal/peripheral/TIM.h 拆出，
 * 依赖方向：本文件 -> hal/peripheral/EXTI.h / hal/peripheral/TIM.h（单向）。
 ******************************************************************************
 */

#pragma once

#include "EMBMartin/macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "EMBMartin/hal/gpio/basic_tools.h"
#include "EMBMartin/hal/peripheral/EXTI.h"
#include "EMBMartin/hal/peripheral/TIM.h"

EMBMARTIN_DEVICE_NAMESPACE_BEGIN

/**
 * @brief 旋转编码器类，用于处理正交编码器输入
 *
 * 该类封装了旋转编码器的配置和处理功能，通过两个相位信号 (A 相和 B 相) 来检测旋转方向和计数。
 * 适用于机械旋转编码器等需要旋转位置检测的设备。
 */
class EXTIRotaryEncoder
{
private:
    const hal::EXTIManager exti_a; //!< A 相外部中断管理器
    const hal::EXTIManager exti_b; //!< B 相外部中断管理器
    int16_t count = 0;        //!< 当前计数值

    volatile int16_t speed = 0;                         //!< 上一个采样周期内的计数增量（变速）
    hal::InnerTimerManager::Handle __speed_handle = {}; //!< 采样回调在定时器队列里的句柄
    const uint16_t __sample_interval = 1;               //!< 每隔多少个节拍采样一次速度
    uint16_t __sample_cnt = 0;                          //!< 距上次采样已过的节拍数

    /**
     * @brief 定时器节拍回调：按分频把计数锁存为速度并清零
     */
    inline void __sample() noexcept
    {
        if (++__sample_cnt >= __sample_interval)
        {
            __sample_cnt = 0;
            speed = count;
            count = 0;
        }
    }

    /**
     * @brief A 相引脚中断处理函数
     *
     * 当 A 相引脚发生中断时调用此函数，根据 B 相引脚的电平状态判断旋转方向并更新计数
     */
    inline void pin_a_handler() noexcept
    {
        if (exti_b.get_pin() == 0)
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
        if (exti_a.get_pin() == 0)
        {
            count--;
        }
    }

public:
    /**
     * @brief 构造一个旋转编码器对象
     * @param pin_a A 相引脚 GPIOPin 结构体
     * @param pin_b B 相引脚 GPIOPin 结构体
     * @param PreemptionPriority 中断抢占优先级，默认为 0
     * @param SubPriority 中断子优先级，默认为 0
     * @param _EXTI_Mode EXTI 模式，默认为 EXTI_Mode_Interrupt (中断模式)
     * @param TIMX 采样速度用的中断定时器；为 nullptr 时不自动采样，get_speed() 退化为按需采样
     * @param sample_interval_cnt 每隔多少个节拍采样一次速度，默认为 1（每个节拍都采样）
     */
    inline EXTIRotaryEncoder(
        hal::GPIOPin pin_a, hal::GPIOPin pin_b,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt,
        TIM_TypeDef *TIMX = nullptr, uint16_t sample_interval_cnt = 1) noexcept
        : exti_a{pin_a, [&]()
                 { this->pin_a_handler(); },
                 EXTI_Trigger_Falling, PreemptionPriority, SubPriority,
                 _EXTI_Mode},
          exti_b{pin_b, [&]()
                 { this->pin_b_handler(); },
                 EXTI_Trigger_Falling, PreemptionPriority, SubPriority,
                 _EXTI_Mode},
          __sample_interval{sample_interval_cnt}
    {
        if (TIMX != nullptr)
        {
            __speed_handle = hal::InnerTimerManager::add(TIMX, [this]()
                                                         { __sample(); });
        }
    }

    /// 析构时注销速度采样
    inline ~EXTIRotaryEncoder() noexcept
    {
        hal::InnerTimerManager::remove(__speed_handle);
    }

    /**
     * @brief 获取当前计数值
     * @return int16_t 当前计数值
     */
    inline int16_t get_count() const noexcept { return count; }

    /**
     * @brief 获取速度
     *
     * @return int16_t 当前速度
     */
    inline int16_t get_speed() noexcept
    {
        return speed;
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

EMBMARTIN_DEVICE_NAMESPACE_END
