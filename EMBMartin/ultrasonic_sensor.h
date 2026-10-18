#pragma once

#include <cmath>

#include "inpin.h"
#include "outpin.h"
#include "system.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class UltrasonicSensor
{
private:
    Inpin _echo;
    OutPin _trig;

    const double TIM_TIME_S;
    uint16_t cnt;
    const uint16_t SOUND_VELOCITY;

    sys::InnerTimerManager::Handle __update_handle = {}; //!< 回波采样在快拍定时器队列里的句柄
    sys::InnerTimerManager::Handle __trig_handle = {};   //!< 测距触发在慢拍定时器队列里的句柄

    // 互补滤波：本次输出 = α · 本次测量 + (1 - α) · 上次输出
    static constexpr double FILTER_ALPHA = 0.8; //!< 新测量的权重

    mutable double filtered_distance = 0.0; //!< 滤波后的距离
    mutable uint16_t filtered_cnt = 0;      //!< 上次参与滤波的回波计数
    mutable bool filtered_valid = false;    //!< 是否已有滤波输出

public:
    /**
     * @brief 构造一个超声波传感器，并自动把测距触发与回波采样注册进各自的定时器队列
     *
     * 两种回调对节拍频率的要求相差很大，因此分开挂：
     * - `trig()` 挂慢拍定时器，两次触发之间必须留够模块的测量时间（HC-SR04 约 60 ms 以上）；
     * - `update()` 挂快拍定时器，靠数节拍累加回波高电平宽度，漏拍会直接算错距离。
     *
     * @param echo 回音引脚
     * @param trig 触发引脚
     * @param tim_time_s 快拍定时器的节拍时长（秒），即 update() 的计数分辨率
     * @param temprature_kelvin 环境温度（开尔文）
     * @param update_TIMX 提供回波采样节拍的定时器；为 nullptr 时不自动采样
     * @param trig_TIMX 提供测距触发节拍的定时器；为 nullptr 时不自动触发，只能手动调 trig()
     */
    inline UltrasonicSensor(const GPIOPin &echo, const GPIOPin &trig, double tim_time_s, uint16_t temprature_kelvin,
                            TIM_TypeDef *update_TIMX = nullptr, TIM_TypeDef *trig_TIMX = nullptr) noexcept
        : _echo{echo, GPIO_Mode_IPD}, _trig{trig}, TIM_TIME_S{tim_time_s}, cnt{0}, SOUND_VELOCITY{uint16_t(20.05 * std::sqrt(temprature_kelvin))}
        {
            _trig = 0;
            if (update_TIMX != nullptr)
                __update_handle = sys::InnerTimerManager::add(update_TIMX, [this]()
                                                              { this->update(); });
            if (trig_TIMX != nullptr)
                __trig_handle = sys::InnerTimerManager::add(trig_TIMX, [this]()
                                                            { this->trig(); });
        }

    /// 析构时注销测距触发与回波采样
    inline ~UltrasonicSensor() noexcept
    {
        sys::InnerTimerManager::remove(__update_handle);
        sys::InnerTimerManager::remove(__trig_handle);
    }

    UltrasonicSensor(const UltrasonicSensor &) = delete;
    UltrasonicSensor &operator=(const UltrasonicSensor &) = delete;

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

    /**
     * @brief 获取滤波后的距离（米）
     *
     * 互补滤波：d = α · d_测量 + (1 - α) · d_上次输出，α = FILTER_ALPHA。
     * 只在收到新的回波测量（cnt 变化）时更新一次，重复调用不会重复滤波，
     * 因此可以被主循环与中断同时读取而不变形。
     */
    inline double get_distance() const noexcept
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

    /// 未经滤波的原始距离（米）
    inline double get_raw_distance() const noexcept
    {
        return cnt * SOUND_VELOCITY * TIM_TIME_S / 2.0;
    }

    inline operator double() const noexcept
    {
        return get_distance();
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END