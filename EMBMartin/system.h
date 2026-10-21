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

#include "macro.h"
#include "Delay.h"
#include "basic_tools.h"
#include "irq.h"

#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)

// 定时器更新中断：统一由本库实现（用户不再自行编写）
extern "C"
{
    void TIM1_UP_IRQHandler(void);
    void TIM2_IRQHandler(void);
    void TIM3_IRQHandler(void);
    void TIM4_IRQHandler(void);
}

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
 * @brief 内部定时器管理器
 *
 * 封装 STM32 硬件定时器的时间基准，并在更新中断里**按注册顺序依次调用**一个回调队列。
 * 定时器的更新中断服务函数（TIM1_UP_IRQHandler / TIM2_IRQHandler / TIM3_IRQHandler /
 * TIM4_IRQHandler）由本库直接实现，用户不再自行编写中断函数。
 *
 * 队列是静态的，每个定时器一条独立队列：
 * TIM1 -> 队列 0、TIM2 -> 队列 1、TIM3 -> 队列 2、TIM4 -> 队列 3。
 *
 * @code
 * void on_tick() { ... }
 *
 * InnerTimerManager timer{TIM3, 0.001, true, 0, 0};
 * auto slot = timer.add(on_tick);            // 或静态接口 InnerTimerManager::add(TIM3, on_tick)
 * InnerTimerManager::remove(slot);           // 成员注册由析构自动注销
 * @endcode
 *
 * @note 1. 回调运行在中断上下文，必须短小、不可阻塞。
 * @note 2. 队列满时 add() 返回的句柄 valid() 为 false。
 */
class InnerTimerManager
{
public:
    /// 单个更新中断回调的类型（内部缓冲区 32 字节，与 EXTIManager 一致）
    using Callback = etl::inplace_function<void(void), 32>;

    /// 每条队列最多容纳的回调个数
    static constexpr size_t MAX_CALLBACKS = 8;

    /**
     * @brief 回调槽位句柄，由 add() 返回，用于注销
     */
    struct Handle
    {
        uint8_t timer = 0xFF; //!< 定时器序号（0~3 对应 TIM1~TIM4），0xFF 表示无效
        uint8_t slot = 0xFF;  //!< 队列内槽位下标

        [[nodiscard]] inline bool valid() const noexcept
        {
            return timer != 0xFF;
        }
    };

private:
    /**
     * @brief 一个回调槽位
     *
     * 这里刻意**不**直接存 `Callback` 数组：`etl::inplace_function` 有非平凡的析构函数，
     * 其默认构造也非平凡，`std::array<Callback, N> obj{}` 因此无法常量初始化，编译器会在
     * `.init_array` 里生成一步「清零 + 逐个默认构造」。而全局对象的构造顺序由链接器决定，
     * 一旦这一步排在别的翻译单元的全局对象之后，别的对象在构造期注册进来的回调就会被
     * 整块清掉——中断照跑，但队列永远是空的。
     *
     * 改成「裸存储 + 使用标志」的平凡聚合体后，全零就是合法初态（所有槽位均空闲），
     * 静态存储期变量直接常量初始化，上述动态初始化不再存在。
     * 注意：不要给这两个成员加默认成员初始化器，否则又会让它变回非平凡默认构造。
     */
    struct Slot
    {
        alignas(Callback) unsigned char storage[sizeof(Callback)]; //!< 回调对象的就地存储
        bool used;                                                 //!< storage 里是否已构造了对象
    };

    /// 4 条独立队列；全零即全部空闲，可常量初始化，不参与 .init_array
    inline static std::array<std::array<Slot, MAX_CALLBACKS>, 4> __queues = {};

    /// 各定时器的更新周期（秒），由构造函数按实际装载的周期写入
    inline static std::array<double, 4> __periods = {};

    /// 各定时器是否已开启（TIM_Cmd 的结果）；未初始化时为 false
    inline static std::array<bool, 4> __opened = {};

    TIM_TypeDef *TIMX; ///< 定时器外设指针

    std::array<Handle, MAX_CALLBACKS> __handles = {}; //!< 构造期注册的初始队列句柄
    uint8_t __handle_count = 0;                       //!< 已登记的句柄数量

    /// 取出槽位里的回调对象（仅当 used 为 true 时有效）
    [[nodiscard]] inline static Callback &at(Slot &__slot) noexcept
    {
        return *std::launder(reinterpret_cast<Callback *>(__slot.storage));
    }

