#include "basic_tools.h"
#include "tools.h"

using namespace EMBMartin::STM32;

extern uint32_t SystemCoreClock;
uint8_t EMBMartin::STM32::Get_GPIO_PortSource(GPIO_TypeDef *port) noexcept
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

uint8_t EMBMartin::STM32::Get_GPIO_PinSource(uint16_t pin) noexcept
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

uint32_t EMBMartin::STM32::Get_RCC_APB2Periph(GPIO_TypeDef *port) noexcept
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
uint32_t EMBMartin::STM32::Get_EXTI_Line(uint16_t pin) noexcept
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

uint32_t EMBMartin::STM32::get_TIM_RCC_APB1Periph(TIM_TypeDef *__TIMX) noexcept
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
IRQn EMBMartin::STM32::Get_IRQChannel(uint16_t pin) noexcept
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

auto EMBMartin::STM32::Get_TIM_Index(TIM_TypeDef *TIMX) noexcept -> size_t
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
EMBMartin::STM32::CounterSensor::CounterSensor(
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

EMBMartin::STM32::EXTIRotaryEncoder::EXTIRotaryEncoder(
    GPIOPin _pin_a, GPIOPin _pin_b,
    uint8_t __PreemptionPriority, uint8_t __SubPriority,
    GPIOSpeed_TypeDef GPIO_Speed, EXTIMode_TypeDef _EXTI_Mode) noexcept
    : pin_a{_pin_a}, pin_b{_pin_b}, count{0}
{
    // ------------------------------------------CLK--------------------------------------------------
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(_pin_a.port) | Get_RCC_APB2Periph(_pin_b.port) | RCC_APB2Periph_AFIO, ENABLE);

    // ------------------------------------------GPIO--------------------------------------------------

    GPIO_InitTypeDef GPIO_InitStruct_a = {
        .GPIO_Pin = _pin_a.pin,
        .GPIO_Speed = GPIO_Speed,
        .GPIO_Mode = GPIO_Mode_IPD,
    };
    GPIO_Init(_pin_a.port, &GPIO_InitStruct_a);

    GPIO_InitTypeDef GPIO_InitStruct_b = {
        .GPIO_Pin = _pin_b.pin,
        .GPIO_Speed = GPIO_Speed,
        .GPIO_Mode = GPIO_Mode_IPD,
    };
    GPIO_Init(_pin_b.port, &GPIO_InitStruct_b);

    // ------------------------------------------EXTI--------------------------------------------------
    GPIO_EXTILineConfig(Get_GPIO_PortSource(_pin_a.port), Get_GPIO_PinSource(_pin_a.pin));
    GPIO_EXTILineConfig(Get_GPIO_PortSource(_pin_b.port), Get_GPIO_PinSource(_pin_b.pin));

    EXTI_InitTypeDef EXTI_InitStructure = {
        .EXTI_Line = Get_EXTI_Line(_pin_a.pin) | Get_EXTI_Line(_pin_b.pin),
        .EXTI_Mode = _EXTI_Mode,
        .EXTI_Trigger = EXTI_Trigger_Falling,
        .EXTI_LineCmd = ENABLE,
    };
    EXTI_Init(&EXTI_InitStructure);

    // ------------------------------------------NVIC--------------------------------------------------
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitTypeDef NVIC_InitStructure_a = {
        .NVIC_IRQChannel = static_cast<uint8_t>(Get_IRQChannel(_pin_a.pin)),
        .NVIC_IRQChannelPreemptionPriority = __PreemptionPriority,
        .NVIC_IRQChannelSubPriority = __SubPriority,
        .NVIC_IRQChannelCmd = ENABLE,
    };

    NVIC_InitTypeDef NVIC_InitStructure_b = {
        .NVIC_IRQChannel = static_cast<uint8_t>(Get_IRQChannel(_pin_b.pin)),
        .NVIC_IRQChannelPreemptionPriority = __PreemptionPriority,
        .NVIC_IRQChannelSubPriority = __SubPriority,
        .NVIC_IRQChannelCmd = ENABLE,
    };
    NVIC_Init(&NVIC_InitStructure_a);
    NVIC_Init(&NVIC_InitStructure_b);
}

