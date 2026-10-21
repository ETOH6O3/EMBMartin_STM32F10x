#include "car.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

void Car::__stop_motors() noexcept
{
    __output_permillage(0.0, 0.0);
    __speed_input_left = 0.0;
    __speed_input_right = 0.0;
    __measured_left_cm_s = 0.0;
    __measured_right_cm_s = 0.0;
}

void Car::__begin_action(double left_cm_s, double right_cm_s, Goal goal, double target) noexcept
{
    __measured_left_cm_s = 0.0;
    __measured_right_cm_s = 0.0;
    __speed_input_left = 0.0;
    __speed_input_right = 0.0;

    __goal = goal;
    __target = target;
    __sample_s = __sample_period_s();
    __left_cm_s = left_cm_s;
    __right_cm_s = right_cm_s;

    // 参数下发 + 清积分：每次动作都从干净的积分状态起步
    sync_pid_params();
    __pid_left.reinit();
    __pid_right.reinit();

    __ff_left = __duty_per_speed * left_cm_s;
    __ff_right = __duty_per_speed * right_cm_s;

    // 起步同步：丢掉上一个采样窗口（它发生在下令之前），并记下当前窗口的残段计数
    __begin_left = __encoder_left.get_count();
    __begin_right = __encoder_right.get_count();
    (void)__encoder_left.get_speed();
    (void)__encoder_right.get_speed();
    __begin_ticks = __sampler.get_ticks();
    __last_ticks = __begin_ticks;

    __summed_left = 0;
    __summed_right = 0;
    __steps = 0;

    // 首拍还没有任何实测值，先只给前馈，避免闭环一上来就把占空比推到饱和
    __output_permillage(__ff_left, __ff_right);
}

bool Car::__update(double left_cm_s, double right_cm_s) noexcept
{
    if (__goal == Goal::NONE)
        return true;

    // 一拍 = __sample_interval 个定时器节拍，用 tick 差值判断而不是阻塞等待
    const uint32_t now = __sampler.get_ticks();
    const uint32_t need = __sample_interval;
    if (uint32_t(now - __last_ticks) < need)
        return false; // 还没到下一拍
    __last_ticks = now;
    ++__steps;

    const int16_t speed_left = __encoder_left.get_speed();
    const int16_t speed_right = __encoder_right.get_speed();
    __summed_left += speed_left;
    __summed_right += speed_right;

    __measured_left_cm_s = double(speed_left) * __cm_per_count * __left_polarity / __sample_s;
    __measured_right_cm_s = double(speed_right) * __cm_per_count * __right_polarity / __sample_s;

    const Travel travel{
        double(__summed_left + __encoder_left.get_count() - __begin_left) * __cm_per_count * __left_polarity,
        double(__summed_right + __encoder_right.get_count() - __begin_right) * __cm_per_count * __right_polarity,
        __elapsed_s(__begin_ticks, __steps, __sample_s)};

    // 反馈取反：让 SingleLoopPID 的 error 变成 (v_target - v_measured)
    __speed_input_left = -__measured_left_cm_s;
    __speed_input_right = -__measured_right_cm_s;

    // 增益每拍从 PIDParams 同步一次：动作执行中途调参也能立刻生效，
    // 只改增益、不清积分，控制量不会因为重新构造而跳变。
    __pid_left.set_gains(__speed_params.kp, __speed_params.ki, __speed_params.kd);
    __pid_right.set_gains(__speed_params.kp, __speed_params.ki, __speed_params.kd);

    __output_permillage(
        __ff_left + __pid_left.compute(-left_cm_s),
        __ff_right + __pid_right.compute(-right_cm_s));

    switch (__goal)
    {
    case Goal::DISTANCE:
        return std::abs(travel.center_cm()) >= __target;
    case Goal::DURATION:
        return travel.elapsed_s >= __target;
    case Goal::ANGLE:
        return std::abs(__angle_deg(travel)) >= __target;
    default:
        return true;
    }
}

Car::Car(
    DCMotorDriver &_driver,
    EXTIRotaryEncoder &_encoder_left,
    EXTIRotaryEncoder &_encoder_right,
    double _track_width_cm,
    double _duty_per_count_speed,
    double _counts_per_revolution,
    double _wheel_diameter_cm,
    TIM_TypeDef *encoder_TIMX,
    const PIDParams &_speed_params,
    int8_t _left_polarity,
    int8_t _right_polarity,
    uint16_t _sample_interval_cnt) noexcept
    : __driver(_driver),
      __encoder_left(_encoder_left),
      __encoder_right(_encoder_right),
      __track_width_cm(_track_width_cm),
      __cm_per_count(__cm_per_count_of(_counts_per_revolution, _wheel_diameter_cm)),
      __duty_per_speed(__cm_per_count > 0.0 ? _duty_per_count_speed / __cm_per_count : 0.0),
      __left_polarity(_left_polarity < 0 ? int8_t(-1) : int8_t(1)),
      __right_polarity(_right_polarity < 0 ? int8_t(-1) : int8_t(1)),
      __speed_params(_speed_params),
      __encoder_TIMX(encoder_TIMX),
      __sample_interval(_sample_interval_cnt == 0 ? uint16_t(1) : _sample_interval_cnt),
      __pid_left(_speed_params.kp, _speed_params.ki, _speed_params.kd,
                 __speed_input_left, 1.0,
                 _speed_params.integral_limit, _speed_params.output_limit, _speed_params.alpha),
      __pid_right(_speed_params.kp, _speed_params.ki, _speed_params.kd,
                  __speed_input_right, 1.0,
                  _speed_params.integral_limit, _speed_params.output_limit, _speed_params.alpha),
      __sampler(encoder_TIMX != nullptr ? encoder_TIMX : TIM1)
{
    // dt 与限幅在构造末尾按运行期真实采样周期对齐一次
    sync_pid_params();
};

void Car::apply_pid_params() noexcept
{
    sync_pid_params();
}

void Car::forward_distance(double speed_cm_s, double distance_cm) noexcept
{
    forward_distance_async(speed_cm_s, distance_cm);
    while (!update())
    {
    }
}

void Car::forward_time(double speed_cm_s, double seconds) noexcept
{
    forward_time_async(speed_cm_s, seconds);
    while (!update())
    {
    }
}

void Car::arc_turn(double center_cm_s, double speed_diff_cm_s, double degrees) noexcept
{
    arc_turn_async(center_cm_s, speed_diff_cm_s, degrees);
    while (!update())
    {
    }
}

void Car::spin_turn(double speed_diff_cm_s, double degrees) noexcept
{
    spin_turn_async(speed_diff_cm_s, degrees);
    while (!update())
    {
    }
}

void Car::pivot_turn(Pivot pivot, double speed_diff_cm_s, double degrees) noexcept
{
    pivot_turn_async(pivot, speed_diff_cm_s, degrees);
    while (!update())
    {
    }
}

EMBMARTIN_STM32F10X_NAMESPACE_END
