#include "EMBMartin/hal/peripheral/TIM.h"
#include "EMBMartin/hal/delay.h" // Delayer::delay 的回退路径要用 sys::delay_ms

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

OuterTimer::OuterTimer(
    TIM_TypeDef *__TIMX, GPIOPin echo_pin, uint16_t time,
    uint8_t PreemptionPriority, uint8_t SubPriority) noexcept
{
    RCC_APB1PeriphClockCmd(get_TIM_RCC_APB1Periph(__TIMX), ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(echo_pin.port), ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Pin = echo_pin.pin,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_IN_FLOATING,
    };
    GPIO_Init(echo_pin.port, &GPIO_InitStructure);

    TIM_ETRClockMode2Config(__TIMX, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0x0f);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {
        .TIM_Prescaler = 1 - 1,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = uint16_t(time - 1),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0};
    TIM_TimeBaseInit(__TIMX, &TIM_TimeBaseInitStructure);
    TIM_ClearFlag(__TIMX, TIM_FLAG_Update); // 清除 TIM_TimeBaseInit 中生成的更新中断标志

    TIM_ITConfig(__TIMX, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = uint8_t((__TIMX == TIM2)   ? TIM2_IRQn
                                   : (__TIMX == TIM3) ? TIM3_IRQn
                                   : (__TIMX == TIM4) ? TIM4_IRQn
                                                      : (IRQn)0xFF),
        .NVIC_IRQChannelPreemptionPriority = PreemptionPriority,
        .NVIC_IRQChannelSubPriority = SubPriority,
        .NVIC_IRQChannelCmd = ENABLE};
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(__TIMX, ENABLE);
}
EMBMARTIN_STM32F10X_NAMESPACE_END
