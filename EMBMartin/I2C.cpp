#include "I2C.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

void I2C::start() noexcept
{
    /***************************************************************************************************
    SCL
    XXXXXXXXXXXXXXXXXXXXXX
                         XX
                          X
                          XX
                           X
                           XX
                            X
    SDA                     XXXXXXXXXXXXXXXX
    XXXXXXXXXXXXXXXXXX
                     X
                     XX
                      X
                      X
                      XX
                       X
                       XXXXXXXXXXXXXXXXXXXX

    ****************************************************************************************************/
    __SDA.set();
    i2c_delay(); // 防止与终止信号混淆，同时保证 SDA 建立时间
    __SCL.set();

    i2c_delay(); // SCL 高电平保持（起始信号的建立时间）
    __SDA.reset();
    i2c_delay(); // 起始信号的数据保持时间
    __SCL.reset();
    i2c_delay(); // SCL 低电平保持
}

void I2C::stop() noexcept
{
    /***************************************************************************************************
                      XXXXXXXXXXXXXXXX
                     XX
                    XX
                    XX
     SCL           XX
    XXXXXXXXXXXXXXXXX


                              XXXXXXXX
                            XXX
                           XXX
                          XX
    SDA                  XX
    XXXXXXXXXXXXXXXXXXXXXXX

    ****************************************************************************************************/
    // 契约：确保 SCL 必定已经是低电平
    __SDA.reset();
    i2c_delay(); // SDA 建立时间

    __SCL.set();
    i2c_delay(); // SCL 高电平保持（停止信号的建立时间）
    __SDA.set();
    i2c_delay(); // 停止信号的数据保持时间 + 总线空闲时间
}

void I2C::send_ack(bool data) noexcept
{
    data ? __SDA.set() : __SDA.reset();
    i2c_delay(); // SDA 建立时间
    __SCL.set();
    i2c_delay(); // SCL 高电平保持
    __SCL.reset();
    i2c_delay(); // SCL 低电平保持
}

void I2C::receive_ack() noexcept
{
    bool rslt;

    __SDA.set(); // 释放 SDA
    i2c_delay();

    __SCL.set();
    i2c_delay(); // ★ 关键：必须等从机把 SDA 拉低后再采样，否则 -O1 下必然误判 NACK
    rslt = __SDA.read();
    __SCL.reset();
    i2c_delay(); // SCL 低电平保持

    EMBMARTIN_ASSERT(!rslt, "I2C req exception", &console);
}

I2C::I2C(GPIOPin SCL, GPIOPin SDA, uint8_t addr) noexcept
    : __SCL(SCL), __SDA(SDA), _addr(addr & 0b11111110)
{
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SCL.port), ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SDA.port), ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {
        .GPIO_Pin = SCL.pin,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_Out_OD,
    };
    GPIO_Init(SCL.port, &GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = SDA.pin;
    GPIO_Init(SDA.port, &GPIO_InitStruct);

    GPIO_SetBits(SCL.port, SCL.pin);
    GPIO_SetBits(SDA.port, SDA.pin);
}

void I2C::send_byte(uint8_t data) noexcept
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        // 契约：确保 SCL 必定已经是低电平
        (data & (0x80 >> i)) ? __SDA.set() : __SDA.reset();
        i2c_delay(); // SDA 建立时间 + SCL 低电平保持
        __SCL.set();
        i2c_delay(); // SCL 高电平保持（从机在这段时间内采样数据位）
        __SCL.reset();
        i2c_delay(); // SCL 低电平保持
    }
}

uint8_t I2C::receive_byte() noexcept
{
    uint8_t rslt{0x00};
    // 契约：确保 SCL 必定已经是低电平
    __SDA.set(); // 释放 SDA
    i2c_delay();

    for (uint8_t i = 0; i < 8; ++i)
    {
        __SCL.set();
        i2c_delay(); // 必须：等从机把数据位放到 SDA 上再采样
        rslt <<= 1;
        rslt += __SDA.read();
        __SCL.reset();
        i2c_delay(); // SCL 低电平保持
    }

    return rslt;
}

uint8_t I2C::read_reg(uint8_t reg_addr) noexcept
{
    // 指定地址
    start();
    send_byte(_addr);
    receive_ack();
    send_byte(reg_addr);
    receive_ack();

    // 进入读模式
    start();
    send_byte(_addr | 0x01);
    receive_ack();

    // 劫收
    uint8_t rslt = receive_byte();
    send_ack(1);

    stop();
    return rslt;
}

EMBMARTIN_STM32F10X_NAMESPACE_END
