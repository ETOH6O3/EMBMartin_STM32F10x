/**
 ******************************************************************************
 * @file    tools.h
 * @author  孙鸣淼
 * @brief   这里存放一些嵌入式开发中常用的工具 （基于 STM32 标准外设库）
 ******************************************************************************
 * @attention
 * 0. 需要预定义宏 STM32_DEVICE_HEADER 为对应的 STM32 设备头文件名，如 stm32f10x.h （不得含引号）
 *    或者在 macro.h 中定义 STM32_DEVICE_HEADER 宏，这也是等价的
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#ifndef EMBMARTIN_TOOLS_H
#define EMBMARTIN_TOOLS_H

#include <cassert>
#include <type_traits>
#include <numeric>
#include <cmath>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "Delay.h"
STM32_BEGIN

// TODO: 时钟使能用 HAL 库重写

/**
 * @brief GPIO 引脚封装结构体
 *
 * 该结构体用于封装一个 GPIO 引脚的信息，包括端口和引脚号，
 * 方便在嵌入式系统中进行 GPIO 操作。
 */
struct GPIOPin
{
    GPIO_TypeDef *port; //!< GPIO 端口指针
    uint16_t pin;       //!< GPIO 引脚号
};

inline bool operator==(const GPIOPin p1, const GPIOPin p2)
{
    return (p1.pin == p2.pin) && (p1.port == p2.port);
}
const GPIOPin null_pin{nullptr, 0};
const GPIOPin PA0{GPIOA, GPIO_Pin_0};
const GPIOPin PA1{GPIOA, GPIO_Pin_1};
const GPIOPin PA2{GPIOA, GPIO_Pin_2};
const GPIOPin PA3{GPIOA, GPIO_Pin_3};
const GPIOPin PA4{GPIOA, GPIO_Pin_4};
const GPIOPin PA5{GPIOA, GPIO_Pin_5};
const GPIOPin PA6{GPIOA, GPIO_Pin_6};
const GPIOPin PA7{GPIOA, GPIO_Pin_7};
const GPIOPin PA8{GPIOA, GPIO_Pin_8};
const GPIOPin PA9{GPIOA, GPIO_Pin_9};
const GPIOPin PA10{GPIOA, GPIO_Pin_10};
const GPIOPin PA11{GPIOA, GPIO_Pin_11};
const GPIOPin PA12{GPIOA, GPIO_Pin_12};
const GPIOPin PA13{GPIOA, GPIO_Pin_13};
const GPIOPin PA14{GPIOA, GPIO_Pin_14};
const GPIOPin PA15{GPIOA, GPIO_Pin_15};
const GPIOPin PB0{GPIOB, GPIO_Pin_0};
const GPIOPin PB1{GPIOB, GPIO_Pin_1};
const GPIOPin PB2{GPIOB, GPIO_Pin_2};
const GPIOPin PB3{GPIOB, GPIO_Pin_3};
const GPIOPin PB4{GPIOB, GPIO_Pin_4};
const GPIOPin PB5{GPIOB, GPIO_Pin_5};
const GPIOPin PB6{GPIOB, GPIO_Pin_6};
const GPIOPin PB7{GPIOB, GPIO_Pin_7};
const GPIOPin PB8{GPIOB, GPIO_Pin_8};
const GPIOPin PB9{GPIOB, GPIO_Pin_9};
const GPIOPin PB10{GPIOB, GPIO_Pin_10};
const GPIOPin PB11{GPIOB, GPIO_Pin_11};
const GPIOPin PB12{GPIOB, GPIO_Pin_12};
const GPIOPin PB13{GPIOB, GPIO_Pin_13};
const GPIOPin PB14{GPIOB, GPIO_Pin_14};
const GPIOPin PB15{GPIOB, GPIO_Pin_15};
const GPIOPin PC0{GPIOC, GPIO_Pin_0};
const GPIOPin PC1{GPIOC, GPIO_Pin_1};
const GPIOPin PC2{GPIOC, GPIO_Pin_2};
const GPIOPin PC3{GPIOC, GPIO_Pin_3};
const GPIOPin PC4{GPIOC, GPIO_Pin_4};
const GPIOPin PC5{GPIOC, GPIO_Pin_5};
const GPIOPin PC6{GPIOC, GPIO_Pin_6};
const GPIOPin PC7{GPIOC, GPIO_Pin_7};
const GPIOPin PC8{GPIOC, GPIO_Pin_8};
const GPIOPin PC9{GPIOC, GPIO_Pin_9};
const GPIOPin PC10{GPIOC, GPIO_Pin_10};
const GPIOPin PC11{GPIOC, GPIO_Pin_11};
const GPIOPin PC12{GPIOC, GPIO_Pin_12};
const GPIOPin PC13{GPIOC, GPIO_Pin_13};
const GPIOPin PC14{GPIOC, GPIO_Pin_14};
const GPIOPin PC15{GPIOC, GPIO_Pin_15};
const GPIOPin PD0{GPIOD, GPIO_Pin_0};
const GPIOPin PD1{GPIOD, GPIO_Pin_1};
const GPIOPin PD2{GPIOD, GPIO_Pin_2};
const GPIOPin PD3{GPIOD, GPIO_Pin_3};
const GPIOPin PD4{GPIOD, GPIO_Pin_4};
const GPIOPin PD5{GPIOD, GPIO_Pin_5};
const GPIOPin PD6{GPIOD, GPIO_Pin_6};
const GPIOPin PD7{GPIOD, GPIO_Pin_7};
const GPIOPin PD8{GPIOD, GPIO_Pin_8};
const GPIOPin PD9{GPIOD, GPIO_Pin_9};
const GPIOPin PD10{GPIOD, GPIO_Pin_10};
const GPIOPin PD11{GPIOD, GPIO_Pin_11};
const GPIOPin PD12{GPIOD, GPIO_Pin_12};
const GPIOPin PD13{GPIOD, GPIO_Pin_13};
const GPIOPin PD14{GPIOD, GPIO_Pin_14};
const GPIOPin PD15{GPIOD, GPIO_Pin_15};
const GPIOPin PE0{GPIOE, GPIO_Pin_0};
const GPIOPin PE1{GPIOE, GPIO_Pin_1};
const GPIOPin PE2{GPIOE, GPIO_Pin_2};
const GPIOPin PE3{GPIOE, GPIO_Pin_3};
const GPIOPin PE4{GPIOE, GPIO_Pin_4};
const GPIOPin PE5{GPIOE, GPIO_Pin_5};
const GPIOPin PE6{GPIOE, GPIO_Pin_6};
const GPIOPin PE7{GPIOE, GPIO_Pin_7};
const GPIOPin PE8{GPIOE, GPIO_Pin_8};
const GPIOPin PE9{GPIOE, GPIO_Pin_9};
const GPIOPin PE10{GPIOE, GPIO_Pin_10};
const GPIOPin PE11{GPIOE, GPIO_Pin_11};
const GPIOPin PE12{GPIOE, GPIO_Pin_12};
const GPIOPin PE13{GPIOE, GPIO_Pin_13};
const GPIOPin PE14{GPIOE, GPIO_Pin_14};
const GPIOPin PE15{GPIOE, GPIO_Pin_15};
/**
 * @brief 通用定时器输出比较端口（未复用）
 * @param _GEN_TIM_CH_TO_GPIOPin [TIM 序号][CH 序号]
 */
