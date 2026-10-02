#include "EXTI.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

EXTIManager::EXTIManager(const EMBMartin::STM32::GPIOPin &gpio_pin, const EXTIManager::Callback &operate,
                         EXTITrigger_TypeDef trigger, std::uint8_t __PreemptionPriority, std::uint8_t __SubPriority,
                         EXTIMode_TypeDef _EXTI_Mode) noexcept
    : __EXTI_pin(gpio_pin, GPIO_Mode_IPD) // GPIO 初始化
    , __operate(operate)                  // 先存好回调，再使能中断
{
    auto __idx = __decode_onehot(gpio_pin.pin);
    __instances[__idx] = this; // 指针数组已零初始化，此处只是写入指针，无初始化顺序问题

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_EXTILineConfig(Get_GPIO_PortSource(gpio_pin.port), Get_GPIO_PinSource(gpio_pin.pin));
    EXTI_InitTypeDef EXTI_InitStructure = {
        .EXTI_Line = Get_EXTI_Line(gpio_pin.pin),
        .EXTI_Mode = _EXTI_Mode,
        .EXTI_Trigger = trigger,
        .EXTI_LineCmd = ENABLE,
    };
    EXTI_Init(&EXTI_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = static_cast<uint8_t>(Get_IRQChannel(gpio_pin.pin)),
        .NVIC_IRQChannelPreemptionPriority = __PreemptionPriority,
        .NVIC_IRQChannelSubPriority = __SubPriority,
        .NVIC_IRQChannelCmd = ENABLE,
    };
    NVIC_Init(&NVIC_InitStructure);
}

EMBMARTIN_STM32F10X_NAMESPACE_END

extern "C"{
    void EXTI0_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line0) != RESET)
        {
            if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(0))
                __manager->invoke();
            EXTI_ClearITPendingBit(EXTI_Line0);
        }
    }

    void EXTI1_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line1) != RESET)
        {
            if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(1))
                __manager->invoke();
            EXTI_ClearITPendingBit(EXTI_Line1);
        }
    }

    void EXTI2_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line2) != RESET)
        {
            if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(2))
                __manager->invoke();
            EXTI_ClearITPendingBit(EXTI_Line2);
        }
    }

    void EXTI3_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line3) != RESET)
        {
            if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(3))
                __manager->invoke();
            EXTI_ClearITPendingBit(EXTI_Line3);
        }
    }

    void EXTI4_IRQHandler(void)
    {
        if (EXTI_GetITStatus(EXTI_Line4) != RESET)
        {
            if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(4))
                __manager->invoke();
            EXTI_ClearITPendingBit(EXTI_Line4);
        }
    }

    void EXTI9_5_IRQHandler(void)
    {
        for (int i = 5; i <= 9; ++i)
        {
            if (EXTI_GetITStatus(1 << i) != RESET)
            {
                if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(i))
                    __manager->invoke();
                EXTI_ClearITPendingBit(1 << i);
            }
        }
    }

    void EXTI15_10_IRQHandler(void)
    {
        for (int i = 10; i <= 15; ++i)
        {
            if (EXTI_GetITStatus(1 << i) != RESET)
            {
                if(auto *__manager = EMBMartin::STM32::EXTIManager::instance(i))
                    __manager->invoke();
                EXTI_ClearITPendingBit(1 << i);
            }
        }
    }
}