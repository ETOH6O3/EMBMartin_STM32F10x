#include "TIM.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief CounterSensor构造函数，用于初始化计数传感器
 * @param _pin GPIO引脚配置，指定使用的GPIO端口和引脚
 * @param _edge 触发边沿设置，true表示上升沿触发，false表示下降沿触发
 * @param __PreemptionPriority 中断抢占优先级，默认值为1
 * @param __SubPriority 中断子优先级，默认值为1
 * @note 中断函数需要用户自行定义
 *
 * 该构造函数完成以下初始化工作：
 *
 * 1. 配置GPIO时钟和复用功能时钟
 *
 * 2. 初始化GPIO引脚为输入模式（上拉或下拉）
 *
 * 3. 配置外部中断线连接到指定GPIO引脚
 *
 * 4. 初始化外部中断控制器参数
 *
 * 5. 配置嵌套向量中断控制器(NVIC)优先级和中断通道
 */
CounterSensor::CounterSensor(
    GPIOPin _pin, bool _edge,
    uint8_t __PreemptionPriority, uint8_t __SubPriority,
    GPIOSpeed_TypeDef _GPIO_Speed, EXTIMode_TypeDef _EXTI_Mode) noexcept
    : pin{_pin}, edge(_edge)
{
    // ------------------------------------------CLK--------------------------------------------------
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(_pin.port) | RCC_APB2Periph_AFIO, ENABLE);

    // ------------------------------------------GPIO--------------------------------------------------

    GPIO_InitTypeDef GPIO_InitStruct = {
        .GPIO_Pin = _pin.pin,
        .GPIO_Speed = _GPIO_Speed,
        .GPIO_Mode = _edge ? GPIO_Mode_IPU : GPIO_Mode_IPD,
    };
    GPIO_Init(_pin.port, &GPIO_InitStruct);

    // ------------------------------------------EXTI--------------------------------------------------
    GPIO_EXTILineConfig(Get_GPIO_PortSource(_pin.port), Get_GPIO_PinSource(_pin.pin));

    EXTI_InitTypeDef EXTI_InitStructure = {
        .EXTI_Line = Get_EXTI_Line(_pin.pin),
        .EXTI_Mode = _EXTI_Mode,
        .EXTI_Trigger = _edge ? EXTI_Trigger_Rising : EXTI_Trigger_Falling,
        .EXTI_LineCmd = ENABLE,
    };
    EXTI_Init(&EXTI_InitStructure);

    // ------------------------------------------NVIC--------------------------------------------------
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = static_cast<uint8_t>(Get_IRQChannel(_pin.pin)),
        .NVIC_IRQChannelPreemptionPriority = __PreemptionPriority,
        .NVIC_IRQChannelSubPriority = __SubPriority,
        .NVIC_IRQChannelCmd = ENABLE,
    };
    NVIC_Init(&NVIC_InitStructure);
}

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
