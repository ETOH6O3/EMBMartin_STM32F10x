#include "EMBMartin/device/counter_sensor.h"

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

EMBMARTIN_STM32F10X_NAMESPACE_END
