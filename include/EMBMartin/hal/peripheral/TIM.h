#pragma once

#include "EMBMartin/macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)

#include "EMBMartin/hal/peripheral/timer_manager.h"
#include "EMBMartin/hal/gpio/basic_tools.h"


EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief 外部定时器类，用于处理 STM32 的外部定时器功能
 *
 * 该类把定时器配置为外部时钟模式（ETR），以指定引脚上的边沿作为计数时钟，
 * 适用于需要统计外部脉冲个数或频率的场合。
 */
class OuterTimer
{
public:
    /**
     * @brief 构造一个外部定时器对象
     * @param TIMX 定时器外设指针
     * @param echo_pin 回音引脚（只能是规定的复用引脚）
     * @param times 次数
     * @param PreemptionPriority 抢占优先级，默认为 0
     * @param SubPriority 子优先级，默认为 0
     */
    OuterTimer(TIM_TypeDef *TIMX, GPIOPin echo_pin, /* 只能是规定的复用引脚 */ uint16_t times, /* 次数 */
               uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0) noexcept;
};

EMBMARTIN_STM32F10X_NAMESPACE_END
