#include "EMBMartin/algorithm/angle_filter.h"

#include <cmath>

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

void AngleComplementaryFilter::update() noexcept
{
    double angle_acc_pitch = std::atan2(double(MPU_data.acc_x), MPU_data.acc_z) * 180 / 3.14159265358979;
    double angle_acc_roll = std::atan2(double(MPU_data.acc_y), MPU_data.acc_z) * 180 / 3.14159265358979;

    current_pitch = alpha * angle_acc_pitch + (1 - alpha) * (current_pitch - MPU_data.gyro_y * 1000 / 32768.0 * dt);
    current_roll = alpha * angle_acc_roll + (1 - alpha) * (current_roll + MPU_data.gyro_x * 1000 / 32768.0 * dt);
    current_yaw = current_yaw + MPU_data.gyro_z * 1000 / 32768.0 * dt; // 偏航角无法互补滤波
}

EMBMARTIN_STM32F10X_NAMESPACE_END
