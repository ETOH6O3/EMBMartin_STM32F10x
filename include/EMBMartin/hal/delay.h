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

#pragma once

#include <stdint.h>
#include <array>
#include <initializer_list>
#include <new>

#include <etl/inplace_function.h>

#include "EMBMartin/macro.h"
#include "__Delay.h"
#include "EMBMartin/hal/peripheral/timer_manager.h"
#include "EMBMartin/hal/gpio/basic_tools.h"
#include "EMBMartin/hal/irq.h"

#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)



EMBMARTIN_SYS_NAMESPACE_BEGIN

// C 函数封装为 C++ 函数

inline auto delay_us(intmax_t us) noexcept
{
    volatile IRQGuard guard; // 关中断，防止中断干扰
    Delay_us(us);
}

inline auto delay_ms(intmax_t ms) noexcept
{
    volatile IRQGuard guard; // 关中断，防止中断干扰
    Delay_ms(ms);
}

inline auto delay_s(intmax_t s) noexcept
{
    volatile IRQGuard guard; // 关中断，防止中断干扰
    Delay_s(s);
}


/**
 * @brief 定时计数器类
 *
 * 继承自 InnerTimerManager，提供计数功能，每次定时中断自动增加计数值
 * @note 构造时自动把自身的计数回调注册进所属定时器的队列，用户不用再写中断函数
 */
class TIMCounter : public InnerTimerManager
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

/**
 * @brief 基于定时中断的延时计数器
 *
 * 本类**不管理定时器**：定时器的时基、更新中断、NVIC 和中断服务函数全部由使用者负责，
 * 类只提供两件事——
 *
 * 1. `tick()`：在定时中断里调用，累加节拍计数；
 * 2. `done()`：在任意上下文（含中断）查询是否到期。
 *
 * 由于只做一次自增和一次有符号差值比较，`done()` 是非阻塞的，也不需要关中断，
 * 因此可以替代会打乱 SysTick 的 `Delay_ms`，以及会关中断的 `sys::delay_ms`。
 *
 * @code
 * Delayer servo_delay{TIM3};                 // 构造即自动把 tick() 注册进 TIM3 的队列
 *
 * servo_delay.start(500);                    // 等 500 个节拍
 * if (servo_delay.done()) { ... }            // 主循环或状态机里查询
 * servo_delay.delay(500);                    // 或直接阻塞等待
 * @endcode
 *
 * @note 1. `start()` 的参数是**节拍数**（cnt），不是时间，节拍时长由所绑定的定时器决定。
 * @note 2. 到期比较使用有符号差值 `(int32_t)(ticks - deadline)`，计数自然回绕，无需手动清零。
 * @note 3. 中断里只写 `ticks`，主循环只读，`uint32_t` 在 Cortex-M3 上是单次原子访问。
 */
class Delayer
{
private:
    TIM_TypeDef *__TIMX;              //!< 提供节拍的中断定时器
    volatile uint32_t ticks = 0;      //!< 累计节拍数（中断写）
    uint32_t deadline = 0;            //!< 到期时刻
    uint32_t duration = 0;            //!< 上次装载的节拍数，供 restart() 使用
    bool running = false;             //!< 是否处于运行中
    sys::InnerTimerManager::Handle __tick_handle = {}; //!< 本对象在定时器队列里的句柄

public:
    /**
     * @brief 构造并自动注册节拍回调
     * @param TIMX 提供节拍的中断定时器
     */
    inline explicit Delayer(TIM_TypeDef *TIMX) noexcept
        : __TIMX(TIMX),
          __tick_handle(sys::InnerTimerManager::add(TIMX, [this]()
                                                    { tick(); }))
    {
    }

    /// 析构时注销节拍回调
    inline ~Delayer() noexcept
    {
        sys::InnerTimerManager::remove(__tick_handle);
    }

    Delayer(const Delayer &) = delete;
    Delayer &operator=(const Delayer &) = delete;
    Delayer(Delayer &&) = delete;
    Delayer &operator=(Delayer &&) = delete;

