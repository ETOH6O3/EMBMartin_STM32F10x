#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/gpio/outpin.h"

EMBMARTIN_DEVICE_NAMESPACE_BEGIN
/**
 * @brief 蜂鸣器类，用于控制 STM32 GPIO 连接的蜂鸣器
 *
 * 该类封装了蜂鸣器的 GPIO 配置和控制功能，支持基本的开关控制以及蜂鸣操作。
 */
class Buzzer
{
private:
    hal::OutPin output;     //!< 物理输出引脚
    bool driven_level; //!< 驱动电平，false 表示低电平驱动，true 表示高电平驱动

public:
    /**
     * @brief 构造一个蜂鸣器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline Buzzer(hal::GPIOPin p, bool driven_level = false) noexcept : output(p), driven_level(driven_level)
    {
        this->off(); // 初始化时关闭蜂鸣器
    }

    /**
     * @brief 构造一个蜂鸣器对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     */
    inline Buzzer(GPIO_TypeDef *port, uint16_t pin, bool driven_level = false) noexcept : Buzzer{hal::GPIOPin{port, pin}, driven_level} {}

    /**
     * @brief 打开蜂鸣器
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相应电平以打开蜂鸣器
     */
    inline void on() const noexcept
    {
        if (this->driven_level)
            this->output.set();
        else
            this->output.reset();
    }

    /**
     * @brief 关闭蜂鸣器
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相反电平以关闭蜂鸣器
     */
    inline void off() const noexcept
    {
        if (this->driven_level)
            this->output.reset();
        else
            this->output.set();
    }

    /**
     * @brief 切换蜂鸣器状态
     *
     * 翻转当前蜂鸣器的状态，如果当前是开启则关闭，如果当前是关闭则开启
     */
    inline void toggle() const noexcept
    {
        this->output.toggle();
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

    inline operator bool() const noexcept
    {
        return this->output.get_level() == this->driven_level;
    }

    inline const Buzzer &operator=(bool value) const noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }

    inline Buzzer &operator=(bool value) noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }
};
EMBMARTIN_DEVICE_NAMESPACE_END
