#ifndef EMBMARTIN_USART_H
#define EMBMARTIN_USART_H


#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "meta.h"
#include "basic_tools.h"
#include "system.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class USART
{
private:
    USART_TypeDef* _USARTx;
public:
    USART(USART_TypeDef * USARTx, int remap, int baud_rate )noexcept;
    void send_byte(uint8_t data)noexcept
    {
        USART_SendData(this->_USARTx, data);
        while(USART_GetFlagStatus(this->_USARTx,USART_FLAG_TXE) == RESET);
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END

#endif  // EMBMARTIN_USART_H