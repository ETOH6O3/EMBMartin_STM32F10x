#include "EMBMartin/hal/gpio/inpin.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN
Inpin::Inpin(const EMBMartin::STM32::GPIOPin &p, GPIOMode_TypeDef mode) noexcept : pin(p)
{
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
        .GPIO_Mode = mode,
    };
    GPIO_Init(p.port, &GPIO_InitStruct);
}

EMBMARTIN_STM32F10X_NAMESPACE_END
