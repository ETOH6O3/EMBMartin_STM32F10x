#ifndef EMBMARTIN_USART_H
#define EMBMARTIN_USART_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "meta.h"
#include "basic_tools.h"
#include "system.h"
#include "IO.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class USART
{
private:
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

class USBConsole : public EMBMartin::IOStream<128>, public EMBMartin::STM32::USART
{
public:
    inline USBConsole(USART_TypeDef *USARTx, int remap = 0b00, int baud_rate = 9600) noexcept
        : EMBMartin::STM32::USART(USARTx, remap, baud_rate)
    {
    }

    void send() noexcept override
    {
        char *out_p = this->EMBMartin::OutStream<128>::buffer;
        do
        {
            this->send_byte(static_cast<uint8_t>(*(out_p++)));
        } while (*out_p);
    }
    void read() noexcept override;
    
};

EMBMARTIN_STM32F10X_NAMESPACE_END

#endif // EMBMARTIN_USART_H