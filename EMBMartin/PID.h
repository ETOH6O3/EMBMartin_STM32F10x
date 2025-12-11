#ifndef EMBMARTIN_PID_H
#define EMBMARTIN_PID_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"
#include "I2C.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN


class AngleComplementaryFilter
{
private:
    const double alpha;            //!< 互补滤波系数
    const double dt;               //!< 采样时间间隔（秒）
    const MPU6050::Data &MPU_data; //!< MPU6050 数据引用

    double current_pitch = 0.0; //!< 当前俯仰角（度）
    double current_roll = 0.0;  //!< 当前横滚角（度）
    double current_yaw = 0.0;   //!< 当前偏航角（度）
public:
    struct Status
    {
        double pitch; //!< 俯仰角（度）
        double roll;  //!< 横滚角（度）
        double yaw;   //!< 偏航角（度）
    };
    AngleComplementaryFilter(
        MPU6050::Data &_MPU_data,
        double _alpha = 0.001, double _dt = 0.001) noexcept
        : alpha(_alpha), dt(_dt), MPU_data(_MPU_data) {};

    void update() noexcept; // 在中断函数或循环中定期调用以更新角度数据
    inline Status get_status() const noexcept { return {current_pitch, current_roll, current_yaw}; }
};

class SingleLoopPID
{
private:
    const double kp;             //!< 比例系数
    const double ki;             //!< 积分系数
    const double kd;             //!< 微分系数
    const double integral_limit; //!< 积分限幅值, 小于=0表示不限制
    const double output_limit;   //!< 输出限幅值，小于=0表示不限制
    const double alpha;          //!< 误差滤波系数

    const double &current; //!< 当前值引用
    const double dt;       //!< 采样时间间隔（秒）

    const int16_t* d_current; // 可选：直接使用现成的微分值

    double previous_error = 0.0; //!< 上一次误差值
    double integral = 0.0;       //!< 积分值

public:
    inline SingleLoopPID(
        double _kp, double _ki, double _kd, double &_current, double _dt,
        double _integral_limit, double _output_limit, double _alpha = 0.0 /*默认不滤波*/, const int16_t* _d_current = nullptr) noexcept
        : kp(_kp), ki(_ki), kd(_kd), current(_current), dt(_dt),
          integral_limit(_integral_limit), output_limit(_output_limit), alpha(std::clamp(_alpha, 0.0, 1.0)), d_current(_d_current) {};

    double compute(double target) noexcept; // 中断函数中调用

    inline void reinit()noexcept
    {
        this->previous_error = 0;
        this->integral = 0;
    }
};

struct PIDParams
{
    double kp;
    double ki;
    double kd;
    double integral_limit;
    double output_limit;
    double alpha = 0.0; // 误差滤波系数，默认不滤波
};

class BalancedCarPID
{
private:
    // 数据获取
    const MPU6050::Data &MPU_data; //!< MPU6050 数据引用
    const int16_t &pace_left;      //!< 左轮编码器数据引用
    const int16_t &pace_right;     //!< 右轮编码器数据引用
    // 互补滤波器
    AngleComplementaryFilter angle_filter;         //!< 角度互补滤波器
    AngleComplementaryFilter::Status angle_status; //!< 当前角度状态,每次compute更新
    const double pitch_med_angle;                 //!< 平衡车静止时的俯仰角（度）
    // PID 控制器
    SingleLoopPID vertical; //!< 垂直方向 PID 控制器
    SingleLoopPID velocity; //!< 速度方向 PID 控制器
    SingleLoopPID turn;     //!< 转向方向 PID 控制器
    // 缓存值（每次compute更新）
    double velocity_pid_current; //!< 速度 PID current 成员
    double turn_pid_current;     //!< 转向 PID current 成员

public:
    struct Output
    {
        double duty_left_permillage;  //!< 左轮占空比千分比
        double duty_right_permillage; //!< 右轮占空比千分比
    };
    inline BalancedCarPID(MPU6050::Data &_MPU_data,
                          int16_t &_pace_left, int16_t &_pace_right,
                          PIDParams vertical_params, PIDParams velocity_params, PIDParams turn_params,
                          double dt /* 采样时间间隔（秒） */, double _pitch_med_angle) noexcept
        : MPU_data(_MPU_data),
          pace_left(_pace_left), pace_right(_pace_right),
          angle_filter(_MPU_data, 0.001, dt),
          vertical(vertical_params.kp, vertical_params.ki, vertical_params.kd,
                   this->angle_status.pitch, dt,
                   vertical_params.integral_limit, vertical_params.output_limit, vertical_params.alpha,
                &this->MPU_data.gyro_y),
          velocity(velocity_params.kp, velocity_params.ki, velocity_params.kd,
                   this->velocity_pid_current, dt,
                   velocity_params.integral_limit, velocity_params.output_limit, velocity_params.alpha),
          turn(turn_params.kp, turn_params.ki, turn_params.kd,
               this->turn_pid_current, dt,
               turn_params.integral_limit, turn_params.output_limit, turn_params.alpha),
                pitch_med_angle(_pitch_med_angle) {};

    Output compute(double turn_trg, double velocity_trg) noexcept; // 中断函数中调用

    
};


EMBMARTIN_STM32F10X_NAMESPACE_END


#endif // EMBMARTIN_PID_H