const GPIOPin _GEN_TIM_CH_TO_GPIOPin[][4] = {
    {PA8, PA9, PA10, PA11}, // TIM1 ( 兼容通用定时器 )
    {PA0, PA1, PA2, PA3},   // TIM2
    {PA6, PA7, PB0, PB1},   // TIM3
    {PB6, PB7, PB8, PB9},   // TIM4
};

/**
 * @brief 通用定时器输出比较端口（部分重映像 01）
 * @param _GEN_TIM_CH_TO_GPIOPin_REMAP_01 [TIM 序号][CH 序号]
 */
const GPIOPin _GEN_TIM_CH_TO_GPIOPin_REMAP_01[][4] = {
    {PA8, PA9, PA10, PA11},   // TIM1 ( 兼容通用定时器 )(此模式下复用的端口是高级端口)
    {PA15, PB3, PA2, PA3},    // TIM2
    {PB4, PB5, PB0, PB1},     // TIM3 (没有 01 复用，这里设置为与 10 相同)
    {PD12, PD13, PD14, PD15}, // TIM4
};

/**
 * @brief 通用定时器输出比较端口（部分重映像 10）
 * @param _GEN_TIM_CH_TO_GPIOPin_REMAP_10 [TIM 序号][CH 序号]
 */
const GPIOPin _GEN_TIM_CH_TO_GPIOPin_REMAP_10[][4] = {
    {PA8, PA9, PA10, PA11}, // TIM1 ( 兼容通用定时器 )(没有 10 复用，这里设置为与 01 相同)
    {PA0, PA1, PB10, PB11}, // TIM2
    {PB4, PB5, PB0, PB1},   // TIM3
    {PB6, PB7, PB8, PB9},   // TIM4 (没有 10 复用，这里设置为与 00 相同)
};

