/**
 ******************************************************************************
 * @file    steering_engine.h
 * @author  孙鸣淼
 * @brief   舵机驱动（PWM 派生类）
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 *
 * 本文件由 hal/peripheral/PWM.h 拆出。
 * 依赖方向：本文件 -> hal/peripheral/PWM.h（单向）。
 ******************************************************************************
 */

#pragma once

#include "EMBMartin/hal/peripheral/PWM.h"

EMBMARTIN_DEVICE_NAMESPACE_BEGIN

/**
 * @brief 舵机驱动类
 *
 * 以 PWM 输出 50 Hz 方波，通过脉宽控制舵机角度。
 * 角度以「百分度」（格恩 / gon，1 gon = 0.9°）为单位，范围 0~200 对应 0~180°。
 */
class SteeringEngine : public hal::PWM
{
private:
    constexpr static int duty_min = 30;                          //!< 占空比最小值（左闭）
    constexpr static int duty_max = 120;                         //!< 占空比最大值（右开）
    constexpr static int delta_duty = duty_max - duty_min;       //!< 占空比上下界差值
    constexpr static int degree_min = 0;                         //!< 百分度最小值（左闭）
    constexpr static int degree_max = 200;                       //!< 百分度最大值（右开）
    constexpr static int delta_degree = degree_max - degree_min; //!< 百分度上下界差值

    int16_t __degree_gon; //!< 角度（格恩 / 百分度 / 0.9 度）

    inline static int16_t truncate_degree_gon(int16_t degree_gon) noexcept
    {
        // return std::abs(degree_gon) % degree_max + degree_min;
        return std::clamp<int>(degree_gon, degree_min, degree_max);
    }

public:
    inline SteeringEngine(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00) noexcept
        : hal::PWM(TIMX, CHn, TIMx_REMAP, duty_min, 50, 10), __degree_gon(0) {};

    /**
     * @brief 设置舵机角度（百分度）
     *
     * @param degree_gon 百分度（1 百分度 = 0.9 度）
     */
    inline void set_degree_gon(int16_t degree_gon) noexcept
    {
        this->__degree_gon = degree_gon;
        this->set_duty(duty_min + truncate_degree_gon(degree_gon) * delta_duty / delta_degree);
    }

    inline void set_degree(double degree) noexcept
    {
        set_degree_gon(int16_t(degree / 0.9));
    }

    inline void clockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon - int16_t(degree / 0.9));
    }

    inline void anticlockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon + int16_t(degree / 0.9));
    }
};

EMBMARTIN_DEVICE_NAMESPACE_END
