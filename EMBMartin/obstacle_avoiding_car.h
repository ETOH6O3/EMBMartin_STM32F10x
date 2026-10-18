#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "functional.h"
#include "PWM.h"
#include "EXTI.h"
#include "TIM.h"
#include "ultrasonic_sensor.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class ObstacleAvoidingCar
{
public:
    enum class State : uint8_t
    {
        COMMON, // 非避障
        SCAN,   // 扫描
        AVOID   // 避障
    };

protected:
    DCMotorDriver &motor_driver;
    EXTIRotaryEncoder &left_encoder;
    EXTIRotaryEncoder &right_encoder;

    SteeringEngine &steering_engine;
    UltrasonicSensor &ultrasonic_sensor;

private:
    constexpr static inline double CRITICAL_DIST = 0.5; // 临界距离（米）
    std::array<double, 180> dist_datas = {};
    State state = State::COMMON;

    Delayer delayer; // 节拍延时，绑定到构造时传入的定时器

public:
    inline ObstacleAvoidingCar(
        DCMotorDriver &motor_driver,
        EXTIRotaryEncoder &left_encoder,
        EXTIRotaryEncoder &right_encoder,
        SteeringEngine &steering_engine,
        UltrasonicSensor &ultrasonic_sensor,
        TIM_TypeDef *delay_TIMX) noexcept
        : motor_driver{motor_driver},
          left_encoder{left_encoder},
          right_encoder{right_encoder},
          steering_engine{steering_engine},
          ultrasonic_sensor{ultrasonic_sensor},
          delayer{delay_TIMX}
    {

        steering_engine.set_degree(0);
        // 节拍时长由定时管理器提供，这里只写毫秒
        delayer.delay_ms(500);
        motor_driver.work();
        steering_engine.set_degree(90);
    }

    virtual ~ObstacleAvoidingCar() = default;

    // 定时中断调用：只跑当前状态的 working
    void working() noexcept
    {
        volatile sys::IRQGuard guard;

        switch (state)
        {
        case State::COMMON:
            common_working();
            break;
        case State::SCAN:
            scan_working();
            break;
        case State::AVOID:
            avoid_working();
            break;
        }
    }

    // 状态转移：旧状态 exit -> 新状态 init
    void transition_to(State next) noexcept
    {
        if (next == state)
        {
            return;
        }

        switch (state)
        {
        case State::COMMON:
            common_exit();
            break;
        case State::SCAN:
            scan_exit();
            break;
        case State::AVOID:
            avoid_exit();
            break;
        }

        state = next;

        switch (state)
        {
        case State::COMMON:
            common_init();
            break;
        case State::SCAN:
            scan_init();
            break;
        case State::AVOID:
            avoid_init();
            break;
        }
    }

protected:
    //-------------------------------- common ---------------------------------

    virtual void common_init() noexcept
    {
        motor_driver.set_mode(DCMotorDriver::Mode::FORWARD);
        motor_driver.set_speed(300, 0);
        motor_driver.set_speed(300, 1);
    }

    // 非避障时的执行函数
    virtual void common_working() noexcept
    {
        // 实时更新超声测距
        if (ultrasonic_sensor.get_distance() <= CRITICAL_DIST)
        {
            transition_to(State::SCAN); // 判断障碍物在左还是右
        }
        
    }

    virtual void common_exit() noexcept
    {
        motor_driver.set_speed(0, 0);
        motor_driver.set_speed(0, 1);
        motor_driver.set_mode(DCMotorDriver::Mode::BRAKE); // 刹车
    }

    //--------------------------------- scan ----------------------------------

    void scan_init() noexcept
    {
        steering_engine.set_degree(0);
        delayer.start_ms(500); // 非阻塞：只装载 500 ms，到点在 scan_working 里查询
    }

    void scan_working() noexcept
    {
        // 等舵机转到 0 度并稳定
        if (!delayer.done())
        {
            return;
        }

        for (const uint8_t degree : range(0, 180))
        {
            steering_engine.set_degree(degree);
            dist_datas[degree] = ultrasonic_sensor.get_distance();
        }
        transition_to(State::AVOID);
    }

    void scan_exit() noexcept
    {
        steering_engine.set_degree(90);
    }

    //--------------------------------- avoid ---------------------------------

    void avoid_init() noexcept
    {
        // 判断障碍物在左还是右
    }

    void avoid_working() noexcept
    {
    }

    void avoid_exit() noexcept
    {
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END
