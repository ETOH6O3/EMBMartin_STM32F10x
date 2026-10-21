#pragma once

///\file
/// 呼吸灯（PWM 派生类）。
///
/// 自 PWM.h 拆出。依赖方向：本文件 -> PWM.h（单向）。
/// 使用方既可以包含本文件，也可以通过 tools.h 一次性获得全部接口。

#include "PWM.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief 呼吸灯类，用于实现呼吸灯效果
 *
 */
class BreathingLED : public PWM
{
private:
    bool __driven_level;
    int __period_s;

public:
    /**
     * @brief 构造一个呼吸灯对象
     *
     * 呼吸灯是基于 PWM 实现的，通过周期性地改变 LED 亮度来模拟 "呼吸" 效果。
     *
     * @param TIMX 定时器外设指针，用于产生 PWM 信号
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     *                   - 0b00: 无重映像
     *                   - 0b01: 部分重映像 01
     *                   - 0b10: 部分重映像 10
     *                   - 0b11: 完全重映像
     * @param driven_level 驱动电平，false 表示低电平点亮 LED，true 表示高电平点亮 LED
     * @param period_s 呼吸周期（秒），即从暗到亮再到暗的完整周期时间
     *
     * @note 呼吸灯初始化时会自动设置 PWM 频率为 1kHz，分辨率为 10‰
     */
    BreathingLED(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
                 bool driven_level = false, int period_s = 5) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, driven_level ? 0 : 1000 /* 初期关闭 */, 1000, 10),
          __driven_level(driven_level), __period_s(period_s) {};

    /**
     * @brief 执行呼吸灯效果
     *
     * 控制 LED 按照设定的周期执行呼吸效果，即亮度从暗逐渐变亮，
     * 再从亮逐渐变暗的过程。
     *
     * @param times 呼吸次数，默认为 1 次
     *
     * @note 该函数是阻塞式的，会持续执行完指定次数的呼吸效果
     * @note 呼吸周期由构造函数中设置的_period_s 决定
     */
    void breathe(int times = 1) noexcept;
};

EMBMARTIN_STM32F10X_NAMESPACE_END
