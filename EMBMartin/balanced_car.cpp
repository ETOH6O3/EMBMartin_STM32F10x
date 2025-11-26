#include "balanced_car.h"

void EMBMartin::STM32::BalancedCar::update(double turn_trg, double velocity_trg) noexcept
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