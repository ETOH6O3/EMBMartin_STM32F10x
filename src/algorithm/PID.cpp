#include "EMBMartin/algorithm/PID.h"

#include <cmath>

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

SingleLoopPID::SingleLoopPID(
    double _kp, double _ki, double _kd, double &_current, double _dt,
    double _integral_limit, double _output_limit, double _alpha, const int16_t *_d_current) noexcept
    : kp(_kp), ki(_ki), kd(_kd),
      integral_limit(_integral_limit), output_limit(_output_limit), alpha(std::clamp(_alpha, 0.0, 1.0)),
      current(_current), dt(_dt),
      d_current(_d_current) {};

void SingleLoopPID::set(
    double _kp, double _ki, double _kd, double _dt,
    double _integral_limit, double _output_limit,
    double _alpha, const int16_t *_d_current) noexcept
{
    set_gains(_kp, _ki, _kd);
    set_dt(_dt);
    set_limits(_integral_limit, _output_limit);
    set_alpha(_alpha);
    set_d_current(_d_current);
}

BalancedCarPID::BalancedCarPID(MPU6050::Data &_MPU_data,
                               int16_t &_pace_left, int16_t &_pace_right,
                               PIDParams vertical_params, PIDParams velocity_params, PIDParams turn_params,
                               double dt, double _pitch_med_angle) noexcept
    : MPU_data(_MPU_data),
      pace_left(_pace_left), pace_right(_pace_right),
      angle_filter(_MPU_data, 0.001, dt),
      vertical(vertical_params.kp, vertical_params.ki, vertical_params.kd,
               this->angle_status.pitch, dt,
               vertical_params.integral_limit, vertical_params.output_limit, vertical_params.alpha,
               &this->MPU_data.gyro_y),
      velocity(velocity_params.kp, velocity_params.ki, velocity_params.kd,
               this->velocity_pid_current, dt,
               velocity_params.integral_limit, velocity_params.output_limit, velocity_params.alpha),
      turn(turn_params.kp, turn_params.ki, turn_params.kd,
           this->turn_pid_current, dt,
           turn_params.integral_limit, turn_params.output_limit, turn_params.alpha),
      pitch_med_angle(_pitch_med_angle) {};

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
