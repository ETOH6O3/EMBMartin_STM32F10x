#include "USART.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

USART::USART(USART_TypeDef *USARTx, int remap, int baud_rate) noexcept
    : _USARTx(USARTx)
{
    auto pin_tx = USARTX_REMAP[Get_USART_Index(USARTx)][remap].tx;
    auto pin_rx = USARTX_REMAP[Get_USART_Index(USARTx)][remap].rx;

    // 开启 GPIO 端口时钟
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin_tx.port), ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(pin_rx.port), ENABLE);

    // 配置引脚
    GPIO_InitTypeDef GPIO_InitStructure;
    // TX
    GPIO_InitStructure.GPIO_Pin = pin_tx.pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(pin_tx.port, &GPIO_InitStructure);
    // RX
    GPIO_InitStructure.GPIO_Pin = pin_rx.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(pin_rx.port, &GPIO_InitStructure);

    // 开启 USART 时钟
    if (USARTx == USART1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    else
        RCC_APB1PeriphClockCmd(Get_RCC_APB1Periph(USARTx), ENABLE);

    // 配置 USART
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud_rate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USARTx, &USART_InitStructure);

    // 配置中断
    USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = Get_USART_IRQChannel(USARTx);
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USARTx, ENABLE);
}

uint16_t USART::read_halfword() noexcept
{
    auto index = Get_USART_Index(this->_USARTx);
    while (!(data_status[index].data_ready))
        EMBMARTIN_KEEP_CODE_ORDER;
    data_status[index].data_ready = false;
    return data_status[index].data;
}

EMBMARTIN_STM32F10X_NAMESPACE_END

extern "C"
{
    void USART1_IRQHandler(void)
    {
        if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
        {
            EMBMartin::STM32::USART::data_status[0] = {.data_ready = true, .data = USART_ReceiveData(USART1)};
            // USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        }
    }

    void USART2_IRQHandler(void)
    {
        if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
        {
            EMBMartin::STM32::USART::data_status[1] = {.data_ready = true, .data = USART_ReceiveData(USART2)};
            // USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        }
    }

    void USART3_IRQHandler(void)
    {
        if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
        {
            EMBMartin::STM32::USART::data_status[2] = {.data_ready = true, .data = USART_ReceiveData(USART3)};
            // USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        }
    }
}
