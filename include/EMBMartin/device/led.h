#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/gpio/outpin.h"
EMBMARTIN_DEVICE_NAMESPACE_BEGIN

/**
 * @brief LED 类，用于控制 STM32 GPIO 连接的 LED
 *
 * 该类封装了 LED 的 GPIO 配置和控制功能，支持点亮、熄灭和翻转操作。
 */
class LED
{
private:
    hal::OutPin output;     //!< 物理输出引脚
    bool driven_level; //!< 驱动电平，false 表示低电平点亮，true 表示高电平点亮

public:
    /**
     * @brief 构造一个 LED 对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    LED(hal::GPIOPin p, bool driven_level = false) noexcept
        : output(p), driven_level(driven_level)
    {
        this->off(); // 初始化时熄灭 LED
    }

    /**
     * @brief 构造一个 LED 对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     */
    LED(GPIO_TypeDef *port, uint16_t pin, bool driven_level = false) noexcept
        : LED{hal::GPIOPin{port, pin}, driven_level} {}

    /**
     * @brief 点亮 LED
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相应电平以点亮 LED
     */
    void on() const noexcept
    {
        if (this->driven_level)
            this->output.set();
        else
            this->output.reset();
    }
    /**
     * @brief 熄灭 LED
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相反电平以熄灭 LED
     */
    void off() const noexcept
    {
        if (this->driven_level)
            this->output.reset();
        else
            this->output.set();
    }

    /**
     * @brief 切换 LED 状态
     *
     * 翻转当前 LED 的状态，如果当前是点亮则熄灭，如果当前是熄灭则点亮
     */
    inline void toggle() const noexcept
    {
        this->output.toggle();
    }

    inline operator bool() const noexcept
    {
        return this->output.get_level() == this->driven_level;
    }

    const LED &operator=(bool value) const noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }

    LED &operator=(bool value) noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }
};

EMBMARTIN_DEVICE_NAMESPACE_END