#include "inpin.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

Key::Key(GPIOPin p, bool trigger) noexcept
    : input(p, trigger ? GPIO_Mode_IPD : GPIO_Mode_IPU), trigger(trigger)
{
}

Key::Key(GPIO_TypeDef *port, uint16_t pin, bool trigger) noexcept : Key{GPIOPin{port, pin}, trigger} {}

Key::operator bool() const noexcept
{
    return this->is_pressed();
}

Sensor::Sensor(GPIO_TypeDef *port, uint16_t pin) noexcept : Sensor{GPIOPin{port, pin}} {}

EMBMARTIN_STM32F10X_NAMESPACE_END
