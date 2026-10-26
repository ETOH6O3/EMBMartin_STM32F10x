#pragma once

#include <cmath>

#include "EMBMartin/macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "EMBMartin/hal/gpio/basic_tools.h"

EMBMARTIN_HAL_NAMESPACE_BEGIN



/**
 * @brief PWM 类，用于生成 PWM 信号控制设备接口
 *
 * 该类封装了 STM32 定时器的 PWM 功能，可以方便地配置和控制 PWM 输出。
 * 支持设置 PWM 频率、占空比和分辨率，并可动态调整占空比。
 *
 * @note
 * 1. 输出引脚是根据定时器和通道自动确定的，无需手动指定
 * 2. Reso_permillage 参数建议使用能被 1000 整除的值，以保证计算精度
 * 3. 占空比范围为 0-100 的整数百分比
 *
 */
class PWM
{
private:
    int __Duty_permillage;
    int __Freq_HZ;
    int __Reso_permillage;
    TIM_TypeDef *_TIMX;
    int __CHn;

    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP, GPIOPin output_pin /* 只能是规定的复用引脚 */,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept;

public:
    /**
     * @brief PWM 构造函数
     * @param TIMX 定时器外设指针
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     * @param Duty_permillage 初始占空比（0-1000），默认 0%
     * @param Freq_HZ PWM 频率（Hz），默认 1kHz
     * @param Reso_permillage PWM 分辨率（千分之一），默认 10‰
     *
     * @note 有一些重映像会导致构造函数自动关闭调试端口，具体查阅参考手册
     * @note Reso_permillage 建议能被 1000 整除，以确保计算准确性
     * @note output_pin 根据定时器、通道和重映像自动选择
     */
    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, _GEN_TIM_CH_TO_GPIOPin_REMAP[TIMx_REMAP][Get_TIM_Index(TIMX)][CHn - 1],
              Duty_permillage, Freq_HZ, Reso_permillage) {};
    /**
     * @brief 设置 PWM 占空比
     * @param Duty_permillage 占空比（0-100）
     *
     * @note 占空比为千分比值，0 表示始终低电平，100 表示始终高电平
     * @note 该函数会根据设定的分辨率自动计算并设置比较寄存器值
     */
    void set_duty(int Duty_permillage) noexcept;
    inline auto get_duty() const noexcept
    {
        return __Duty_permillage;
    }
};


EMBMARTIN_HAL_NAMESPACE_END