/**
 * @brief 通用定时器输出比较端口（完全重映像）
 * @param _GEN_TIM_CH_TO_GPIOPin_REMAP_11 [TIM 序号][CH 序号]
 */
const GPIOPin _GEN_TIM_CH_TO_GPIOPin_REMAP_11[][4] = {
    {PE9, PE11, PE13, PE14},  // TIM1 ( 兼容通用定时器 )
    {PA15, PB3, PB10, PB11},  // TIM2
    {PC6, PC7, PC8, PC9},     // TIM3
    {PD12, PD13, PD14, PD15}, // TIM4
};

/**
 * @brief 通用定时器输出比较端口
 * @param _GEN_TIM_CH_TO_GPIOPin_REMAP [TIMx_REMAP][TIM 序号][CH 序号]
 */
const GPIOPin (*const _GEN_TIM_CH_TO_GPIOPin_REMAP[4])[4] = {
    _GEN_TIM_CH_TO_GPIOPin,          // 未复用模式
    _GEN_TIM_CH_TO_GPIOPin_REMAP_01, // 部分重映像 01
    _GEN_TIM_CH_TO_GPIOPin_REMAP_10, // 部分重映像 10
    _GEN_TIM_CH_TO_GPIOPin_REMAP_11  // 完全重映像
};

uint8_t Get_GPIO_PortSource(GPIO_TypeDef *port) noexcept;
uint8_t Get_GPIO_PinSource(uint16_t pin) noexcept;
uint32_t Get_RCC_APB2Periph(GPIO_TypeDef *port) noexcept;
uint32_t Get_EXTI_Line(uint16_t pin) noexcept; // TODO: 使能用 HAL 库重写
IRQn Get_IRQChannel(uint16_t pin) noexcept;
uint32_t get_TIM_RCC_APB1Periph(TIM_TypeDef *__TIMX) noexcept;
auto Get_TIM_Index(TIM_TypeDef *TIMX) noexcept -> size_t;

/**
 * @brief 读取按键状态并进行消抖处理
 * @param GPIOx GPIO 端口指针
 * @param pin GPIO 引脚号
 * @param trigger 按下状态，默认为 false, 低电平为按下状态；若设为 true，则高电平为按下状态
 * @return 按键按下并释放后返回 true，否则返回 false
 */
inline bool read_key(GPIO_TypeDef *GPIOx, uint16_t pin, bool trigger = false, intmax_t debounce_delay_ms = 20) noexcept
{
    if (GPIO_ReadInputDataBit(GPIOx, pin) == trigger) // 按键按下
    {
        Delay_ms(debounce_delay_ms); // 消抖动
        while (GPIO_ReadInputDataBit(GPIOx, pin) == trigger)
            ;                        // 等待按键释放
        Delay_ms(debounce_delay_ms); // 消抖动
        return true;
    }
    return false;
}
/**
 * @brief 读取按键状态并进行消抖处理
 * @param pin GPIO 引脚封装结构体
 * @param trigger 按下状态，默认为 false, 低电平为按下状态；若设为 true，则高电平为按下状态
 * @param debounce_delay_ms 消抖动延迟时间，单位为毫秒，默认为 20 毫秒
 * @return 按键按下并释放后返回 true，否则返回 false
 */
inline bool read_key(GPIOPin pin, bool trigger = false, intmax_t debounce_delay_ms = 20) noexcept
{
    return read_key(pin.port, pin.pin, trigger, debounce_delay_ms);
}

/**
 * @brief 将容器中的多个 GPIO 引脚合并为一个 GPIOPin 对象
 *
 * 此函数通过按位或操作将多个 GPIO 引脚的 pin 值合并成一个新的 GPIOPin 对象。
 * 合并后的对象保留第一个引脚的端口，并将所有引脚的 pin 值进行或运算组合。
 *
 * @tparam Container 容器类型，其元素类型必须为 GPIOPin
 * @param pins 包含多个 GPIOPin 对象的容器
 * @return GPIOPin 合并后的 GPIOPin 对象，其 port 为第一个引脚的 port，
 *         pin 为所有引脚 pin 值按位或运算的结果
 *
 * @note 使用 static_assert 确保容器元素类型为 GPIOPin
 */
