#include "ultrasonic_sensor.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

UltrasonicSensor::UltrasonicSensor(const GPIOPin &echo, const GPIOPin &trig, double tim_time_s, uint16_t temprature_kelvin,
                                   TIM_TypeDef *update_TIMX, TIM_TypeDef *trig_TIMX) noexcept
    : __echo{echo, GPIO_Mode_IPD}, __trig{trig}, TIM_TIME_S{tim_time_s}, cnt{0}, SOUND_VELOCITY{uint16_t(20.05 * std::sqrt(temprature_kelvin))}
{
    __trig = 0;
    if (update_TIMX != nullptr)
        __update_handle = sys::InnerTimerManager::add(update_TIMX, [this]()
                                                      { this->update(); });
    if (trig_TIMX != nullptr)
        __trig_handle = sys::InnerTimerManager::add(trig_TIMX, [this]()
                                                    { this->trig(); });
}

void UltrasonicSensor::update() noexcept
{
    static uint16_t __cnt = 0;
    static bool _echo_last = __echo;
    if (__echo == 1)
    {
        __cnt++;
    }
    else
    {
        if (_echo_last == 1)
        {
            cnt = __cnt;
            __cnt = 0;
            measured = true; // 收到一个完整回波，测距结果有效
        }
    }
    _echo_last = __echo;
}

double UltrasonicSensor::get_distance() const noexcept
{
    if (filtered_valid && cnt == filtered_cnt)
    {
        return filtered_distance;
    }

    const double raw = cnt * SOUND_VELOCITY * TIM_TIME_S / 2.0;

    filtered_distance = filtered_valid ? FILTER_ALPHA * raw + (1.0 - FILTER_ALPHA) * filtered_distance
                                       : raw; // 首次直接用测量值起步
    filtered_cnt = cnt;
    filtered_valid = true;
    return filtered_distance;
}

EMBMARTIN_STM32F10X_NAMESPACE_END