    /// 在槽位里就地构造回调对象
    inline static void construct(Slot &__slot, const Callback &__callback) noexcept
    {
        ::new (static_cast<void *>(__slot.storage)) Callback(__callback);
        __slot.used = true;
    }

    /// 析构槽位里的回调对象并置为空闲
    inline static void destroy(Slot &__slot) noexcept
    {
        if (!__slot.used)
            return;
        at(__slot).~Callback();
        __slot.used = false;
    }

    /**
     * @brief 执行某个定时器的整条队列（中断服务函数使用）
     * @param __idx 定时器序号（0~3）
     */
    inline static void invoke(uint8_t __idx) noexcept
    {
        if (__idx >= __queues.size())
            return;
        for (auto &__slot : __queues[__idx])
        {
            if (__slot.used)
                at(__slot)();
        }
    }

    static Handle __add(uint8_t __idx, const Callback &__callback) noexcept;

public:
    /**
     * @brief 构造函数，初始化定时器并使能更新中断
     *
     * @param TIMX 定时器外设指针
     * @param time_second 定时时间(秒)，支持最小精度0.0001秒
     * @param open 初始是否开启定时器，默认为true
     * @param PreemptionPriority 抢占优先级，默认为0
     * @param SubPriority 子优先级，默认为0
     * @param init 初始调用队列（每次更新中断按此顺序调用），默认为空
     */
    InnerTimerManager(
        TIM_TypeDef *TIMX, double time_second, // 支持最小精度：0.0001秒
        bool open = true,                      /* 初始是否开启定时器 */
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        std::initializer_list<Callback> init = {}) noexcept;

    /// 析构时注销构造期注册的初始队列
    ~InnerTimerManager() noexcept;

    InnerTimerManager(const InnerTimerManager &) = delete;
    InnerTimerManager &operator=(const InnerTimerManager &) = delete;
    InnerTimerManager(InnerTimerManager &&) = delete;
    InnerTimerManager &operator=(InnerTimerManager &&) = delete;

    /// 静态接口：向指定定时器队列尾部追加一个回调
    [[nodiscard]] static inline Handle add(TIM_TypeDef *TIMX, const Callback &__callback) noexcept
    {
        return __add(uint8_t(Get_TIM_Index(TIMX)), __callback);
    }

    /// 静态接口：按句柄注销一个回调
    static void remove(const Handle &__handle) noexcept;

    /// 成员接口：追加到本实例所属定时器的队列（返回的句柄由调用者自行 remove）
    [[nodiscard]] inline Handle add(const Callback &__callback) noexcept
    {
        return __add(uint8_t(Get_TIM_Index(TIMX)), __callback);
    }

    /// 静态接口：查询某个定时器当前的中断周期（秒），未初始化过则为 0
    [[nodiscard]] static double period(TIM_TypeDef *TIMX) noexcept;

    /// 静态接口：查询某个定时器的更新中断是否已开启
    [[nodiscard]] static bool is_open(TIM_TypeDef *TIMX) noexcept;

    /// 静态接口：定时器是否已初始化且节拍正在推进（周期已知且已开启）
    [[nodiscard]] static bool is_ticking(TIM_TypeDef *TIMX) noexcept
    {
        return period(TIMX) > 0.0 && is_open(TIMX);
    }

    /// 静态接口：查询某个定时器当前已注册的回调个数
    [[nodiscard]] static size_t count(TIM_TypeDef *TIMX) noexcept;

    /// 本实例所属定时器的中断周期（秒）
    [[nodiscard]] inline double period() const noexcept
    {
        return period(TIMX);
    }

    /// 本实例所属定时器的序号（0~3 对应 TIM1~TIM4）
    [[nodiscard]] inline uint8_t index() const noexcept
    {
        return uint8_t(Get_TIM_Index(TIMX));
    }

    inline void open() noexcept
    {
        TIM_Cmd(TIMX, ENABLE);
        __opened[uint8_t(Get_TIM_Index(TIMX))] = true;
    }

    inline void close() noexcept
    {
        TIM_Cmd(TIMX, DISABLE);
        __opened[uint8_t(Get_TIM_Index(TIMX))] = false;
    }

    friend void ::TIM1_UP_IRQHandler(void);
    friend void ::TIM2_IRQHandler(void);
    friend void ::TIM3_IRQHandler(void);
    friend void ::TIM4_IRQHandler(void);
};

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

EMBMARTIN_SYS_NAMESPACE_END

