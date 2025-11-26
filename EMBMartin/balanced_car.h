#ifndef EMBMARTIN_BALANCED_CAR_H
#define EMBMARTIN_BALANCED_CAR_H

#include "tools.h"

EMBMARTIN_BALANCED_CAR_NAMESPACE_BEGIN

class BalancedCar
{
private:
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
    inline BalancedCar(
        DCMotorDriver &_driver,
        RotaryEncoder &_encoder_left,
        RotaryEncoder &_encoder_right,
        MPU6050 &_MPU,
        PIDParams vertical_params,
        PIDParams velocity_params,
        PIDParams turn_params,
        double pitch_med_angle,
        double _left_speed_scaling,
        double dt) noexcept
        : EXTI_TIME_S(dt),
          left_speed_scaling(_left_speed_scaling),
          driver(_driver),
          encoder_left(_encoder_left),
          encoder_right(_encoder_right),
          v_left(0),
          v_right(0),
          MPU(_MPU),
          mpu_data(MPU.get_data()),
          angle_filter(mpu_data, 0.001, dt),
          controller(
              mpu_data,
              v_left,
              v_right,
              vertical_params,
              velocity_params,
              turn_params,
              dt,
              pitch_med_angle) {};

    auto get_MPU_data() noexcept { return mpu_data; }
    auto get_angle_status() noexcept { return angle; }
    auto get_v_left() noexcept { return v_left; }
    auto get_v_right() noexcept { return v_right; }

    void update(double turn_trg, double velocity_trg) noexcept;
};

EMBMARTIN_BALANCED_CAR_NAMESPACE_END

#endif // EMBMARTIN_BALANCED_CAR_H