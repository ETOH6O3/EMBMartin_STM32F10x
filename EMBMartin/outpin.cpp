#include "outpin.h"

#include "Delay.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

OutPin::OutPin(GPIOPin p) noexcept : pin(p), current_level(0)
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

OutPin& OutPin::operator=(bool value) noexcept
{
    if (pin == null_pin)
        return *this;
    if (value)
        this->set();
    else
        this->reset();
    return *this;
}

LED::LED(GPIOPin p, bool driven_level) noexcept : output(p), driven_level(driven_level)
{
    this->off(); // 初始化时熄灭 LED
}

LED::LED(GPIO_TypeDef *port, uint16_t pin, bool driven_level) noexcept : LED{GPIOPin{port, pin}, driven_level} {}

void LED::on() const noexcept
{
    if (this->driven_level)
        this->output.set();
    else
        this->output.reset();
}

void LED::off() const noexcept
{
    if (this->driven_level)
        this->output.reset();
    else
        this->output.set();
}

const LED& LED::operator=(bool value) const noexcept
{
    if (value)
        this->on();
    else
        this->off();
    return *this;
}

LED& LED::operator=(bool value) noexcept
{
    if (value)
        this->on();
    else
        this->off();
    return *this;
}

Buzzer::Buzzer(GPIOPin p, bool driven_level) noexcept : output(p), driven_level(driven_level)
{
    this->off(); // 初始化时关闭蜂鸣器
}

Buzzer::Buzzer(GPIO_TypeDef *port, uint16_t pin, bool driven_level) noexcept : Buzzer{GPIOPin{port, pin}, driven_level} {}

void Buzzer::on() const noexcept
{
    if (this->driven_level)
        this->output.set();
    else
        this->output.reset();
}

void Buzzer::off() const noexcept
{
    if (this->driven_level)
        this->output.reset();
    else
        this->output.set();
}

void Buzzer::beep(intmax_t duration_ms) const noexcept
{
    this->on();
    Delay_ms(duration_ms);
    this->off();
}

void Buzzer::beep(intmax_t times, intmax_t duration_ms, intmax_t interval_ms) const noexcept
{
    for (intmax_t i = 0; i < times - 1; i++)
    {
        this->beep(duration_ms);
        Delay_ms(interval_ms);
    }
    this->beep(duration_ms);
}

const Buzzer& Buzzer::operator=(bool value) const noexcept
{
    if (value)
        this->on();
    else
        this->off();
    return *this;
}

Buzzer& Buzzer::operator=(bool value) noexcept
{
    if (value)
        this->on();
    else
        this->off();
    return *this;
}

EMBMARTIN_STM32F10X_NAMESPACE_END
