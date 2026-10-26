/**
 ******************************************************************************
 * @file    dc_motor_driver.h
 * @author  孙鸣淼
 * @brief   直流电机驱动（L298N 类双路 H 桥）
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 *
 * 本文件由 hal/peripheral/PWM.h 拆出。
 * 依赖方向：本文件 -> hal/peripheral/PWM.h / hal/gpio/outpin.h（单向）。
 ******************************************************************************
 */

#pragma once

#include "EMBMartin/macro.h"
#include "EMBMartin/hal/peripheral/PWM.h"
#include "EMBMartin/hal/gpio/outpin.h"

EMBMARTIN_DEVICE_NAMESPACE_BEGIN

/**
 * @brief 直流电机驱动类
 *
 * 面向 L298N 一类双路 H 桥模块：每路需要 1 路 PWM（调速）与 2 路方向引脚，
 * 整模块另有 STBY（使能）引脚。两路可以独立控制，也可以只用单路。
 */
class DCMotorDriver
{
EMBMARTIN_DEBUGING_SPECIFIER:
    hal::PWM PWMA;
    hal::PWM PWMB;
    hal::OutPin STBY;
    hal::OutPin AIN1;
    hal::OutPin BIN1;
    hal::OutPin AIN2;
    hal::OutPin BIN2;

    int16_t current_left_speed_permillage = 0;
    int16_t current_right_speed_permillage = 0;

public:
    enum class Mode
    {
        STOP,
        BRAKE,
        FORWARD,
        REVERSE
    };
    DCMotorDriver(
        hal::PWM _PWMA, hal::OutPin _AIN1, hal::OutPin _AIN2,
        hal::OutPin STBY,
        hal::PWM _PWMB, hal::OutPin _BIN1, hal::OutPin _BIN2) noexcept;

    DCMotorDriver(hal::PWM _PWMA, hal::OutPin _AIN1, hal::OutPin _AIN2, hal::OutPin STBY = hal::null_pin /* 直接接到了 3V */) noexcept;

    inline void standby() noexcept
    {
        STBY.reset();
    }
    inline void work() noexcept
    {
        STBY.set();
    }
    void set_mode(Mode mode, uint8_t motor_index = 0) noexcept;
    void set_speed(int16_t speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept;
    inline void add_speed(int16_t delta_speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept
    {
        set_speed(
            get_speed(motor_index) + delta_speed_permillage,
            motor_index);
    }
    inline int16_t get_speed(uint8_t motor_index = 0) noexcept
    {
        return motor_index == 0 ? current_left_speed_permillage : current_right_speed_permillage;
    }
};

EMBMARTIN_DEVICE_NAMESPACE_END