template <typename Container>
inline GPIOPin merge_pins(const Container &pins) noexcept
{
    static_assert(std::is_same_v<typename Container::value_type, GPIOPin>,
                  "容器元素类型必须为 GPIOPin");
    assert(std::accumulate(pins.begin(), pins.end(), bool(1), [&](bool last_rslt, GPIOPin pin2)
                           { return last_rslt && (pins[0].port == pin2.port); }) &&
           "所有引脚必须属于同一端口");

    return {pins[0].port, std::accumulate(
                              pins.begin(),
                              pins.end(),
                              (uint16_t)0,
                              [](uint16_t last_rslt, GPIOPin pin2) -> uint16_t
                              { return last_rslt | pin2.pin; })};
}

/**
 * @brief 按键类，用于处理 STM32 GPIO 按键输入
 *
 * 该类封装了按键的 GPIO 配置和状态读取功能，支持按键消抖处理。
 * 可以配置按键的触发方式（高电平或低电平触发）。
 */
class Key
{
private:
    GPIOPin pin;  //!< GPIO 引脚信息
    bool trigger; //!< 按键触发状态，false 表示低电平触发，true 表示高电平触发

public:
    /**
     * @brief 构造一个按键对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param trigger 按键触发状态，默认为 false（低电平触发）
     * @note 会在构造时自动初始化 GPIO 为输入模式
     */
    inline Key(GPIOPin p, bool trigger = false) noexcept : pin(p), trigger(trigger)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = trigger ? GPIO_Mode_IPD : GPIO_Mode_IPU,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);
    }

    /**
     * @brief 构造一个按键对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param trigger 按键触发状态，默认为 false（低电平触发）
     */
    inline Key(GPIO_TypeDef *port, uint16_t pin, bool trigger = false) noexcept : Key{GPIOPin{port, pin}, trigger} {}

    /**
     * @brief 检测按键是否被按下
     * @return bool 按键按下并释放后返回 true，否则返回 false
     * @note 内部已实现按键消抖处理
     */
    inline bool is_pressed() const noexcept
    {
        return read_key(this->pin, this->trigger);
    }
};


/**
 * @brief 
 * 安全的输出引脚类，与普通输出引脚类不同的是，该类初始化为 null_pin 时不会导致未定义行为
 * 
 * 同时，该类封装了一些基础的输出操作方法，如 set、reset、toggle 等
 * 
 */
class OutPin
{
private:
    GPIOPin pin;        //!< GPIO 引脚信息
    bool current_level; //!< 目前输出
public:
    inline OutPin(GPIOPin p) noexcept : pin(p), current_level(0)
    {
        if (p == null_pin)
            return;
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->reset();
    }
    inline void set() noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_SET);
        current_level = true;
    }
    inline void reset() noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, Bit_RESET);
        current_level = false;
    }
    inline void toggle() noexcept
    {
        if (pin == null_pin)
            return;
        GPIO_WriteBit(this->pin.port, this->pin.pin, current_level ? Bit_RESET : Bit_SET);
        current_level = !current_level;
    }
    inline bool get_level()noexcept
    {
        return current_level;
    }
};

/**
 * @brief LED 类，用于控制 STM32 GPIO 连接的 LED
 *
 * 该类封装了 LED 的 GPIO 配置和控制功能，支持点亮、熄灭和翻转操作。
 */
class LED
{
private:
    GPIOPin pin;       //!< GPIO 引脚信息
    bool driven_level; //!< 驱动电平，false 表示低电平点亮，true 表示高电平点亮

public:
    /**
     * @brief 构造一个 LED 对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline LED(GPIOPin p, bool driven_level = false) noexcept : pin(p), driven_level(driven_level)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->off(); // 初始化时熄灭 LED
    }

    /**
     * @brief 构造一个 LED 对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param driven_level 驱动电平，默认为 false（低电平点亮 LED）
     */
    inline LED(GPIO_TypeDef *port, uint16_t pin, bool driven_level = false) noexcept : LED{GPIOPin{port, pin}, driven_level} {}

