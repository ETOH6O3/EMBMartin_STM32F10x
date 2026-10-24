#pragma once

#include "EMBMartin/macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "EMBMartin/hal/gpio/basic_tools.h"


extern "C" void USART1_IRQHandler(void);
extern "C" void USART2_IRQHandler(void);
extern "C" void USART3_IRQHandler(void);

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


EMBMARTIN_STM32F10X_NAMESPACE_END
