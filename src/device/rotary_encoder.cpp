#include "EMBMartin/device/rotary_encoder.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

RotaryEncoder::RotaryEncoder(TIM_TypeDef *TIMX, uint8_t TIMx_REMAP, bool reverse) noexcept
    : __TIMX(TIMX)
{
    auto gpiopin1 = _GEN_TIM_CH_TO_GPIOPin_REMAP[TIMx_REMAP][Get_TIM_Index(TIMX)][0];
    auto gpiopin2 = _GEN_TIM_CH_TO_GPIOPin_REMAP[TIMx_REMAP][Get_TIM_Index(TIMX)][1];

    /*开启时钟*/
    if (TIMX != TIM1)
        RCC_APB1PeriphClockCmd(get_TIM_RCC_APB1Periph(TIMX), ENABLE);
    else
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(gpiopin1.port), ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(gpiopin2.port), ENABLE);

    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = gpiopin1.pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpiopin1.port, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = gpiopin2.pin;
    GPIO_Init(gpiopin2.port, &GPIO_InitStructure);
    if (gpiopin1 == PB4)
    {
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
    }

    /*时基单元初始化*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIMX, &TIM_TimeBaseInitStructure);

    /*输入捕获初始化*/
    TIM_ICInitTypeDef TIM_ICInitStructure;

    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_ICInit(TIMX, &TIM_ICInitStructure);

    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_ICInit(TIMX, &TIM_ICInitStructure);

    auto polarity = reverse ? TIM_ICPolarity_Falling : TIM_ICPolarity_Rising;

    TIM_EncoderInterfaceConfig(TIMX, TIM_EncoderMode_TI12, polarity, TIM_ICPolarity_Rising);

    TIM_Cmd(TIMX, ENABLE);
}

EMBMARTIN_STM32F10X_NAMESPACE_END
