#include "basic_tools.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

uint8_t Get_GPIO_PortSource(GPIO_TypeDef *port) noexcept
{
#ifdef GPIOA
    if (port == GPIOA)
    {
        return GPIO_PortSourceGPIOA;
    }
#endif
#ifdef GPIOB
    if (port == GPIOB)
    {
        return GPIO_PortSourceGPIOB;
    }
#endif
#ifdef GPIOC
    if (port == GPIOC)
    {
        return GPIO_PortSourceGPIOC;
    }
#endif
#ifdef GPIOD
    if (port == GPIOD)
    {
        return GPIO_PortSourceGPIOD;
    }
#endif
#ifdef GPIOE
    if (port == GPIOE)
    {
        return GPIO_PortSourceGPIOE;
    }
#endif
#ifdef GPIOF
    if (port == GPIOF)
    {
        return GPIO_PortSourceGPIOF;
    }
#endif
#ifdef GPIOG
    if (port == GPIOG)
    {
        return GPIO_PortSourceGPIOG;
    }
#endif
#ifdef GPIOH
    if (port == GPIOH)
    {
        return GPIO_PortSourceGPIOH;
    }
#endif
#ifdef GPIOI
    if (port == GPIOI)
    {
        return GPIO_PortSourceGPIOI;
    }
#endif
#ifdef GPIOJ
    if (port == GPIOJ)
    {
        return GPIO_PortSourceGPIOJ;
    }
#endif
#ifdef GPIOK
    if (port == GPIOK)
    {
        return GPIO_PortSourceGPIOK;
    }
#endif
#ifdef GPIOL
    if (port == GPIOL)
    {
        return GPIO_PortSourceGPIOL;
    }
#endif
#ifdef GPIOM
    if (port == GPIOM)
    {
        return GPIO_PortSourceGPIOM;
    }
#endif
#ifdef GPION
    if (port == GPION)
    {
        return GPIO_PortSourceGPION;
    }
#endif
    return GPIO_PortSourceGPIOA; // 默认回退
}

uint8_t Get_GPIO_PinSource(uint16_t pin) noexcept
{
    switch (pin)
    {
    case GPIO_Pin_0:
        return GPIO_PinSource0;
    case GPIO_Pin_1:
        return GPIO_PinSource1;
    case GPIO_Pin_2:
        return GPIO_PinSource2;
    case GPIO_Pin_3:
        return GPIO_PinSource3;
    case GPIO_Pin_4:
        return GPIO_PinSource4;
    case GPIO_Pin_5:
        return GPIO_PinSource5;
    case GPIO_Pin_6:
        return GPIO_PinSource6;
    case GPIO_Pin_7:
        return GPIO_PinSource7;
    case GPIO_Pin_8:
        return GPIO_PinSource8;
    case GPIO_Pin_9:
        return GPIO_PinSource9;
    case GPIO_Pin_10:
        return GPIO_PinSource10;
    case GPIO_Pin_11:
        return GPIO_PinSource11;
    case GPIO_Pin_12:
        return GPIO_PinSource12;
    case GPIO_Pin_13:
        return GPIO_PinSource13;
    case GPIO_Pin_14:
        return GPIO_PinSource14;
    case GPIO_Pin_15:
        return GPIO_PinSource15;
    default:
        return GPIO_PinSource0; // 默认回退
    }
}

uint32_t Get_RCC_APB2Periph(GPIO_TypeDef *port) noexcept
{
#ifdef GPIOA
    if (port == GPIOA)
    {
        return RCC_APB2Periph_GPIOA;
    }
#endif // GPIOA
#ifdef GPIOB
    if (port == GPIOB)
    {
        return RCC_APB2Periph_GPIOB;
    }
#endif // GPIOB
#ifdef GPIOC
    if (port == GPIOC)
    {
        return RCC_APB2Periph_GPIOC;
    }
#endif // GPIOC
#ifdef GPIOD
    if (port == GPIOD)
    {
        return RCC_APB2Periph_GPIOD;
    }
#endif // GPIOD
#ifdef GPIOE
    if (port == GPIOE)
    {
        return RCC_APB2Periph_GPIOE;
    }
#endif // GPIOE
#ifdef GPIOF
    if (port == GPIOF)
    {
        return RCC_APB2Periph_GPIOF;
    }
#endif // GPIOF
#ifdef GPIOG
    if (port == GPIOG)
    {
        return RCC_APB2Periph_GPIOG;
    }
#endif // GPIOG
#ifdef GPIOH
    if (port == GPIOH)
    {
        return RCC_APB2Periph_GPIOH;
    }
#endif // GPIOH
#ifdef GPIOI
    if (port == GPIOI)
    {
        return RCC_APB2Periph_GPIOI;
    }
#endif // GPIOI
#ifdef GPIOJ
    if (port == GPIOJ)
    {
        return RCC_APB2Periph_GPIOJ;
    }
#endif // GPIOJ
#ifdef GPIOK
    if (port == GPIOK)
    {
        return RCC_APB2Periph_GPIOK;
    }
#endif // GPIOK
#ifdef GPIOL
    if (port == GPIOL)
    {
        return RCC_APB2Periph_GPIOL;
    }
#endif // GPIOL
#ifdef GPIOM
    if (port == GPIOM)
    {
        return RCC_APB2Periph_GPIOM;
    }
#endif // GPIOM
#ifdef GPION
    if (port == GPION)
    {
        return RCC_APB2Periph_GPION;
    }
#endif        // GPION
    return 0; // 默认回退
}

