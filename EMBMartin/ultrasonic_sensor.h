#pragma once

#include <cmath>

#include "inpin.h"
#include "outpin.h"
#include "system.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class UltrasonicSensor
{
private:
    Inpin __echo;
    OutPin __trig;

    const double TIM_TIME_S;
    uint16_t cnt;
    const uint16_t SOUND_VELOCITY;

    /// 当前回波高电平已持续的快拍数（**必须是成员**，函数内 static 会被多个实例共用）
    uint16_t __pulse_ticks = 0;
    /// 上一拍的回波电平，用于检测下降沿（同上，必须是成员）
    bool __echo_last = false;

    /// 触发分频：每 __trig_divider 个触发节拍才真正发一次波
    const uint8_t __trig_divider;
    /// 启动等待：上电后先跳过这么多触发节拍再开始按分频发波（用于错开多个探头）
    const uint8_t __trig_start_wait;
    /// 分频/等待状态机的当前剩余节拍数
    uint8_t __trig_phase;

    sys::InnerTimerManager::Handle __update_handle = {}; //!< 回波采样在快拍定时器队列里的句柄
    sys::InnerTimerManager::Handle __trig_handle = {};   //!< 测距触发在慢拍定时器队列里的句柄

    // 互补滤波：本次输出 = α · 本次测量 + (1 - α) · 上次输出
    static constexpr double FILTER_ALPHA = 1; //!< 新测量的权重

    mutable double filtered_distance = 0.0; //!< 滤波后的距离
    mutable uint16_t filtered_cnt = 0;      //!< 上次参与滤波的回波计数
    mutable bool filtered_valid = false;    //!< 是否已有滤波输出
    volatile bool measured = false;         //!< 是否至少收到过一次回波（中断写，主循环读）

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
     * @param trig_divider 触发分频，默认 1（每个触发节拍都发波）
     * @param trig_start_wait 启动等待的触发节拍数，默认 0（立刻开始）
     *
     * @par 多探头错开（重要）
     *
     * 两个 HC-SR04 若在同一拍同时发波，会互相收到对方的回波；本类靠数回波宽度算距离，
     * 先到的那串回波会把计数锁死，两侧读数会同时失真。用「分频 + 启动等待」把相位错开：
     *
     * @code
     * // 触发节拍 100 ms：左在 0/200/400 ms 发波，右在 100/300/500 ms 发波，互不重叠
     * UltrasonicSensor left {echo_l, trig_l, 0.0001, 300, TIM3, TIM4, 2, 0};
     * UltrasonicSensor right{echo_r, trig_r, 0.0001, 300, TIM3, TIM4, 2, 1};
     * @endcode
     *
     * 即：第 k 个触发节拍（k 从 0 起）真正发波当且仅当 `k >= trig_start_wait` 且
     * `(k - trig_start_wait) % trig_divider == 0`。分频为 N 时每个探头的实际测量周期
     * 是触发周期的 N 倍，N 个探头错开后总测量率不变。
     */
    UltrasonicSensor(const GPIOPin &echo, const GPIOPin &trig, double tim_time_s, uint16_t temprature_kelvin,
                     TIM_TypeDef *update_TIMX = nullptr, TIM_TypeDef *trig_TIMX = nullptr,
                     uint8_t trig_divider = 1, uint8_t trig_start_wait = 0) noexcept;

    /// 析构时注销测距触发与回波采样
    inline ~UltrasonicSensor() noexcept
    {
        sys::InnerTimerManager::remove(__update_handle);
        sys::InnerTimerManager::remove(__trig_handle);
    }

    UltrasonicSensor(const UltrasonicSensor &) = delete;
    UltrasonicSensor &operator=(const UltrasonicSensor &) = delete;

    /// 立即发一次测距脉冲（不看分频，供手动触发使用）
    inline void trig() noexcept
    {
        __trig = 1;
        sys::delay_us(10);
        __trig = 0;
    }

    // 定时中断调用函数
    void update() noexcept;

    /**
     * @brief 触发节拍回调：按分频与启动等待决定本拍要不要发波
     *
     * 挂到 trig_TIMX 的队列上，由定时器更新中断每拍调用一次。
     */
    void trig_tick() noexcept;

    /**
     * @brief 是否已收到过至少一个完整回波
     *
     * 上电后到第一个回波之间 `cnt` 恒为 0，`get_distance()` 会算出 0 米，
     * 这**不代表**正前方贴脸有障碍物。用本函数区分「没测到」与「测到 0 米」。
     */
    [[nodiscard]] inline bool has_measurement() const noexcept
    {
        return measured;
    }

    /**
     * @brief 获取滤波后的距离（米）
     *
     * 互补滤波：d = α · d_测量 + (1 - α) · d_上次输出，α = FILTER_ALPHA。
     * 只在收到新的回波测量（cnt 变化）时更新一次，重复调用不会重复滤波，
     * 因此可以被主循环与中断同时读取而不变形。
     *
     * @note 尚未收到任何回波时返回 0，调用前可用 has_measurement() 判断。
     */
    double get_distance() const noexcept;

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
