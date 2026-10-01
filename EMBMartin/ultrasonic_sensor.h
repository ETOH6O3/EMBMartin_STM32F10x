#pragma once

#include <cmath>

#include "inpin.h"
#include "outpin.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class UltrasonicSensor
{
private:
    Inpin _echo;
    OutPin _trig;

    const double TIM_TIME_S;
    uint16_t cnt;
    const uint16_t SOUND_VELOCITY;

public:
    inline UltrasonicSensor(const GPIOPin &echo, const GPIOPin &trig, double tim_time_s, uint16_t temprature_kelvin) noexcept
        : _echo{echo, GPIO_Mode_IPD}, _trig{trig}, TIM_TIME_S{tim_time_s}, cnt{0}, SOUND_VELOCITY{uint16_t(20.05 * std::sqrt(temprature_kelvin))}
        {
            _trig = 0;
        }

    inline void trig() noexcept
    {
        _trig = 1;
        sys::delay_us(10);
        _trig = 0;
    }

    // 定时中断调用函数
    inline void update() noexcept
    {
        static uint16_t __cnt = 0;
        static bool _echo_last = _echo;
        if (_echo == 1)
        {
            __cnt++;
        }
        else
        {
            if (_echo_last == 1) {
                cnt = __cnt;
                __cnt = 0;
            }
        }  
        _echo_last = _echo;
    }

    inline double get_distance() const noexcept
    {
        return cnt * SOUND_VELOCITY * TIM_TIME_S / 2.0;
    }

    inline operator double() const noexcept
    {
        return get_distance();
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END