    /**
     * @brief 点亮 LED
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相应电平以点亮 LED
     */
    inline void on() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_SET : Bit_RESET);
    }

    /**
     * @brief 熄灭 LED
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相反电平以熄灭 LED
     */
    inline void off() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_RESET : Bit_SET);
    }

    /**
     * @brief 切换 LED 状态
     *
     * 翻转当前 LED 的状态，如果当前是点亮则熄灭，如果当前是熄灭则点亮
     */
    inline void toggle() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin,
                      BitAction(!GPIO_ReadOutputDataBit(this->pin.port, this->pin.pin)));
    }
};

/**
 * @brief 蜂鸣器类，用于控制 STM32 GPIO 连接的蜂鸣器
 *
 * 该类封装了蜂鸣器的 GPIO 配置和控制功能，支持基本的开关控制以及蜂鸣操作。
 */
class Buzzer
{
private:
    GPIOPin pin;       //!< GPIO 引脚信息
    bool driven_level; //!< 驱动电平，false 表示低电平驱动，true 表示高电平驱动

public:
    /**
     * @brief 构造一个蜂鸣器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     * @note 会在构造时自动初始化 GPIO 为推挽输出模式
     */
    inline Buzzer(GPIOPin p, bool driven_level = false) noexcept : pin(p), driven_level(driven_level)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);

        this->off(); // 初始化时关闭蜂鸣器
    }

    /**
     * @brief 构造一个蜂鸣器对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     * @param driven_level 驱动电平，默认为 false（低电平驱动蜂鸣器）
     */
    inline Buzzer(GPIO_TypeDef *port, uint16_t pin, bool driven_level = false) noexcept : Buzzer{GPIOPin{port, pin}, driven_level} {}

    /**
     * @brief 打开蜂鸣器
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相应电平以打开蜂鸣器
     */
    inline void on() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_SET : Bit_RESET);
    }

    /**
     * @brief 关闭蜂鸣器
     *
     * 根据 driven_level 设置，将对应的 GPIO 引脚设置为相反电平以关闭蜂鸣器
     */
    inline void off() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin, this->driven_level ? Bit_RESET : Bit_SET);
    }

    /**
     * @brief 切换蜂鸣器状态
     *
     * 翻转当前蜂鸣器的状态，如果当前是开启则关闭，如果当前是关闭则开启
     */
    inline void toggle() const noexcept
    {
        GPIO_WriteBit(this->pin.port, this->pin.pin,
                      BitAction(!GPIO_ReadOutputDataBit(this->pin.port, this->pin.pin)));
    }

    /**
     * @brief 发出一次蜂鸣声
     * @param duration_ms 蜂鸣持续时间（毫秒），默认为 100ms
     */
    inline void beep(intmax_t duration_ms = 100) const noexcept
    {
        this->on();
        Delay_ms(duration_ms);
        this->off();
    }

    /**
     * @brief 发出多次蜂鸣声
     * @param times 蜂鸣次数
     * @param duration_ms 每次蜂鸣持续时间（毫秒）
     * @param interval_ms 每次蜂鸣之间的间隔时间（毫秒），默认为 500ms
     */
    inline void beep(intmax_t times, intmax_t duration_ms, intmax_t interval_ms = 500) const noexcept
    {
        for (intmax_t i = 0; i < times - 1; i++)
        {
            this->beep(duration_ms);
            Delay_ms(interval_ms);
        }
        this->beep(duration_ms);
    }
};

/**
 * @brief 传感器类，用于读取 STM32 GPIO 连接的传感器状态
 *
 * 该类封装了传感器的 GPIO 配置和状态读取功能，适用于简单的数字传感器。
 */
class Sensor
{
private:
    GPIOPin pin; //!< GPIO 引脚信息

public:
    /**
     * @brief 构造一个传感器对象
     * @param p GPIOPin 结构体，包含端口和引脚信息
     */
    inline Sensor(GPIOPin p) noexcept : pin(p)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = p.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_IPU,
        };
        GPIO_Init(p.port, &GPIO_InitStruct);
    }

    /**
     * @brief 构造一个传感器对象
     * @param port GPIO 端口指针
     * @param pin GPIO 引脚号
     */
    inline Sensor(GPIO_TypeDef *port, uint16_t pin) noexcept : Sensor{GPIOPin{port, pin}} {}

    /**
     * @brief 获取传感器状态
     * @return bool 传感器状态，高电平返回 true，低电平返回 false
     */
    inline bool get() const noexcept
    {
        return GPIO_ReadInputDataBit(this->pin.port, this->pin.pin) == Bit_SET;
    }
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
 * @brief 旋转编码器类，用于处理正交编码器输入
 *
 * 该类封装了旋转编码器的配置和处理功能，通过两个相位信号 (A 相和 B 相) 来检测旋转方向和计数。
 * 适用于机械旋转编码器等需要旋转位置检测的设备。
 */
