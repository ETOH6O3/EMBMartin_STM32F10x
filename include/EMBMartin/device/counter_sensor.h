/**
 ******************************************************************************
 * @file    counter_sensor.h
 * @author  孙鸣淼
 * @brief   基于外部中断的计数传感器驱动
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 *
 * 本文件由 hal/peripheral/TIM.h 拆出（原类名不变）。
 * 依赖方向：本文件 -> hal/gpio/basic_tools.h（单向）。
 ******************************************************************************
 */

#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/gpio/basic_tools.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

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

EMBMARTIN_STM32F10X_NAMESPACE_END
