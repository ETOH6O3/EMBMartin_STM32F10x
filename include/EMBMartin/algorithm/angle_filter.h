#pragma once

///\file
/// 角度互补滤波器（MPU6050 姿态解算）。
///
/// 自 PID.h 拆出。依赖方向：本文件 -> mpu6050.h（单向）。
/// PID.h 中的 BalancedCarPID 以成员方式持有本类，故 PID.h 包含本文件。

#include "EMBMartin/device/mpu6050.h"

EMBMARTIN_ALGORITHM_NAMESPACE_BEGIN

class AngleComplementaryFilter
{
private:
    const double alpha;            //!< 互补滤波系数
    const double dt;               //!< 采样时间间隔（秒）
    const device::MPU6050::Data &MPU_data; //!< MPU6050 数据引用

    double current_pitch = 0.0; //!< 当前俯仰角（度）
    double current_roll = 0.0;  //!< 当前横滚角（度）
    double current_yaw = 0.0;   //!< 当前偏航角（度）
public:
    struct Status
    {
        double pitch; //!< 俯仰角（度）
        double roll;  //!< 横滚角（度）
        double yaw;   //!< 偏航角（度）
    };
    AngleComplementaryFilter(
        device::MPU6050::Data &_MPU_data,
        double _alpha = 0.001, double _dt = 0.001) noexcept
        : alpha(_alpha), dt(_dt), MPU_data(_MPU_data) {};

    void update() noexcept; // 在中断函数或循环中定期调用以更新角度数据
    inline Status get_status() const noexcept { return {current_pitch, current_roll, current_yaw}; }
};

EMBMARTIN_ALGORITHM_NAMESPACE_END
