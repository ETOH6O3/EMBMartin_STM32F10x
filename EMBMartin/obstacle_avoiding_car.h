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
private:
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
    std::array<double, 5> dist_datas = {}; // 0, 45, 90, 135, 180 度的距离数据
    State state = State::COMMON;

    Delayer delayer; // 延时器
    sys::InnerTimerManager::Handle __working_handle = {}; // 工作回调在定时器队列里的句柄

    bool obstacle_on_left = false; // 障碍物在左侧还是右侧

public:
    ObstacleAvoidingCar(
        DCMotorDriver &motor_driver,
        EXTIRotaryEncoder &left_encoder,
        EXTIRotaryEncoder &right_encoder,
        SteeringEngine &steering_engine,
        UltrasonicSensor &ultrasonic_sensor,
        TIM_TypeDef *work_TIMX,
        TIM_TypeDef *delay_TIMX) noexcept
        : motor_driver{motor_driver},
          left_encoder{left_encoder},
          right_encoder{right_encoder},
          steering_engine{steering_engine},
          ultrasonic_sensor{ultrasonic_sensor},
          __working_handle{sys::InnerTimerManager::add(work_TIMX, [this]()
                                      { this->working(); })},
          delayer{delay_TIMX}
    {

        steering_engine.set_degree(0);
        delayer.delay_ms(500);
        motor_driver.work();
        steering_engine.set_degree(90);

        common_init();
    }

    virtual ~ObstacleAvoidingCar() noexcept
    {
        sys::InnerTimerManager::remove(__working_handle);
    }

    // 定时中断调用，用于获取当前任务所需的实时数据
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
        motor_driver.set_mode(DCMotorDriver::Mode::FORWARD, 0);
        motor_driver.set_mode(DCMotorDriver::Mode::FORWARD, 1);
        motor_driver.set_speed(300, 0);
        motor_driver.set_speed(300, 1);
        delayer.delay_ms(500); // 让超声稳定
    }

    // 非避障时的执行函数
    virtual void common_working() noexcept
    {
        // 实时更新超声测距
        if (ultrasonic_sensor.get_distance() <= CRITICAL_DIST)
        {
            dist_datas[2] = ultrasonic_sensor.get_distance();
            transition_to(State::SCAN); // 进入避障程序
        }
        
    }

    virtual void common_exit() noexcept
    {
        motor_driver.set_speed(0, 0);
        motor_driver.set_speed(0, 1);
        motor_driver.set_mode(DCMotorDriver::Mode::BRAKE, 0); // 刹车
        motor_driver.set_mode(DCMotorDriver::Mode::BRAKE, 1); // 刹车
    }

private:

    //--------------------------------- scan ----------------------------------

    void scan_init() noexcept
    {
        steering_engine.set_degree(0);
        delayer.delay_ms(650); // 减弱互补滤波影响
        dist_datas[0] = ultrasonic_sensor.get_distance();
        // 扫描周围环境，获取不同角度的障碍物距离
        for (const uint8_t degree : range(1, 5))
        {
            if (degree == 2)
            {
                continue; // 90 度已经测过了
            }
            
            steering_engine.set_degree(degree * 45);
            delayer.delay_ms(450); // 等待稳定
            dist_datas[degree] = ultrasonic_sensor.get_distance();
        }
        transition_to(State::AVOID);
    }

    void scan_working() noexcept
    {
        // pass
        // 无需轮询，scan_init() 已经完成了扫描
    }

    void scan_exit() noexcept
    {
        steering_engine.set_degree(90);
    }

    //--------------------------------- avoid ---------------------------------

    void avoid_init() noexcept
    {
        // 判断障碍物在左还是右
        auto right_dist = (dist_datas[0] + dist_datas[1] + dist_datas[2]) / 3.0; // 90 度数据会略微受到右侧滤波影响，故算入
        auto left_dist = (dist_datas[3] + dist_datas[4]) / 2.0;
        this->obstacle_on_left = left_dist < right_dist;
        // 根据方位绕开障碍
        motor_driver.set_mode(DCMotorDriver::Mode::FORWARD, 0);
        motor_driver.set_mode(DCMotorDriver::Mode::FORWARD, 1);
        motor_driver.set_speed(300, !obstacle_on_left); // 转向
        delayer.delay_ms(500);
        motor_driver.set_speed(300, obstacle_on_left); // 偏航
        delayer.delay_ms(500);
        motor_driver.set_speed(0, !obstacle_on_left); // 转正
        delayer.delay_ms(500);
        motor_driver.set_speed(300, !obstacle_on_left); // 避障
        delayer.delay_ms(500);
        motor_driver.set_speed(0, !obstacle_on_left); // 转回
        delayer.delay_ms(500);
        motor_driver.set_speed(300, !obstacle_on_left); // 回到正轨
        delayer.delay_ms(500);
    }

    void avoid_working() noexcept
    {
        // pass
        // 无需轮询，avoid_init() 已经完成了避障
    }

    void avoid_exit() noexcept
    {
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END
