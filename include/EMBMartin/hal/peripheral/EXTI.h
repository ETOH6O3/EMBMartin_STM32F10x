#pragma once

#include <etl/inplace_function.h>

#include "EMBMartin/hal/gpio/inpin.h"

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

EMBMARTIN_HAL_NAMESPACE_BEGIN

class EXTIManager
{
public:
    /// 单个 EXTI line 的回调类型
    // 回调捕获对象的内部缓冲区为 32 字节；超过该大小的回调无法用于此接口。
    using Callback = etl::inplace_function<void(void), 32>;

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

    ~EXTIManager(); // 禁用对应 EXTI line、注销回调

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


EMBMARTIN_HAL_NAMESPACE_END
