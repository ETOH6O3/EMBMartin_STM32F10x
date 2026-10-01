/**
 ******************************************************************************
 * @file    system.h
 * @author  孙鸣淼
 * @brief   这里存放一些无关外设的工具 （基于 STM32 标准外设库）
 ******************************************************************************
 * @attention
 * 0. 需要预定义宏 STM32_DEVICE_HEADER 为对应的 STM32 设备头文件名，如 stm32f10x.h （不得含引号）
 *    或者在 macro.h 中定义 STM32_DEVICE_HEADER 宏，这也是等价的
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#ifndef EMBM_SYSTEM_H
#define EMBM_SYSTEM_H

#include <stdint.h>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "Delay.h"
#include "basic_tools.h"

EMBMARTIN_SYS_NAMESPACE_BEGIN

// C 函数封装为 C++ 函数

inline auto delay_us(intmax_t us) noexcept
{
    Delay_us(us);
}

inline auto delay_ms(intmax_t ms) noexcept
{
    Delay_ms(ms);
}

inline auto delay_s(intmax_t s) noexcept
{
    Delay_s(s);
}

/**
 * @brief 内部定时器类
 *
 * 该类封装了STM32的硬件定时器功能，提供基于秒的时间基准配置
 * @note 中断函数的实现由用户自行完成
 */
class InnerTimer
{
private:
    TIM_TypeDef *TIMX; ///< 定时器外设指针
public:
    /**
     * @brief 构造函数，初始化定时器
     *
     * @param TIMX 定时器外设指针
     * @param time_second 定时时间(秒)，支持最小精度0.0001秒
     * @param PreemptionPriority 抢占优先级，默认为0
     * @param SubPriority 子优先级，默认为0
     * @param open 初始是否开启定时器，默认为true
     */
    InnerTimer(
        TIM_TypeDef *TIMX, double time_second, // 支持最小精度：0.0001秒
        bool open = true, /* 初始是否开启定时器 */
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0) noexcept;

    inline void open()noexcept
    {
        TIM_Cmd(TIMX, ENABLE);
    }

    inline void close()noexcept
    {
        TIM_Cmd(TIMX, DISABLE);
    }
};

/**
 * @brief 定时计数器类
 *
 * 继承自InnerTimer，提供计数功能，在每次定时中断时增加计数值
 * @note 中断函数的核心逻辑已经封装为成员函数 handler，方便用户编写中断函数时调用
 */
class TIMCounter : public InnerTimer
{
private:
    uint16_t count = 0; ///< 计数值

public:
    /**
     * @brief 继承基类构造函数
     */
    using InnerTimer::InnerTimer;

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
     * 在定时器中断服务程序中调用，用于递增计数值
     */
    inline void handler()
    {
        count++;
    }
};

EMBMARTIN_SYS_NAMESPACE_END

#endif // EMBM_SYSTEM_H