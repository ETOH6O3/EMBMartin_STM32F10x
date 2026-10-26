#include "EMBMartin/hal/gpio/outpin.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

OutPin::OutPin(GPIOPin p) noexcept : pin(p), current_level(0)
{
    if (p == null_pin)
        return;

    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);

    if (p == PB4)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
    }
    else if ((p == PA15) || (p == PB3))
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    }
    else if ((p == PA13) || (p == PA14))
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
    }

    GPIO_InitTypeDef GPIO_InitStruct = {
        .GPIO_Pin = p.pin,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP,
    };
    GPIO_Init(p.port, &GPIO_InitStruct);

    this->reset();
}

void OutPin::set() const noexcept
{
    if (pin == null_pin)
        return;
    GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_SET);
    current_level = true;
}

void OutPin::reset() const noexcept
{
    if (pin == null_pin)
        return;
    GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_RESET);
    current_level = false;
}

void OutPin::toggle() const noexcept
{
    if (pin == null_pin)
        return;
    GPIO_WriteBit(this->pin.port, this->pin.pin, current_level ? Bit_RESET : Bit_SET);
    current_level = !current_level;
}

OutPin &OutPin::operator=(bool value) noexcept
{
    if (pin == null_pin)
        return *this;
    if (value)
        this->set();
    else
        this->reset();
    return *this;
}

EMBMARTIN_STM32F10X_NAMESPACE_END
