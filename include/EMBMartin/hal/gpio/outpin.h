#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/gpio/basic_tools.h"
#include "__Delay.h"

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
    GPIOPin pin;                //!< GPIO 引脚信息
    mutable bool current_level; //!< 目前输出
public:
    OutPin(GPIOPin p) noexcept;

    void set() const noexcept;
    void reset() const noexcept;
    void toggle() const noexcept;

    inline bool get_level() const noexcept
    {
        return current_level;
    }

    OutPin &operator=(bool value) noexcept;

    inline operator bool() const noexcept
    {
        return current_level;
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END
