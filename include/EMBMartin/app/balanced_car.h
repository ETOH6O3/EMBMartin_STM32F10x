#pragma once

#include "EMBMartin/device/dc_motor_driver.h"
#include "EMBMartin/device/rotary_encoder.h"
#include "EMBMartin/device/mpu6050.h"
#include "EMBMartin/algorithm/angle_filter.h"
#include "EMBMartin/algorithm/PID.h"

EMBMARTIN_BALANCED_CAR_NAMESPACE_BEGIN

class BalancedCar
{
EMBMARTIN_DEBUGING_SPECIFIER:
    const double EXTI_TIME_S;
    const double left_speed_scaling;

    device::DCMotorDriver &driver;
    device::RotaryEncoder &encoder_left;
    device::RotaryEncoder &encoder_right;
    int16_t v_left;
    int16_t v_right;
    device::MPU6050 &MPU;
    device::MPU6050::Data mpu_data;
    algorithm::AngleComplementaryFilter angle_filter;
    algorithm::AngleComplementaryFilter::Status angle;
    algorithm::BalancedCarPID controller;

public:
    BalancedCar(
        device::DCMotorDriver &_driver,
        device::RotaryEncoder &_encoder_left,
        device::RotaryEncoder &_encoder_right,
        device::MPU6050 &_MPU,
        algorithm::PIDParams vertical_params,
        algorithm::PIDParams velocity_params,
        algorithm::PIDParams turn_params,
        double pitch_med_angle,
        double _left_speed_scaling,
        double dt) noexcept;

    auto get_MPU_data() noexcept { return mpu_data; }
    auto get_angle_status() noexcept { return angle; }
    auto get_v_left() noexcept { return v_left; }
    auto get_v_right() noexcept { return v_right; }

    void update(double turn_trg, double velocity_trg) noexcept;
};

EMBMARTIN_BALANCED_CAR_NAMESPACE_END