    /**
     * @brief 定时中断调用：累加一个节拍
     *
     * 一条自增，可在中断里高频调用；不访问任何外设寄存器。
     */
    inline void tick() noexcept
    {
        ++ticks;
    }

    /**
     * @brief 装载延时
     * @param cnt 需要等待的节拍数（0 表示立即到期）
     *
     * 若绑定的定时器尚未初始化或未开启，节拍不会推进，`done()` 永远不会变真，
     * 此时直接跳过——立刻置为到期，不装载等待。需要真正等待请用 `delay()`
     * 或 `delay_ms()`，它们在这种情况下会自动改走 `sys::delay`。
     */
    void start(uint32_t cnt) noexcept;

    /// 按上次 `start()` 的节拍数重新装载
    inline void restart() noexcept
    {
        start(duration);
    }

    /// 单个节拍的时长（秒），直接向定时管理器查询，未初始化定时器时为 0
    [[nodiscard]] inline double tick_second() const noexcept
    {
        return sys::InnerTimerManager::period(__TIMX);
    }

    /**
     * @brief 节拍是否正在推进
     *
     * 定时器必须既初始化过（周期已知）又已开启中断，节拍才会自增。
     * 二者任一不满足时，`done()` 永远不会因时间推移而变真。
     */
    [[nodiscard]] inline bool is_ready() const noexcept
    {
        return sys::InnerTimerManager::is_ticking(__TIMX);
    }

    /// 单个节拍的时长（毫秒）
    [[nodiscard]] inline double tick_ms() const noexcept
    {
        return tick_second() * 1000.0;
    }

    /**
     * @brief 把毫秒换算成节拍数（向上取整，宁长勿短）
     * @param ms 毫秒数
     */
    [[nodiscard]] uint32_t ticks_of_ms(double ms) const noexcept;

    /// 按毫秒装载延时（内部换算成节拍）
    void start_ms(double ms) noexcept;

    /// 按毫秒阻塞延时（内部换算成节拍）
    void delay_ms(double ms) noexcept;

    /**
     * @brief 阻塞延时：装载后自行等待到点，无需使用者轮询
     * @param cnt 需要等待的节拍数（0 表示立即返回）
     *
     * 循环期间**不关中断**，节拍由使用者自己的定时中断推进，所以超声回波、
     * 编码器等中断仍然正常工作——这是它与 `Delay_ms`（占用 SysTick）、
     * `sys::delay_ms`（整个延时关中断）的关键区别。
     *
     * @note 若定时器未初始化或未开启，节拍不会推进，此时自动改用 `sys::delay_ms`
     *       等完同样长的时间（周期未知时直接返回），不会死等。
     */
    void delay(uint32_t cnt) noexcept;

    /// 取消本次延时，`done()` 之后恒为 true 直到下次 start()
    inline void stop() noexcept
    {
        running = false;
        deadline = ticks;
    }

    /**
     * @brief 是否已到期
     *
     * 用有符号差值比较，计数器回绕时结果仍然正确。
     */
    [[nodiscard]] inline bool done() const noexcept
    {
        return !running || int32_t(ticks - deadline) >= 0;
    }

    /// 是否处于运行中（到期后仍为 true，直到 stop()）
    [[nodiscard]] inline bool is_running() const noexcept
    {
        return running;
    }

    /// 已过去的节拍数（未启动时为 0）
    [[nodiscard]] inline uint32_t elapsed() const noexcept
    {
        const uint32_t used = ticks - (deadline - duration);
        return used < duration ? used : duration;
    }

    /// 剩余节拍数（已到期时为 0）
    [[nodiscard]] inline uint32_t remaining() const noexcept
    {
        if (done())
        {
            return 0;
        }
        return deadline - ticks;
    }

    /// 累计节拍数（每次 tick() 加一，永不回绕清零）
    [[nodiscard]] inline uint32_t get_ticks() const noexcept
    {
        return ticks;
    }

    /// 上次装载的节拍数
    [[nodiscard]] inline uint32_t get_duration() const noexcept
    {
        return duration;
    }
};

EMBMARTIN_SYS_NAMESPACE_END