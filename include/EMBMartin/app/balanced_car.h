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

    DCMotorDriver &driver;
    RotaryEncoder &encoder_left;
    RotaryEncoder &encoder_right;
    int16_t v_left;
    int16_t v_right;
    MPU6050 &MPU;
    MPU6050::Data mpu_data;
    AngleComplementaryFilter angle_filter;
    AngleComplementaryFilter::Status angle;
    BalancedCarPID controller;

public:
    BalancedCar(
        DCMotorDriver &_driver,
        RotaryEncoder &_encoder_left,
        RotaryEncoder &_encoder_right,
        MPU6050 &_MPU,
        PIDParams vertical_params,
        PIDParams velocity_params,
        PIDParams turn_params,
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

