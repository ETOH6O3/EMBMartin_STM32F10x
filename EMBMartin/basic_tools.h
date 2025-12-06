/**
 ******************************************************************************
 * @file    tools.h
 * @author  孙鸣淼
 * @brief   这里存放一些嵌入式开发中非常底层的工具 （基于 STM32 标准外设库）
 ******************************************************************************
 * @attention
 * 0. 需要预定义宏 STM32_DEVICE_HEADER 为对应的 STM32 设备头文件名，如 stm32f10x.h （不得含引号）
 *    或者在 macro.h 中定义 STM32_DEVICE_HEADER 宏，这也是等价的
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#ifndef EMBMARTIN_BASIC_TOOLS_H
#define EMBMARTIN_BASIC_TOOLS_H

#include <stddef.h>
#include <numeric>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief GPIO 引脚封装结构体
 *
 * 该结构体用于封装一个 GPIO 引脚的信息，包括端口和引脚号，
 * 方便在嵌入式系统中进行 GPIO 操作。
 *
 * @note 所有操作的安全合法性完全由用户确保
 */
struct GPIOPin
{
    GPIO_TypeDef *port; //!< GPIO 端口指针
    uint16_t pin;       //!< GPIO 引脚号

    inline GPIOPin(GPIO_TypeDef *_port, uint16_t _pin) noexcept : port{_port}, pin(_pin) {};
    volatile inline void set() noexcept // 读取模式可用
    {
        GPIO_WriteBit(this->port, this->pin, Bit_SET);
    }

    volatile inline void reset() noexcept // 读取模式可用
    {
        GPIO_WriteBit(this->port, this->pin, Bit_RESET);
    }

    volatile inline bool read() noexcept // 读取模式和开漏输出模式可用
    {
        return GPIO_ReadInputDataBit(this->port, this->pin);
    }

    volatile inline bool read_output() noexcept // 输出模式可用
    {
        return GPIO_ReadOutputDataBit(this->port, this->pin);
    }
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
inline uint32_t Get_RCC_APB1Periph(USART_TypeDef *usartx) noexcept
{
    if (usartx == USART2)
        return RCC_APB1Periph_USART2;
    else if (usartx == USART3)
        return RCC_APB1Periph_USART3;
    else
        return 0;
}

uint32_t Get_EXTI_Line(uint16_t pin) noexcept; // TODO: 使能用 HAL 库重写
IRQn Get_IRQChannel(uint16_t pin) noexcept;
uint32_t get_TIM_RCC_APB1Periph(TIM_TypeDef *__TIMX) noexcept;
auto Get_TIM_Index(TIM_TypeDef *TIMX) noexcept -> size_t;
inline auto Get_USART_Index(USART_TypeDef *USARTx) noexcept
{
    if (USARTx == USART1)
        return 0;
    else if (USARTx == USART2)
        return 1;
    else if (USARTx == USART3)
        return 2;
    else
        return -1;
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
           "All pins must be on the same port");

    return {pins[0].port, std::accumulate(
                              pins.begin(),
                              pins.end(),
                              (uint16_t)0,
                              [](uint16_t last_rslt, GPIOPin pin2) -> uint16_t
                              { return last_rslt | pin2.pin; })};
}

struct USARTPins
{
    GPIOPin tx;
    GPIOPin rx;
    GPIOPin ck = null_pin;
    GPIOPin cts = null_pin;
    GPIOPin rts = null_pin;
};

const USARTPins USARTX_REMAP[3][4] =
    {
        // USART1
        {
            // 未复用
            {PA9, PA10},
            // 完全重映像
            {PB6, PB7},
            // 保留
            {PA9, PA10},
            // 保留
            {PB6, PB7},
        },
        // USART2
        {
            // 未复用
            {PA2, PA3, PA4, PA0, PA1},
            // 部分重映像
            {PD5, PD6, PA7, PA3, PA4},
            // 完全重映像
            {PA2, PA3, PA4, PA0, PA1},
            // 保留
            {PD5, PD6, PA7, PA3, PA4},
        },
        // USART3
        {
            // 未复用
            {PB10, PB11, PB12, PB13, PB14},
            // 部分重映像
            {PC10, PC11, PC12, PB13, PB14},
            // 完全重映像
            {PD8, PD9, PD10, PD11, PD12},
            // 保留
            {PD8, PD9, PD10, PD11, PD12},
        },

};

EMBMARTIN_STM32F10X_NAMESPACE_END

#endif // EMBMARTIN_BASIC_TOOLS_H