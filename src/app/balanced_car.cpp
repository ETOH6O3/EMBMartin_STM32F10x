#include "EMBMartin/app/balanced_car.h"

EMBMARTIN_BALANCED_CAR_NAMESPACE_BEGIN

BalancedCar::BalancedCar(
    device::DCMotorDriver &_driver,
    device::RotaryEncoder &_encoder_left,
    device::RotaryEncoder &_encoder_right,
    device::MPU6050 &_MPU,
    algorithm::PIDParams vertical_params,
    algorithm::PIDParams velocity_params,
    algorithm::PIDParams turn_params,
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

void BalancedCar::update(double turn_trg, double velocity_trg) noexcept
{
    // 读取 MPU6050 数据
    this->mpu_data = this->MPU.get_data();
    // 读取编码器数据
    this->v_left = this->encoder_left.get();
    this->v_right = this->encoder_right.get();

    // 计算角度
    this->angle_filter.update();
    this->angle = this->angle_filter.get_status();
    // 计算 PID
    auto output = this->controller.compute(turn_trg, velocity_trg);
    // 设置电机占空比
    this->driver.set_speed(output.duty_left_permillage * this->left_speed_scaling, 0);
    this->driver.set_speed(output.duty_right_permillage, 1);
}

EMBMARTIN_BALANCED_CAR_NAMESPACE_END