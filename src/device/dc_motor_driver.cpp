#include "EMBMartin/device/dc_motor_driver.h"

#include <cmath>

EMBMARTIN_DEVICE_NAMESPACE_BEGIN

DCMotorDriver::DCMotorDriver(
    hal::PWM _PWMA, hal::OutPin _AIN1, hal::OutPin _AIN2,
    hal::OutPin STBY,
    hal::PWM _PWMB, hal::OutPin _BIN1, hal::OutPin _BIN2) noexcept
    : PWMA{_PWMA}, AIN1{_AIN1}, AIN2{_AIN2},
      STBY{STBY},
      PWMB{_PWMB}, BIN1{_BIN1}, BIN2{_BIN2}
{
    work();
    set_mode(Mode::FORWARD, 0);
    set_mode(Mode::FORWARD, 1);
}

DCMotorDriver::DCMotorDriver(hal::PWM _PWMA, hal::OutPin _AIN1, hal::OutPin _AIN2, hal::OutPin STBY) noexcept
    : DCMotorDriver(_PWMA, _AIN1, _AIN2, STBY,
                    hal::PWM{nullptr, 0, 0}, hal::OutPin{hal::null_pin}, hal::OutPin{hal::null_pin}) {}

void DCMotorDriver::set_mode(Mode mode, uint8_t motor_index) noexcept
{
    hal::OutPin *IN1;
    hal::OutPin *IN2;
    hal::PWM *PWm;
    if (motor_index == 0)
    {
        IN1 = &AIN1;
        IN2 = &AIN2;
        PWm = &PWMA;
    }
    else
    {
        IN1 = &BIN1;
        IN2 = &BIN2;
        PWm = &PWMB;
    }

    switch (mode)
    {
    case Mode::STOP:
        IN1->reset();
        IN2->reset();
        PWm->set_duty(0);
        break;
    case Mode::BRAKE:
        PWm->set_duty(0);
        break;
    case Mode::FORWARD:
        IN1->set();
        IN2->reset();
        break;
    case Mode::REVERSE:
        IN1->reset();
        IN2->set();
        break;
    default:
        break;
    }
}

void DCMotorDriver::set_speed(int16_t speed_permillage, uint8_t motor_index) noexcept
{
    speed_permillage = std::clamp<int16_t>(speed_permillage, -990, 990);
    hal::PWM *PWm;
    if (motor_index == 0)
    {
        current_left_speed_permillage = speed_permillage;
        PWm = &PWMA;
    }
    else
    {
        current_right_speed_permillage = speed_permillage;
        PWm = &PWMB;
    }

    if (speed_permillage < 0)
        set_mode(Mode::REVERSE, motor_index);
    else
        set_mode(Mode::FORWARD, motor_index);

    PWm->set_duty(std::abs(speed_permillage));
}

EMBMARTIN_DEVICE_NAMESPACE_END