EMBMartin::STM32::RotaryEncoder::RotaryEncoder(TIM_TypeDef *TIMX, uint8_t TIMx_REMAP, bool reverse) noexcept
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
EMBMartin::STM32::OuterTimer::OuterTimer(
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

EMBMartin::STM32::PWM::PWM(
    TIM_TypeDef *__TIMX, int CHn, uint8_t TIMx_REMAP, const GPIOPin output_pin,
    int Duty_permillage, int Freq_HZ, int Reso_permillage) noexcept
    : _Duty_permillage(Duty_permillage), _Freq_HZ(Freq_HZ), _Reso_permillage(Reso_permillage),
      _TIMX(__TIMX), _CHn(CHn)

{
    SystemCoreClockUpdate();
    if (__TIMX != TIM1)
        RCC_APB1PeriphClockCmd(get_TIM_RCC_APB1Periph(__TIMX), ENABLE);
    else
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(output_pin.port), ENABLE);

    if (TIMx_REMAP) // 复用模式
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

        if (output_pin == PB4)
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
        else if ((output_pin == PA15) || (output_pin == PB3))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
        else if ((output_pin == PA13) || (output_pin == PA14))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        if (__TIMX == TIM1)
        {
            if (TIMx_REMAP == 0b01)
                GPIO_PinRemapConfig(GPIO_PartialRemap_TIM1, ENABLE);
            else if (TIMx_REMAP == 0b11)
                GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);
        }
        else if (__TIMX == TIM2)
        {
            if (TIMx_REMAP == 0b01)
                GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
            else if (TIMx_REMAP == 0b10)
                GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM2, ENABLE);
            else if (TIMx_REMAP == 0b11)
                GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);
        }
        else if (__TIMX == TIM3)
        {
            if (TIMx_REMAP == 0b10)
                GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);
            else if (TIMx_REMAP == 0b11)
                GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);
        }
        else if (__TIMX == TIM4)
        {
            if (TIMx_REMAP == 0b01)
                GPIO_PinRemapConfig(GPIO_Remap_TIM4, ENABLE);
        }
    }

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Pin = output_pin.pin,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_AF_PP,
    };
    GPIO_Init(output_pin.port, &GPIO_InitStructure);

    TIM_InternalClockConfig(__TIMX);

    const uint16_t ARR_plus1(1000 / Reso_permillage);
    const uint16_t CCR(Duty_permillage * ARR_plus1 / 1000);
    const uint16_t PSC_plus1(SystemCoreClock / (Freq_HZ * ARR_plus1));

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {
        .TIM_Prescaler = uint16_t(PSC_plus1 - 1),
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = uint16_t(ARR_plus1 - 1),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0};
    TIM_TimeBaseInit(__TIMX, &TIM_TimeBaseInitStructure);

    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = CCR;
    switch (_CHn)
    {
    case 1:
        TIM_OC1Init(__TIMX, &TIM_OCInitStructure);
        break;
    case 2:
        TIM_OC2Init(__TIMX, &TIM_OCInitStructure);
        break;
    case 3:
        TIM_OC3Init(__TIMX, &TIM_OCInitStructure);
        break;
    case 4:
        TIM_OC4Init(__TIMX, &TIM_OCInitStructure);
        break;
    default:
        break;
    }

    TIM_Cmd(__TIMX, ENABLE);
}

