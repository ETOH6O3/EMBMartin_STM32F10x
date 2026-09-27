#ifndef EMBMARTIN_OUTPIN_H
#define EMBMARTIN_OUTPIN_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "meta.h"
#include "basic_tools.h"
#include "system.h"
EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

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
    mutable bool current_level; //!< 目前输出
public:
    inline OutPin(GPIOPin p) noexcept : pin(p), current_level(0)
    {
        if (p == null_pin)
            return;

        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        if (p == PB4)
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
        else if ((p == PA15) || (p == PB3))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
        else if ((p == PA13) || (p == PA14))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->reset();
    }
    inline void set() const noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_SET);
        current_level = true;
    }
    inline void reset() const noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_RESET);
        current_level = false;
    }
    inline void toggle() const noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, current_level ? Bit_RESET : Bit_SET);
        current_level = !current_level;
    }
    inline bool get_level() const noexcept
    {
        return current_level;
    }

    inline OutPin& operator=(bool value) noexcept
    {
        if (pin == null_pin)
            return *this;
        if (value)
            this->set();
        else
            this->reset();
        return *this;
    }

    inline operator bool() const noexcept
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
    OutPin output;     //!< 物理输出引脚
    bool driven_level; //!< 驱动电平，false 表示低电平点亮，true 表示高电平点亮

public:
    /**
     * @brief 构造一个 LED 对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline LED(GPIOPin p, bool driven_level = false) noexcept : output(p), driven_level(driven_level)
    {
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
    inline void off() const noexcept
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

    inline const LED& operator=(bool value) const noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }

    inline LED& operator=(bool value) noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
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
    OutPin output;     //!< 物理输出引脚
    bool driven_level; //!< 驱动电平，false 表示低电平驱动，true 表示高电平驱动

public:
    /**
     * @brief 构造一个蜂鸣器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline Buzzer(GPIOPin p, bool driven_level = false) noexcept : output(p), driven_level(driven_level)
    {
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

    inline const Buzzer& operator=(bool value) const noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }

    inline Buzzer& operator=(bool value) noexcept
    {
        if (value)
            this->on();
        else
            this->off();
        return *this;
    }
};
EMBMARTIN_STM32F10X_NAMESPACE_END

#endif // EMBMARTIN_OUTPIN_H