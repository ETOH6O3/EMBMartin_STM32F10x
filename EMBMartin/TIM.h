#ifndef EMBMARTIN_EXTI_H
#define EMBMARTIN_EXTI_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"


EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief 外部定时器类，用于处理 STM32 的外部定时器功能
 *
 * 该类封装了外部定时器的配置和使用，通过指定的定时器外设和回音引脚来实现定时功能。
 * 主要用于超声波传感器等需要外部触发的定时测量场景。
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
 * @brief 计数传感器类，用于处理基于外部中断的计数传感器
 *
 * 该类封装了计数传感器的配置和中断处理功能，通过指定的 GPIO 引脚来检测信号边沿变化并进行计数。
 * 适用于如霍尔传感器、光电编码器等需要计数的传感器设备。
 */
class CounterSensor
{
private:
    bool edge;   //!< 触发方式，true 表示上升沿触发，false 表示下降沿触发
    GPIOPin pin; //!< GPIO 引脚信息

public:
    /**
     * @brief 构造一个计数传感器对象
     * @param _pin GPIOPin 结构体，包含端口和引脚信息
     * @param _edge 触发方式，true 表示上升沿触发，false 表示下降沿触发
     * @param PreemptionPriority 中断抢占优先级，默认为 0
     * @param SubPriority 中断子优先级，默认为 0
     * @param _GPIO_Speed GPIO 速度，默认为 GPIO_Speed_50MHz
     * @param _EXTI_Mode EXTI 模式，默认为 EXTI_Mode_Interrupt (中断模式)
     */
    CounterSensor(
        GPIOPin _pin, bool _edge,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        GPIOSpeed_TypeDef _GPIO_Speed = GPIO_Speed_50MHz,
        EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt) noexcept;
};


/**
 * @brief 使用 STM32 内部硬件的旋转编码器接口
 * @note 只能使用通用/高级定时器的 CH1 和 CH2
 */
class RotaryEncoder
{
private:
    TIM_TypeDef *__TIMX;

public:
    RotaryEncoder(TIM_TypeDef *TIMX, uint8_t TIMx_REMAP, bool reverse = false) noexcept;
    inline int16_t get() noexcept
    {
        auto temp = TIM_GetCounter(__TIMX);
        TIM_SetCounter(__TIMX, 0);
        return temp;
    }
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
    inline void start(uint32_t cnt) noexcept
    {
        duration = cnt;

        if (!is_ready())
        {
            running = false; // 节拍不会自增，跳过等待
            deadline = ticks;
            return;
        }

        deadline = ticks + cnt;
        running = true;
    }

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
    [[nodiscard]] inline uint32_t ticks_of_ms(double ms) const noexcept
    {
        const double __tick_ms = tick_ms();
        if (__tick_ms <= 0.0)
        {
            return 0; // 定时器还没初始化，无法换算
        }
        return uint32_t(ms / __tick_ms + 0.999);
    }

    /// 按毫秒装载延时（内部换算成节拍）
    inline void start_ms(double ms) noexcept
    {
        start(ticks_of_ms(ms));
    }

    /// 按毫秒阻塞延时（内部换算成节拍）
    inline void delay_ms(double ms) noexcept
    {
        delay(ticks_of_ms(ms));
    }

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
    inline void delay(uint32_t cnt) noexcept
    {
        start(cnt);

        if (running)
        {
            while (!done())
            {
                EMBMARTIN_KEEP_CODE_ORDER;
            }
            return;
        }

        // 定时中断没跑起来，退化为阻塞延时
        const double __second = tick_second();
        if (__second > 0.0 && cnt > 0)
        {
            sys::delay_ms(intmax_t(double(cnt) * __second * 1000.0 + 0.5));
        }
    }

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

EMBMARTIN_STM32F10X_NAMESPACE_END


#include "PWM.h" // 理论上属于

#endif // EMBMARTIN_EXTI_H