class RotaryEncoder
{
private:
    GPIOPin pin_a;     //!< A 相引脚
    GPIOPin pin_b;     //!< B 相引脚
    int16_t count = 0; //!< 当前计数值

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
    RotaryEncoder(
        GPIOPin pin_a, GPIOPin pin_b,
        uint8_t PreemptionPriority = 0, uint8_t SubPriority = 0,
        GPIOSpeed_TypeDef GPIO_Speed = GPIO_Speed_50MHz,
        EXTIMode_TypeDef _EXTI_Mode = EXTI_Mode_Interrupt) noexcept;

    /**
     * @brief 获取当前计数值
     * @return int16_t 当前计数值
     */
    inline int16_t get_count() const noexcept { return count; }

    /**
     * @brief A 相引脚中断处理函数
     *
     * 当 A 相引脚发生中断时调用此函数，根据 B 相引脚的电平状态判断旋转方向并更新计数
     */
    inline void pin_a_handler() noexcept
    {
        if (GPIO_ReadInputDataBit(pin_b.port, pin_b.pin) == 0)
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
        if (GPIO_ReadInputDataBit(pin_a.port, pin_a.pin) == 0)
        {
            count--;
        }
    }
};

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
 * @brief PWM 类，用于生成 PWM 信号控制设备接口
 *
 * 该类封装了 STM32 定时器的 PWM 功能，可以方便地配置和控制 PWM 输出。
 * 支持设置 PWM 频率、占空比和分辨率，并可动态调整占空比。
 *
 * @note
 * 1. 输出引脚是根据定时器和通道自动确定的，无需手动指定
 * 2. Reso_permillage 参数建议使用能被 1000 整除的值，以保证计算精度
 * 3. 占空比范围为 0-100 的整数百分比
 *
 */
class PWM
{
private:
    int _Duty_permillage;
    int _Freq_HZ;
    int _Reso_permillage;
    TIM_TypeDef *_TIMX;
    int _CHn;

    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP, GPIOPin output_pin /* 只能是规定的复用引脚 */,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept;

public:
    /**
     * @brief PWM 构造函数
     * @param TIMX 定时器外设指针
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     * @param Duty_permillage 初始占空比（0-1000），默认 0%
     * @param Freq_HZ PWM 频率（Hz），默认 1kHz
     * @param Reso_permillage PWM 分辨率（千分之一），默认 10‰
     *
     * @note 有一些重映像会导致构造函数自动关闭调试端口，具体查阅参考手册
     * @note Reso_permillage 建议能被 1000 整除，以确保计算准确性
     * @note output_pin 根据定时器、通道和重映像自动选择
     */
    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, _GEN_TIM_CH_TO_GPIOPin_REMAP[TIMx_REMAP][Get_TIM_Index(TIMX)][CHn - 1],
              Duty_permillage, Freq_HZ, Reso_permillage) {};
    /**
     * @brief 设置 PWM 占空比
     * @param Duty_permillage 占空比（0-100）
     *
     * @note 占空比为千分比值，0 表示始终低电平，100 表示始终高电平
     * @note 该函数会根据设定的分辨率自动计算并设置比较寄存器值
     */
    void set_duty(int Duty_permillage) noexcept;
    inline auto get_duty() const noexcept
    {
        return _Duty_permillage;
    }
};

/**
 * @brief 呼吸灯类，用于实现呼吸灯效果
 *
 */
