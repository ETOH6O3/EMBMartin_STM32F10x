#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/gpio/inpin.h"

EMBMARTIN_DEVICE_NAMESPACE_BEGIN

/**
 * @brief 传感器类，用于读取 STM32 GPIO 连接的传感器状态
 *
 * 该类封装了传感器的 GPIO 配置和状态读取功能，适用于简单的数字传感器。
 */
class Sensor
{
private:
    hal::Inpin input; //!< 输入引脚

public:
    /**
     * @brief 构造一个传感器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     */
    inline Sensor(hal::GPIOPin p) noexcept : input(p, GPIO_Mode_IPU)
    {
    }

    /**
     * @brief 构造一个传感器对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     */
    Sensor(GPIO_TypeDef *port, uint16_t pin) noexcept : Sensor{hal::GPIOPin{port, pin}} {}

    /**
     * @brief 获取传感器状态
     * @return bool 传感器状态，高电平返回 true，低电平返回 false
     */
    inline bool get() const noexcept
    {
        return this->input.get();
    }

    inline operator bool() const noexcept
    {
        return this->get();
    }
};

EMBMARTIN_DEVICE_NAMESPACE_END