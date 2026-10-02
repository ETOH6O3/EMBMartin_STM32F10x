#ifndef EMBMARTIN_USART_H
#define EMBMARTIN_USART_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "meta.h"
#include "basic_tools.h"
#include "system.h"
#include "stream.h"
#include "debugger.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class USART
{
protected:
    struct ReadStatus
    {
        bool data_ready;
        uint16_t data;
    };

    static inline ReadStatus data_status[3] = {
        {false, 0},
        {false, 0},
        {false, 0},
    };
    USART_TypeDef *_USARTx;

public:
    USART(USART_TypeDef *USARTx, int remap, int baud_rate) noexcept;
    void send_byte(uint8_t data) noexcept
    {
        USART_SendData(this->_USARTx, data);
        while (USART_GetFlagStatus(this->_USARTx, USART_FLAG_TXE) == RESET)
            EMBMARTIN_KEEP_CODE_ORDER;
    }
    uint16_t read_halfword() noexcept;

    friend void ::USART1_IRQHandler(void);
    friend void ::USART2_IRQHandler(void);
    friend void ::USART3_IRQHandler(void);
};

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

template<class BaseStream> 
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

template <typename BaseStream = JiangXieDebugger<>>
using BluetoothConsole = USBConsole<BaseStream>;

EMBMARTIN_STM32F10X_NAMESPACE_END

#endif // EMBMARTIN_USART_H