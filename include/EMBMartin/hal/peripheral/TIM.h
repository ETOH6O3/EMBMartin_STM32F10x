#pragma once

#include "EMBMartin/macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)

#include "EMBMartin/hal/gpio/basic_tools.h"
#include "EMBMartin/hal/peripheral/timer_manager.h"


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

/**
 * @brief 定时计数器类
 *
 * 继承自 InnerTimerManager，提供计数功能，每次定时中断自动增加计数值
 * @note 构造时自动把自身的计数回调注册进所属定时器的队列，用户不用再写中断函数
 */
class TIMCounter : public sys::InnerTimerManager
{
private:
    uint16_t count = 0; ///< 计数值

    /// 供基类构造期注册的计数回调
    inline Callback __make_counter() noexcept
    {
        return [this]()
        { handler(); };
    }

public:
    /**
     * @brief 构造一个定时计数器，并把计数回调追加到 init 队列末尾
     *
     * @param TIMX 定时器外设指针
     * @param time_second 定时时间(秒)，支持最小精度0.0001秒
     * @param open 初始是否开启定时器，默认为true
     * @param PreemptionPriority 抢占优先级，默认为0
     * @param SubPriority 子优先级，默认为0
     * @param init 初始调用队列，计数回调会排在它之后
     */
    inline TIMCounter(
        TIM_TypeDef *TIMX, double time_second,
        bool open = true,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        std::initializer_list<Callback> init = {}) noexcept
        : InnerTimerManager(TIMX, time_second, open, PreemptionPriority, SubPriority, init),
          count(0)
    {
        __counter_handle = add(__make_counter()); // 构造体结束后 this 已完全可用
    }

    /// 注销自身的计数回调
    inline ~TIMCounter() noexcept
    {
        remove(__counter_handle);
    }

    TIMCounter(const TIMCounter &) = delete;
    TIMCounter &operator=(const TIMCounter &) = delete;

    /**
     * @brief 获取当前计数值
     *
     * @return 当前计数值
     */
    inline auto get_count() const
    {
        return count;
    }

    /**
     * @brief 中断处理函数
     *
     * 已在队列中自动注册，也可手动调用
     */
    inline void handler()
    {
        count++;
    }

private:
    Handle __counter_handle = {}; //!< 计数回调节点句柄
};

EMBMARTIN_STM32F10X_NAMESPACE_END
