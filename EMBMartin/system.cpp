
#include "system.h"

using namespace EMBMartin::STM32::sys;

extern uint32_t SystemCoreClock;

constexpr inline uint16_t __TIM_Prescaler_plus1 = 1000; // 配合 TIM_GetCounter 可获得小数点后三位精度
InnerTimer::InnerTimer(
    TIM_TypeDef *__TIMX, double time, 
    bool open,
    uint8_t PreemptionPriority, uint8_t SubPriority) noexcept
    : TIMX{__TIMX}
{
    if (__TIMX != TIM1)
        RCC_APB1PeriphClockCmd(get_TIM_RCC_APB1Periph(__TIMX), ENABLE);
    else
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_InternalClockConfig(__TIMX);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {
        .TIM_Prescaler = __TIM_Prescaler_plus1 - 1,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = uint16_t(time * SystemCoreClock / __TIM_Prescaler_plus1 - 1),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0};
    TIM_TimeBaseInit(__TIMX, &TIM_TimeBaseInitStructure);
    TIM_ClearFlag(__TIMX, TIM_FLAG_Update); // 清除 TIM_TimeBaseInit 中生成的更新中断标志

    TIM_ITConfig(__TIMX, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = uint8_t((__TIMX == TIM1) ? TIM1_UP_IRQn : (__TIMX == TIM2) ? TIM2_IRQn
                                                                 : (__TIMX == TIM3)   ? TIM3_IRQn
                                                                 : (__TIMX == TIM4)   ? TIM4_IRQn
                                                                                      : (IRQn)0xFF),
        .NVIC_IRQChannelPreemptionPriority = PreemptionPriority,
        .NVIC_IRQChannelSubPriority = SubPriority,
        .NVIC_IRQChannelCmd = ENABLE};
    NVIC_Init(&NVIC_InitStructure);

    if(open)
        TIM_Cmd(__TIMX, ENABLE);
}
