#include "PID.h"

#include <algorithm>
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

double SingleLoopPID::compute(double target) noexcept
{
    double error = this->current - target;

    // 误差滤波
    error = (1 - this->alpha) * error + this->alpha * this->previous_error;

    // 积分项计算
    this->integral += error * this->dt;

    // 积分限幅
    if (integral_limit > 0) // 小于等于 0 表示不启用限幅
    {
        if (this->integral > this->integral_limit)
            this->integral = this->integral_limit;
        else if (this->integral < -this->integral_limit)
            this->integral = -this->integral_limit;
    }

    // 微分项计算
    double derivative;
    if (this->d_current == nullptr)
        derivative = (error - this->previous_error) / this->dt;
    else
        derivative = *(this->d_current);

    // PID输出计算
    double output = this->kp * error + this->ki * this->integral + this->kd * derivative;

    // 保存当前误差以供下次计算微分项
    this->previous_error = error;

    // 输出限幅
    if (output_limit > 0) // 小于等于 0 表示不启用限幅
    {
        if (output > this->output_limit)
            output = this->output_limit;
        else if (output < -this->output_limit)
            output = -this->output_limit;
    }

    return output;
}

BalancedCarPID::Output BalancedCarPID::compute(double turn_trg, double velocity_trg) noexcept
{
    this->angle_filter.update();
    this->angle_status = this->angle_filter.get_status();
    this->angle_status.pitch -= this->pitch_med_angle;

    this->turn_pid_current = (this->pace_left - this->pace_right) / 2.0;
    this->velocity_pid_current = (this->pace_left + this->pace_right) / 2.0;

    auto velocity_out = this->velocity.compute(velocity_trg);
    auto vertical_out = this->vertical.compute(velocity_out);
    auto turn_out = this->turn.compute(turn_trg);

    return Output{
        .duty_left_permillage = vertical_out - turn_out,
        .duty_right_permillage = vertical_out + turn_out,
    };
}

EMBMARTIN_STM32F10X_NAMESPACE_END
