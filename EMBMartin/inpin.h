#ifndef EMBMARTIN_INPIN_H
#define EMBMARTIN_INPIN_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

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


EMBMARTIN_STM32F10X_NAMESPACE_END

#endif // EMBMARTIN_INPIN_H