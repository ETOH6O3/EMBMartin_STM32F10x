#include "obstacle_avoiding_car.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

ObstacleAvoidingCar::ObstacleAvoidingCar(
    Car &_car,
    SteeringEngine &_steering_engine,
    UltrasonicSensor &_ultrasonic_sensor,
    TIM_TypeDef *delay_TIMX) noexcept
    : car{_car},
      steering_engine{_steering_engine},
      ultrasonic_sensor{_ultrasonic_sensor},
      delayer{delay_TIMX}
{
    steering_engine.set_degree(CENTER_DEGREE); // 回正
    common_init();
}

// 状态转移：旧状态 exit -> 新状态 init
void ObstacleAvoidingCar::transition_to(State next) noexcept
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

// 非避障时的执行函数：只轮询超声，不驱动电机
void ObstacleAvoidingCar::common_working() noexcept
{
    if (!__common_armed)
    {
        if (!delayer.is_ready()) // 定时器还没跑起来，下一拍再试
        {
            return;
        }
        delayer.start_ms(COMMON_SETTLE_MS);
        __common_armed = true;
        return;
    }

    if (!delayer.done()) // 刚起步/刚回到本状态，先等超声稳定
    {
        return;
    }

    if (!ultrasonic_sensor.has_measurement()) // 还没收到过回波，cnt 为 0 不代表贴脸
    {
        return;
    }

    // 实时更新超声测距
    if (ultrasonic_sensor.get_distance() <= CRITICAL_DIST)
    {
        dist_datas[2] = ultrasonic_sensor.get_distance();
        transition_to(State::SCAN); // 进入避障程序
    }
}

/// 进入扫描点：摆舵机并装载等待
void ObstacleAvoidingCar::scan_enter_point(uint8_t __idx) noexcept
{
    steering_engine.set_degree(SCAN_POINTS[__idx].angle);
    delayer.start_ms(SCAN_POINTS[__idx].wait_ms);
}

// 扫描期间只轮询舵机是否到位，到位就取一个点，然后切下一个角度
void ObstacleAvoidingCar::scan_working() noexcept
{
    if (!delayer.done())
    {
        return; // 还没转到位
    }

    // 到位，取数
    dist_datas[SCAN_POINTS[__step].index] = ultrasonic_sensor.get_distance();

    if (++__step >= SCAN_POINTS.size())
    {
        transition_to(State::AVOID);
        return;
    }

    scan_enter_point(__step);
}

void ObstacleAvoidingCar::turn(double degrees) noexcept
{
    const bool left = degrees > 0.0;
    // 左转（正）绕左轮、速度取正；右转（负）绕右轮、速度取负 —— 都是绕内侧轮
    car.pivot_turn_async(left ? Car::Pivot::LEFT : Car::Pivot::RIGHT,
                         left ? DRIVE_SPEED_CM_S : -DRIVE_SPEED_CM_S,
                         degrees);
}

/// 启动第 __idx 步（转向或直行），不等待
void ObstacleAvoidingCar::avoid_enter_step(uint8_t __idx) noexcept
{
    const AvoidStep &step = __plan[__idx];
    if (step.is_turn)
        turn(step.param);
    else
        car.forward_distance_async(DRIVE_SPEED_CM_S, step.param);
}

bool ObstacleAvoidingCar::interrupt_avoid() noexcept
{
    if (!avoid_can_be_interrupted())
    {
        return false;
    }

    car.stop();
    transition_to(State::COMMON);
    on_avoid_interrupted();
    return true;
}

void ObstacleAvoidingCar::avoid_init() noexcept
{
    // 判断障碍物在左还是右：左侧更近 => 障碍在左 => 向右绕（负角度）
    auto right_dist = (dist_datas[0] + dist_datas[1] + dist_datas[2]) / 3.0; // 中位数据会略微受到右侧滤波影响，故算入
    auto left_dist = (dist_datas[3] + dist_datas[4]) / 2.0;
    this->obstacle_on_left = left_dist < right_dist;

    // 转向符号：障碍在左则先右转（负），障碍在右则先左转（正）
    const double dir = obstacle_on_left ? -1.0 : 1.0;

    // 穿越距离：实测前方距离（米 → cm）+ 余量
    const double forward_cm = dist_datas[2] * 100.0 + DIST_MARGIN_CM;

    // 转 → 跑 S → 转 → 跑 D → 转 → 跑 S → 转
    __plan = {{
        {true, dir * TURN_DEG},           // 1 驶离障碍物一侧
        {false, SIDE_SHIFT_CM},           // 2 偏航
        {true, -dir * TURN_DEG},          // 3 摆回原航向
        {false, forward_cm},              // 4 沿偏移车道越过障碍
        {true, -dir * TURN_DEG},          // 5 再摆向另一侧
        {false, SIDE_SHIFT_CM},           // 6 回航
        {true, dir * TURN_DEG},           // 7 回正
    }};

    __avoid_step = 0;
    avoid_enter_step(__avoid_step);
}

void ObstacleAvoidingCar::avoid_working() noexcept
{
    // 先处理打断请求：时机不合法时基类会自动忽略，继续推进原动作
    if (avoid_interrupt_requested() && interrupt_avoid())
    {
        return;
    }

    // 当前步还没跑完（Car 的 update 返回 false），本拍什么都不做
    if (!car.update())
    {
        return;
    }

    // 当前步完成，进下一步
    if (++__avoid_step < __plan.size())
    {
        avoid_enter_step(__avoid_step);
        return;
    }

    // 七步全部完成，回到正常行驶
    transition_to(State::COMMON);
}

EMBMARTIN_STM32F10X_NAMESPACE_END