uint32_t Get_EXTI_Line(uint16_t pin) noexcept
{
    switch (pin)
    {
    case GPIO_Pin_0:
        return EXTI_Line0;
    case GPIO_Pin_1:
        return EXTI_Line1;
    case GPIO_Pin_2:
        return EXTI_Line2;
    case GPIO_Pin_3:
        return EXTI_Line3;
    case GPIO_Pin_4:
        return EXTI_Line4;
    case GPIO_Pin_5:
        return EXTI_Line5;
    case GPIO_Pin_6:
        return EXTI_Line6;
    case GPIO_Pin_7:
        return EXTI_Line7;
    case GPIO_Pin_8:
        return EXTI_Line8;
    case GPIO_Pin_9:
        return EXTI_Line9;
    case GPIO_Pin_10:
        return EXTI_Line10;
    case GPIO_Pin_11:
        return EXTI_Line11;
    case GPIO_Pin_12:
        return EXTI_Line12;
    case GPIO_Pin_13:
        return EXTI_Line13;
    case GPIO_Pin_14:
        return EXTI_Line14;
    case GPIO_Pin_15:
        return EXTI_Line15;
    default:
        return 0; // 默认回退
    }
}

IRQn Get_IRQChannel(uint16_t pin) noexcept
{
    switch (pin)
    {
    case GPIO_Pin_0:
        return EXTI0_IRQn;
    case GPIO_Pin_1:
        return EXTI1_IRQn;
    case GPIO_Pin_2:
        return EXTI2_IRQn;
    case GPIO_Pin_3:
        return EXTI3_IRQn;
    case GPIO_Pin_4:
        return EXTI4_IRQn;
    case GPIO_Pin_5: // 合并通道
    case GPIO_Pin_6:
    case GPIO_Pin_7:
    case GPIO_Pin_8:
    case GPIO_Pin_9:
        return EXTI9_5_IRQn;
    case GPIO_Pin_10: // 合并通道
    case GPIO_Pin_11:
    case GPIO_Pin_12:
    case GPIO_Pin_13:
    case GPIO_Pin_14:
    case GPIO_Pin_15:
        return EXTI15_10_IRQn;
    default:
        return EXTI0_IRQn; // 默认回退
    }
}

uint32_t get_TIM_RCC_APB1Periph(TIM_TypeDef *__TIMX) noexcept
{
    if (__TIMX == TIM2)
        return RCC_APB1Periph_TIM2;
    else if (__TIMX == TIM3)
        return RCC_APB1Periph_TIM3;
    else if (__TIMX == TIM4)
        return RCC_APB1Periph_TIM4;
    else if (__TIMX == TIM5)
        return RCC_APB1Periph_TIM5;
    else if (__TIMX == TIM6)
        return RCC_APB1Periph_TIM6;
    else if (__TIMX == TIM7)
        return RCC_APB1Periph_TIM7;
    else if (__TIMX == TIM12)
        return RCC_APB1Periph_TIM12;
    else if (__TIMX == TIM13)
        return RCC_APB1Periph_TIM13;
    else if (__TIMX == TIM14)
        return RCC_APB1Periph_TIM14;
    else
        return 0;
}

auto Get_TIM_Index(TIM_TypeDef *TIMX) noexcept -> size_t
{
    if (TIMX == TIM1)
        return 0;
    else if (TIMX == TIM2)
        return 1;
    else if (TIMX == TIM3)
        return 2;
    else if (TIMX == TIM4)
        return 3;
    else
        return 0; // 默认回退
}

EMBMARTIN_STM32F10X_NAMESPACE_END
