#include "ultrasonic_sensor.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

UltrasonicSensor::UltrasonicSensor(const GPIOPin &echo, const GPIOPin &trig, double tim_time_s, uint16_t temprature_kelvin,
                                   TIM_TypeDef *update_TIMX, TIM_TypeDef *trig_TIMX,
                                   uint8_t trig_divider, uint8_t trig_start_wait) noexcept
    : __echo{echo, GPIO_Mode_IPU}, __trig{trig}, TIM_TIME_S{tim_time_s}, cnt{0}, SOUND_VELOCITY{uint16_t(20.05 * std::sqrt(temprature_kelvin))},
      __trig_divider{trig_divider ? trig_divider : uint8_t(1)}, __trig_start_wait{trig_start_wait},
      __trig_phase{trig_start_wait}
{
    __trig = 0;
    __echo_last = __echo; // 以构造时刻的电平为基准，避免把上电前的高电平当成一次回波

    if (update_TIMX != nullptr)
        __update_handle = sys::InnerTimerManager::add(update_TIMX, [this]()
                                                      { this->update(); });
    if (trig_TIMX != nullptr)
        __trig_handle = sys::InnerTimerManager::add(trig_TIMX, [this]()
                                                    { this->trig_tick(); });
}

void UltrasonicSensor::trig_tick() noexcept
{
    // 启动等待期间只消耗节拍，不发波
    if (__trig_phase > 0)
    {
        --__trig_phase;
        return;
    }

    if (__echo == 1)
    {
        this->cnt = 2000;
    }
    
    trig();

    // 重新装载分频间隔：下一次发波要再等 __trig_divider 个触发节拍
    __trig_phase = uint8_t(__trig_divider - 1U);
}

void UltrasonicSensor::update() noexcept
{
    if (__echo == 1)
    {
        ++__pulse_ticks;
    }
    else
    {
        if (__echo_last)
        {
            cnt = __pulse_ticks;
            __pulse_ticks = 0;
            measured = true; // 收到一个完整回波，测距结果有效
        }
    }
    __echo_last = __echo;
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