class BreathingLED : public PWM
{
private:
    bool _driven_level;
    int _period_s;

public:
    /**
     * @brief 构造一个呼吸灯对象
     *
     * 呼吸灯是基于 PWM 实现的，通过周期性地改变 LED 亮度来模拟 "呼吸" 效果。
     *
     * @param TIMX 定时器外设指针，用于产生 PWM 信号
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     *                   - 0b00: 无重映像
     *                   - 0b01: 部分重映像 01
     *                   - 0b10: 部分重映像 10
     *                   - 0b11: 完全重映像
     * @param driven_level 驱动电平，false 表示低电平点亮 LED，true 表示高电平点亮 LED
     * @param period_s 呼吸周期（秒），即从暗到亮再到暗的完整周期时间
     *
     * @note 呼吸灯初始化时会自动设置 PWM 频率为 1kHz，分辨率为 10‰
     */
    BreathingLED(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
                 bool driven_level = false, int period_s = 5) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, driven_level ? 0 : 1000 /* 初期关闭 */, 1000, 10),
          _driven_level(driven_level), _period_s(period_s) {};

    /**
     * @brief 执行呼吸灯效果
     *
     * 控制 LED 按照设定的周期执行呼吸效果，即亮度从暗逐渐变亮，
     * 再从亮逐渐变暗的过程。
     *
     * @param times 呼吸次数，默认为 1 次
     *
     * @note 该函数是阻塞式的，会持续执行完指定次数的呼吸效果
     * @note 呼吸周期由构造函数中设置的_period_s 决定
     */
    void breathe(int times = 1) noexcept;
};

class SteeringEngine : public PWM
{
private:
    const static int duty_min = 30;                          //!< 占空比最小值（左闭）
    const static int duty_max = 130;                         //!< 占空比最大值（右开）
    const static int delta_duty = duty_max - duty_min;       //!< 占空比上下界差值
    const static int degree_min = 0;                         //!< 百分度最小值（左闭）
    const static int degree_max = 200;                       //!< 百分度最大值（右开）
    const static int delta_degree = degree_max - degree_min; //!< 百分度上下界差值

    int16_t __degree_gon; //!< 角度（格恩 / 百分度 / 0.9 度）

    inline static int16_t truncate_degree_gon(int16_t degree_gon) noexcept
    {
        return std::abs(degree_gon) % degree_max + degree_min;
    }

public:
    inline SteeringEngine(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, duty_min, 50, 10), __degree_gon(0) {};

    inline void set_degree_gon(int16_t degree_gon) noexcept
    {
        this->__degree_gon = degree_gon;
        this->set_duty(duty_min + truncate_degree_gon(degree_gon) * delta_duty / delta_degree);
    }

    inline void set_degree(double degree) noexcept
    {
        set_degree_gon(int16_t(degree / 0.9));
    }

    inline void clockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon - int16_t(degree / 0.9));
    }

    inline void anticlockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon + int16_t(degree / 0.9));
    }
};

class DCMotorDriver
{
private:
    PWM PWMA;
    PWM PWMB;
    OutPin STBY;
    OutPin AIN1;
    OutPin BIN1;
    OutPin AIN2;
    OutPin BIN2;

public:
    enum class Mode
    {
        STOP,
        BRAKE,
        FORWARD,
        REVERSE
    };
    inline DCMotorDriver(
        PWM _PWMA, OutPin _AIN1, OutPin _AIN2,
        OutPin STBY,
        PWM _PWMB, OutPin _BIN1, OutPin _BIN2) noexcept
        : PWMA{_PWMA}, AIN1{_AIN1}, AIN2{_AIN2},
          STBY{STBY},
          PWMB{_PWMB}, BIN1{_BIN1}, BIN2{_BIN2} {};

    inline DCMotorDriver(PWM _PWMA, OutPin _AIN1, OutPin _AIN2, OutPin STBY = null_pin /* 直接接到了 3V */) noexcept
        : DCMotorDriver(_PWMA, _AIN1, _AIN2, STBY,
                        PWM{nullptr, 0, 0}, OutPin{null_pin}, OutPin{null_pin}) {};

    inline void standby() noexcept
    {
        STBY.reset();
    }
    inline void work() noexcept
    {
        STBY.set();
    }
    void set_mode(Mode mode, uint8_t motor_index = 0) noexcept;
    void set_speed(uint16_t speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept;
    inline void add_speed(int16_t delta_speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept
    {
        set_speed(
            (motor_index == 0 ? PWMA.get_duty() : PWMB.get_duty()) + delta_speed_permillage,
            motor_index);
    }
    inline int get_speed(uint8_t motor_index = 0) noexcept
    {
        return (motor_index == 0 ? PWMA.get_duty() : PWMB.get_duty());
    }
};

STM32_END

#endif // EMBMARTIN_TOOLS_H