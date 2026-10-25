/**
 ******************************************************************************
 * @file    console.h
 * @author  孙鸣淼
 * @brief   串口终端：USB 串口与蓝牙串口
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 *
 * 本文件由 hal/peripheral/USART.h 拆出。
 * 依赖方向：本文件 -> hal/peripheral/USART.h / lang/stream.h / device/debugger.h（单向）。
 ******************************************************************************
 */

#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/peripheral/USART.h"
#include "EMBMartin/lang/stream.h"
#include "EMBMartin/device/debugger.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief USB 串口终端
 *
 * 把 USART 硬件与一个流式调试器（BaseStream）组合起来：
 * 输出走 BaseStream 的格式化缓冲，字符最终由 output_char() 交给 USART 硬件；
 * 输入由 read() 从 USART 收进 InStream 的缓冲区。
 *
 * @tparam BaseStream 提供 output_char / 格式化接口的流类型，默认 MartinDebugger<128>
 */
template <typename BaseStream = MartinDebugger<128>>
class USBConsole : public BaseStream, public EMBMartin::STM32::USART
{
public:
    inline USBConsole(USART_TypeDef *USARTx, int remap = 0b00, int baud_rate = 9600) noexcept
        : EMBMartin::STM32::USART(USARTx, remap, baud_rate)
    {
    }
    /**
     * @brief 输出一个字符到串口
     * 缓冲区由 OutStream 管理，这里只需要把字符交给硬件
     */
    void output_char(char c) noexcept override
    {
        this->send_byte(static_cast<uint8_t>(c));
    }
    void read() noexcept override;
};

template <class BaseStream>
void USBConsole<BaseStream>::read() noexcept
{
    char *in_p = this->EMBMartin::InStream<128>::buffer;
    // while ((*(in_p++) = this->read_halfword()) && (*in_p != '\n') && (*in_p != '\r'))
    // ;
    while (true)
    {
        char ch = this->read_halfword();
        if (ch == '\0' || ch == '\n' || ch == '\r' || (in_p - this->EMBMartin::InStream<128>::buffer) >= 127)
            break;
        *(in_p++) = ch;
    }
    USART_ClearITPendingBit(this->_USARTx, USART_IT_RXNE);
    *in_p = '\0';
}

/**
 * @brief 蓝牙串口终端
 *
 * 与 USBConsole 完全同构，只是默认流类型换成江协科技的绘图调试器，
 * 以便直接用其上位机观察波形。
 */
template <typename BaseStream = JiangXieDebugger<>>
using BluetoothConsole = USBConsole<BaseStream>;

EMBMARTIN_STM32F10X_NAMESPACE_END
