#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/gpio/basic_tools.h"
#include "cdelay.h"

EMBMARTIN_HAL_NAMESPACE_BEGIN

/**
 * @brief 通用输入引脚封装，负责 GPIO 输入模式初始化和电平读取
 */
class Inpin
{
private:
    GPIOPin pin;

public:
    Inpin(const GPIOPin &p, GPIOMode_TypeDef mode = GPIO_Mode_IPU) noexcept;

    inline Inpin(GPIO_TypeDef *port, uint16_t pin,
                 GPIOMode_TypeDef mode = GPIO_Mode_IPU) noexcept
        : Inpin{GPIOPin{port, pin}, mode}
    {
    }

    inline bool get() const noexcept
    {
        return GPIO_ReadInputDataBit(this->pin.port, this->pin.pin) == Bit_SET;
    }

    inline const GPIOPin &get_pin() const noexcept
    {
        return this->pin;
    }

    inline operator bool() const noexcept
    {
        return this->get();
    }

    inline auto idx() const noexcept
    {
        return __decode_onehot(this->pin.pin);
    }
};

EMBMARTIN_HAL_NAMESPACE_END
