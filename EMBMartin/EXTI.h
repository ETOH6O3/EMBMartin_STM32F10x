#pragma once

#include <functional>

#include "inpin.h"
#include "outpin.h"
// #include "stream.h"

extern "C"
{
    void EXTI0_IRQHandler(void);
    void EXTI1_IRQHandler(void);
    void EXTI2_IRQHandler(void);
    void EXTI3_IRQHandler(void);
    void EXTI4_IRQHandler(void);
    void EXTI9_5_IRQHandler(void);
    void EXTI15_10_IRQHandler(void);
}

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class EXTIManager
{
public:
    /// 单个 EXTI line 的回调类型
    using Callback = std::function<void(void)>;

private:
    inline static EXTIManager *__instances[16] = {};

    Inpin __EXTI_pin;
    Callback __operate; //!< 回调随实例一起构造/析构，先于使能中断

    /**
     * @brief 触发本实例的回调（中断服务函数使用）
     */
    inline void invoke() const noexcept
    {
        if (__operate)
            __operate();
    }

public:
    [[nodiscard]] EXTIManager(const GPIOPin &gpio_pin, const Callback &operate,
                              EXTITrigger_TypeDef trigger = EXTI_Trigger_Falling, uint8_t __PreemptionPriority = 0, uint8_t __SubPriority = 0,
                              EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt) noexcept;

    inline ~EXTIManager() // 禁用对应 EXTI line、注销回调
    {
        auto __idx = __EXTI_pin.idx();
        if (__instances[__idx] == this)
            __instances[__idx] = nullptr;

        EXTI->IMR &= ~__EXTI_pin.get_pin().pin;
        EXTI->EMR &= ~__EXTI_pin.get_pin().pin;

        EXTI_ClearITPendingBit(Get_EXTI_Line(__EXTI_pin.get_pin().pin));
    }

    EXTIManager(const EXTIManager &) = delete;
    EXTIManager &operator=(const EXTIManager &) = delete;
    EXTIManager(EXTIManager &&) = delete;
    EXTIManager &operator=(EXTIManager &&) = delete;

    inline void set_operate(const Callback &operate) noexcept
    {
        __operate = operate;
    }

    inline void software_trig()
    {
        EXTI_GenerateSWInterrupt(__EXTI_pin.get_pin().pin);
    }

    inline const Inpin &get_pin() const noexcept
    {
        return __EXTI_pin;
    }

    /**
     * @brief 取出指定 EXTI line 上已注册的实例
     * @param __idx EXTI line 序号（0~15），即引脚在端口内的位号
     * @return 对应实例指针；该 line 尚未注册时返回 nullptr
     * @note 只需读取一个常量初始化的指针数组，可在中断中安全调用
     */
    static EXTIManager *instance(uint8_t __idx) noexcept
    {
        return __idx < 16 ? __instances[__idx] : nullptr;
    }

    friend void ::EXTI0_IRQHandler(void);
    friend void ::EXTI1_IRQHandler(void);
    friend void ::EXTI2_IRQHandler(void);
    friend void ::EXTI3_IRQHandler(void);
    friend void ::EXTI4_IRQHandler(void);
    friend void ::EXTI9_5_IRQHandler(void);
    friend void ::EXTI15_10_IRQHandler(void);
};

/**
 * @brief 旋转编码器类，用于处理正交编码器输入
 *
 * 该类封装了旋转编码器的配置和处理功能，通过两个相位信号 (A 相和 B 相) 来检测旋转方向和计数。
 * 适用于机械旋转编码器等需要旋转位置检测的设备。
 */
class EXTIRotaryEncoder
{
private:
    const EXTIManager exti_a; //!< A 相外部中断管理器
    const EXTIManager exti_b; //!< B 相外部中断管理器
    int16_t count = 0;        //!< 当前计数值

    /**
     * @brief A 相引脚中断处理函数
     *
     * 当 A 相引脚发生中断时调用此函数，根据 B 相引脚的电平状态判断旋转方向并更新计数
     */
    inline void pin_a_handler() noexcept
    {
        if (exti_b.get_pin() == 0)
        {
            count++;
        }
    }

    /**
     * @brief B 相引脚中断处理函数
     *
     * 当 B 相引脚发生中断时调用此函数，根据 A 相引脚的电平状态判断旋转方向并更新计数
     */
    inline void pin_b_handler() noexcept
    {
        if (exti_a.get_pin() == 0)
        {
            count--;
        }
    }

public:
    /**
     * @brief 构造一个旋转编码器对象
     * @param pin_a A 相引脚 GPIOPin 结构体
     * @param pin_b B 相引脚 GPIOPin 结构体
     * @param PreemptionPriority 中断抢占优先级，默认为 0
     * @param SubPriority 中断子优先级，默认为 0
     * @param GPIO_Speed GPIO 速度，默认为 GPIO_Speed_50MHz
     * @param _EXTI_Mode EXTI 模式，默认为 EXTI_Mode_Interrupt (中断模式)
     */
    inline EXTIRotaryEncoder(
        GPIOPin pin_a, GPIOPin pin_b,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt) noexcept
        : exti_a{pin_a, [&]()
                 { this->pin_a_handler(); },
                 EXTI_Trigger_Falling, PreemptionPriority, SubPriority,
                 _EXTI_Mode},
          exti_b{pin_b, [&]()
                 { this->pin_b_handler(); },
                 EXTI_Trigger_Falling, PreemptionPriority, SubPriority,
                 _EXTI_Mode}
    {
    }

    /**
     * @brief 获取当前计数值
     * @return int16_t 当前计数值
     */
    inline int16_t get_count() const noexcept { return count; }

    /**
     * @brief 获取速度
     * 
     * @return int16_t 当前速度
     */
    inline int16_t get_speed() noexcept
    {
        const auto rslt = count;
        count = 0;
        return rslt;
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END