void EMBMartin::STM32::PWM::set_duty(int Duty_permillage) noexcept
{
    // 限制在 0-1000 之间
    if (Duty_permillage < 0)
        Duty_permillage = 0;
    else if (Duty_permillage > 1000)
        Duty_permillage = 1000;

    this->_Duty_permillage = Duty_permillage;
    switch (this->_CHn)
    {
    case 1:
        TIM_SetCompare1(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    case 2:
        TIM_SetCompare2(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    case 3:
        TIM_SetCompare3(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    case 4:
        TIM_SetCompare4(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    default:
        break;
    }
}

void EMBMartin::STM32::BreathingLED::breathe(int times) noexcept
{
    for (int t = 0; t < times; t++)
    {
        if (_driven_level)
        {
            int i;
            for (i = 0; i <= 1000; i++)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
            for (; i >= 0; i--)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
        }
        else
        {
            int i;
            for (i = 1000; i >= 0; i--)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
            for (; i <= 1000; i++)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
        }
    }
}

void EMBMartin::STM32::DCMotorDriver::set_mode(Mode mode, uint8_t motor_index) noexcept
{
    OutPin *IN1;
    OutPin *IN2;
    PWM *PWm;
    if (motor_index == 0)
    {
        IN1 = &AIN1;
        IN2 = &AIN2;
        PWm = &PWMA;
    }
    else
    {
        IN1 = &BIN1;
        IN2 = &BIN2;
        PWm = &PWMB;
    }

    switch (mode)
    {
    case Mode::STOP:
        IN1->reset();
        IN2->reset();
        PWm->set_duty(0);
        break;
    case Mode::BRAKE:
        PWm->set_duty(0);
        break;
    case Mode::FORWARD:
        IN1->set();
        IN2->reset();
        break;
    case Mode::REVERSE:
        IN1->reset();
        IN2->set();
        break;
    default:
        break;
    }
}

void EMBMartin::STM32::DCMotorDriver::set_speed(int16_t speed_permillage, uint8_t motor_index) noexcept
{
    speed_permillage = std::clamp<int16_t>(speed_permillage, -1000, 1000);
    PWM *PWm;
    if (motor_index == 0)
    {
        current_left_speed_permillage = speed_permillage;
        PWm = &PWMA;
    }
    else
    {
        current_right_speed_permillage = speed_permillage;
        PWm = &PWMB;
    }

    if (speed_permillage < 0)
        set_mode(Mode::REVERSE, motor_index);
    else
        set_mode(Mode::FORWARD, motor_index);

    PWm->set_duty(std::abs(speed_permillage));
}

void EMBMartin::STM32::I2C::send_byte(uint8_t data) noexcept
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        // 契约：确保 SCL 必定已经是低电平
        (data & (0x80 >> i)) ? _SDA.set() : _SDA.reset();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.set();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.reset();
    }
}

uint8_t EMBMartin::STM32::I2C::receive_byte() noexcept
{
    uint8_t rslt{0x00};
    // 契约：确保 SCL 必定已经是低电平
    _SDA.set(); // 释放 SDA
    EMBMARTIN_KEEP_CODE_ORDER;

    for (uint8_t i = 0; i < 8; ++i)
    {
        _SCL.set();
        EMBMARTIN_KEEP_CODE_ORDER;
        rslt <<= 1;
        rslt += _SDA.read();
        EMBMARTIN_KEEP_CODE_ORDER;
        _SCL.reset();
    }

    return rslt;
}

uint8_t EMBMartin::STM32::I2C::read_reg(uint8_t reg_addr) noexcept
{
    // 指定地址
    start();
    send_byte(_addr);
    receive_ack();
    send_byte(reg_addr);
    receive_ack();

    // 进入读模式
    start();
    send_byte(_addr | 0x01);
    receive_ack();

    // 劫收
    uint8_t rslt = receive_byte();
    send_ack(1);

    stop();
    return rslt;
}

void EMBMartin::STM32::AngleComplementaryFilter::update() noexcept
{
    double angle_acc_pitch = std::atan2(double(MPU_data.acc_x), MPU_data.acc_z) * 180 / 3.14159265358979;
    double angle_acc_roll = std::atan2(double(MPU_data.acc_y), MPU_data.acc_z) * 180 / 3.14159265358979;

    current_pitch = alpha * angle_acc_pitch + (1 - alpha) * (current_pitch - MPU_data.gyro_y * 1000 / 32768.0 * dt);
    current_roll = alpha * angle_acc_roll + (1 - alpha) * (current_roll + MPU_data.gyro_x * 1000 / 32768.0 * dt);
    current_yaw = current_yaw + MPU_data.gyro_z * 1000 / 32768.0 * dt; // 偏航角无法互补滤波
}

double EMBMartin::STM32::SingleLoopPID::compute(double target) noexcept
{
    double error = this->current - target;

    // 误差滤波
    error = (1 - this->alpha) * error + this->alpha * this->previous_error;

    // 积分项计算
    this->integral += error * this->dt;

    // 积分限幅
    if (integral_limit > 0) // 小于等于 0 表示不启用限幅
    {
        if (this->integral > this->integral_limit)
            this->integral = this->integral_limit;
        else if (this->integral < -this->integral_limit)
            this->integral = -this->integral_limit;
    }

    // 微分项计算
    double derivative;
    if (this->d_current == nullptr)
        derivative = (error - this->previous_error) / this->dt;
    else
        derivative = *(this->d_current);

    // PID输出计算
    double output = this->kp * error + this->ki * this->integral + this->kd * derivative;

    // 保存当前误差以供下次计算微分项
    this->previous_error = error;

    // 输出限幅
    if (output_limit > 0) // 小于等于 0 表示不启用限幅
    {
        if (output > this->output_limit)
            output = this->output_limit;
        else if (output < -this->output_limit)
            output = -this->output_limit;
    }

    return output;
}

EMBMartin::STM32::BalancedCarPID::Output EMBMartin::STM32::BalancedCarPID::compute(double turn_trg, double velocity_trg) noexcept
{
    this->angle_filter.update();
    this->angle_status = this->angle_filter.get_status();
    this->angle_status.pitch -= this->pitch_med_angle;

    this->turn_pid_current = (this->pace_left - this->pace_right) / 2.0;
    this->velocity_pid_current = (this->pace_left + this->pace_right) / 2.0;

    auto velocity_out = this->velocity.compute(velocity_trg);
    auto vertical_out = this->vertical.compute(velocity_out);
    auto turn_out = this->turn.compute(turn_trg);

    return Output{
        .duty_left_permillage = vertical_out - turn_out,
        .duty_right_permillage = vertical_out + turn_out,
    };
}

EMBMartin::STM32::USART::USART(USART_TypeDef *USARTx, int remap, int baud_rate) noexcept
:_USARTx(USARTx)
{
    auto pin_tx = USARTX_REMAP[Get_USART_Index(USARTx)][remap].tx;
    auto pin_rx = USARTX_REMAP[Get_USART_Index(USARTx)][remap].rx;
    // 开启时钟
    if (USARTx == USART1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    else
        RCC_APB1PeriphClockCmd(Get_RCC_APB1Periph(USARTx), ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin_tx.port), ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin_rx.port), ENABLE);

    // 配置引脚
    GPIO_InitTypeDef GPIO_InitStructure;
    // TX
    GPIO_InitStructure.GPIO_Pin = pin_tx.pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(pin_tx.port, &GPIO_InitStructure);
    // RX
    GPIO_InitStructure.GPIO_Pin = pin_rx.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(pin_rx.port, &GPIO_InitStructure);

    // 配置 USART
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud_rate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USARTx, &USART_InitStructure);
    USART_Cmd(USARTx, ENABLE);
}