#include "I2C.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

void I2C::send_byte(uint8_t data) noexcept
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        // 契约：确保 SCL 必定已经是低电平
        (data & (0x80 >> i)) ? _SDA.set() : _SDA.reset();
        i2c_delay(); // SDA 建立时间 + SCL 低电平保持
        _SCL.set();
        i2c_delay(); // SCL 高电平保持（从机在这段时间内采样数据位）
        _SCL.reset();
        i2c_delay(); // SCL 低电平保持
    }
}

uint8_t I2C::receive_byte() noexcept
{
    uint8_t rslt{0x00};
    // 契约：确保 SCL 必定已经是低电平
    _SDA.set(); // 释放 SDA
    i2c_delay();

    for (uint8_t i = 0; i < 8; ++i)
    {
        _SCL.set();
        i2c_delay(); // 必须：等从机把数据位放到 SDA 上再采样
        rslt <<= 1;
        rslt += _SDA.read();
        _SCL.reset();
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
