#include "mpu6050.h"

#include <tuple>

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

MPU6050::MPU6050(
    GPIOPin SCL, GPIOPin SDA, bool AD0,
    bool gyroscope_enable, uint8_t SMPRT_DIV,
    uint8_t ACCEL_AFS_SCL,
    uint8_t GYRO_FS_SEL) noexcept
    : I2C{SCL, SDA, uint8_t(0xD0 + (AD0 << 1))}
{
    write_reg(REGS::PWR_MGMT_1, gyroscope_enable);     // 解除休眠，是否启用陀螺仪作为时钟源
    write_reg(REGS::PWR_MGMT_2, 0x00);                 // 六轴都工作
    write_reg(REGS::SMPLRT_DIV, SMPRT_DIV - 1);        // 设置采样率
    write_reg(REGS::CONFIG, 0x00);                     // 平滑地滤波
    write_reg(REGS::ACCEL_CONFIG, ACCEL_AFS_SCL << 3); // 4g 满量程
    write_reg(REGS::GYRO_CONFIG, GYRO_FS_SEL << 3);    // 1000°/s 满量程
};

MPU6050::Data MPU6050::get_data() noexcept
{
    Data merge_rslt;

    auto data_acc = read_reg<6>(REGS::ACCEL_XOUT_H);
    auto data_gyro = read_reg<6>(REGS::GYRO_XOUT_H);

    merge_rslt.acc_x = (uint16_t(std::get<0>(data_acc)) << 8) | std::get<1>(data_acc);
    merge_rslt.acc_y = (uint16_t(std::get<2>(data_acc)) << 8) | std::get<3>(data_acc);
    merge_rslt.acc_z = (uint16_t(std::get<4>(data_acc)) << 8) | std::get<5>(data_acc);

    merge_rslt.gyro_x = (uint16_t(std::get<0>(data_gyro)) << 8) | std::get<1>(data_gyro);
    merge_rslt.gyro_y = (uint16_t(std::get<2>(data_gyro)) << 8) | std::get<3>(data_gyro);
    merge_rslt.gyro_z = (uint16_t(std::get<4>(data_gyro)) << 8) | std::get<5>(data_gyro);

    return merge_rslt;
}

EMBMARTIN_STM32F10X_NAMESPACE_END
