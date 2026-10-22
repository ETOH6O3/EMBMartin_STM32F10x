#include "obstacle_avoiding_car.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

ObstacleAvoidingCar::ObstacleAvoidingCar(
    Car &_car,
    UltrasonicSensor &_ultrasonic_left,
    UltrasonicSensor &_ultrasonic_right) noexcept
    : car{_car},
      ultrasonic_left{_ultrasonic_left},
      ultrasonic_right{_ultrasonic_right}
{
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
    case State::AVOID:
        avoid_init();
        break;
    }
}

// 非避障时的执行函数：只轮询两个探头，不驱动电机
void ObstacleAvoidingCar::common_working() noexcept
{
    /* 每一侧各自判断，互不等待：只要某一侧已经收到过完整回波，就可以用那一侧。
     * 还没收到过回波的探头 cnt 恒为 0、会算出 0 米，那不代表贴脸有障碍物，
     * 因此**只跳过没测到的那一侧**，不能让一侧没测到就拖住另一侧的判断。 */
    const bool left_ready = ultrasonic_left.has_measurement();
    const bool right_ready = ultrasonic_right.has_measurement();

    if ((left_ready && ultrasonic_left.get_distance() <= CRITICAL_DIST) ||
        (right_ready && ultrasonic_right.get_distance() <= CRITICAL_DIST))
    {
        transition_to(State::AVOID);
    }
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
    /* 进入避障的可能是任意一侧，另一侧未必已有回波。没测到的一侧 get_distance()
     * 恒为 0，直接参与比较会把它误判成「更近」。因此只把**有效**的那一侧纳入比较，
     * 两侧都有效时才比较左右。 */
    const bool left_ready = ultrasonic_left.has_measurement();
    const bool right_ready = ultrasonic_right.has_measurement();
    const double left_m = left_ready ? ultrasonic_left.get_distance() : -1.0;
    const double right_m = right_ready ? ultrasonic_right.get_distance() : -1.0;

    if (left_ready && right_ready)
        obstacle_on_left = left_m < right_m; // 左侧更近 => 障碍在左 => 向右绕（负角度）
    else if (left_ready)
        obstacle_on_left = true;             // 只有左侧测到且它触发了 => 障碍在左
    else
        obstacle_on_left = false;            // 只有右侧有效（或都无效）=> 按障碍在右处理

    // 转向符号：障碍在左则先右转（负），障碍在右则先左转（正）
    const double dir = obstacle_on_left ? -1.0 : 1.0;

    // 穿越距离：只取有效那一侧；两侧都有效时取较小值。米 → cm，再加余量。
    double forward_m;
    if (left_ready && right_ready)
        forward_m = left_m < right_m ? left_m : right_m;
    else if (left_ready)
        forward_m = left_m;
    else if (right_ready)
        forward_m = right_m;
    else
        forward_m = 0.0; // 两侧都没测到（理论上不会进到这里）
    const double forward_cm = forward_m * 100.0 + DIST_MARGIN_CM;

    // 转 → 跑 S → 转 → 跑 D → 转 → 跑 S → 转
    __plan = {{
        {true, dir * TURN_DEG},  // 1 驶离障碍物一侧
        {false, SIDE_SHIFT_CM},  // 2 偏航
        {true, -dir * TURN_DEG}, // 3 摆回原航向
        {false, forward_cm},     // 4 沿偏移车道越过障碍
        {true, -dir * TURN_DEG}, // 5 再摆向另一侧
        {false, SIDE_SHIFT_CM + 5},  // 6 回航
        {true, dir * TURN_DEG},  // 7 回正
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
