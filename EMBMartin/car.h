#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"
#include "PWM.h"
#include "EXTI.h"
#include "TIM.h"
#include "PID.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief 差速小车运动封装：组合 DCMotorDriver 与两个 EXTIRotaryEncoder
 *
 * 本类把「左右轮占空比」这一层与「厘米 / 度」这一层隔开：调用者只描述
 * 速度（cm/s）、距离（cm）、角度（度），轮速换算、方向判定、到位判停全部在这里完成。
 *
 * ---------------------------------- 物理换算 ----------------------------------
 *
 * 构造参数里有四个用于标定的量：
 *
 * - 轮距 L（cm）：左右轮中心的横向距离；
 *
 * - 占空比 / 编码器速度比值 r（‰ 每 计数/s）：实测「给定占空比 → 编码器读数」的斜率；
 *
 * - 编码器计数比 N（计数/圈）：轮子转一圈编码器给出的计数；
 *
 * - 轮径 d（cm）。
 *
 * 由后两者可得每个计数对应的行进距离
 *
 *     cm_per_count = π·d / N
 *
 * 于是把 r 从「‰ 每 计数/s」折算成「‰ 每 cm/s」
 *
 *     duty_per_speed = r / cm_per_count
 *
 * 这两个量才是运动时真正用得到的，其余标定量在构造后即可丢弃。
 *
 * ---------------------------------- 速度不是开环 ----------------------------------
 *
 * r 只是均值：同一占空比下左右轮的转速差、电池电压跌落、地面阻力都会让它漂。
 * 因此每个轮子各挂一个 SingleLoopPID 做**转速闭环校准**，被控量就是上面那个
 * duty_per_speed 折算出来的前馈量：
 *
 *     duty = duty_per_speed × v_target + PID(实测速度)
 *
 * 由于 SingleLoopPID 的输出方向与 (current - target) 同向，而这里希望
 * 「速度偏慢 → 加大占空比」，所以把反馈量取反：
 *
 *     current = -实测速度(cm/s)，target = -目标速度(cm/s)
 *     → error = v_target - v，输出为正表示需要加速
 *
 * 这样 kp/ki/kd 全部填正值即可。
 *
 * ------------------------------- PID 参数是引用 -------------------------------
 *
 * PIDParams 以 **const 引用** 持有，不拷贝：外部改了这个对象，下一次动作就会用新参数。
 *
 * 两个 PID 在**每次动作开始时**按参数的当前值现造（放在栈上），用完即弃；
 * 动作执行期间每个控制周期还会把 kp/ki/kd 从 PIDParams 再同步一次
 * （只改增益、不清积分），所以**动作执行中途调参也能立刻生效**，且不会因为
 * 重新构造 PID 而让控制量跳变。
 *
 * 选「每次现造 + 逐拍同步增益」而不是「长期持有成员 PID」，附带两个好处：
 *
 * - 每次动作开始时积分自然清零，不会把上一次动作（可能方向相反）的积分带进来；
 * - dt 用的是动作发起时查到的真实采样周期，定时器周期被改过也能自动跟上。
 *
 * 影响：
 *
 * - 调参从下一个控制周期开始生效；
 * - 代价是两个 SingleLoopPID 约一百多字节的栈开销（仅在动作执行期间存在）。
 *
 * @note PIDParams 对象必须比 Car 活得久（通常是全局或静态对象）。
 * @note integral_limit / output_limit / alpha 仍然只在动作开始时读取一次：
 *       它们参与的是限幅与滤波的结构，中途改容易让输出突变，如需修改请在两次动作之间进行。
 *
 * ---------------------------------- 方向约定 ----------------------------------
 *
 * - 速度正值 = 前进；
 *
 * - 轮速差 = v_右 - v_左，正值 = 左转（俯视逆时针）；
 *
 * - 角度正值 = 左转（逆时针），负值 = 右转（顺时针）；
 *
 * - 左右编码器各带一个 ±1 极性：+1 表示「车往前进方向跑时 get_speed() 为正」。
 *   实车上经常有一路是反的，用 -1 修正，不必重新接线。
 *
 * ---------------------------------- 阻塞式 ----------------------------------
 *
 * 五个动作全部是**阻塞式**：函数返回时车已经停下、动作已经完成。
 * 等待用 TIM.h 里的 Delayer，也就是 sys::InnerTimerManager 的定时器节拍，
 * 等待期间**不关中断**，编码器采样、串口、超声都照常工作
 * （这是它区别于 sys::delay_ms / Delay_ms 的关键）。
 *
 * @note 1. 编码器必须绑定到 encoder_TIMX 这个定时器，且该定时器的更新中断已开启；
 *          否则 is_ready() 为 false，所有动作直接返回不动车（不会死循环）。
 * @note 2. 到位判停的分辨率等于一个编码器采样周期，超调量约为「一拍内的行程」。
 *          需要更准就把采样定时器的周期调小。
 * @note 3. 不可在中断服务函数里调用这些阻塞动作。
 * @note 4. 本类会在 encoder_TIMX 上额外占用一个 InnerTimerManager 回调槽位（Delayer）。
 *
 * @code
 * // 全局区（定时器必须先于 Car 构造）
 * sys::InnerTimerManager timer1{TIM1, 0.2, true, 0, 0};
 * EXTIRotaryEncoder left_encoder{PB5, PB6, 0, 0, EXTI_Mode_Interrupt, TIM1};
 * EXTIRotaryEncoder right_encoder{PB8, PB7, 0, 0, EXTI_Mode_Interrupt, TIM1};
 *
 * // 单独放一个全局参数对象，随时可改，Car 会引用它
 * PIDParams speed_pid{ .kp = 2.0, .ki = 1.0, .kd = 0.0,
 *                      .integral_limit = 300.0, .output_limit = 400.0, .alpha = 0.0 };
 *
 * Car car{driver, left_encoder, right_encoder,
 *         15.0,  // 轮距 15 cm
 *         0.9,   // 占空比‰ / (计数/s)
 *         780.0, // 计数/圈
 *         6.5,   // 轮径 6.5 cm
 *         TIM1,
 *         speed_pid,
 *         1,     // 左编码器极性
 *         -1);   // 右编码器极性（实车反装）
 *
 * car.forward_distance(30.0, 100.0);  // 以 30 cm/s 前进 100 cm
 * speed_pid.kp = 3.0;                 // 调参，下一个动作生效
 * car.spin_turn(20.0, 90.0);          // 原地左转 90 度，轮速差 20 cm/s
 * @endcode
 */
class Car
{
public:
    /// 以哪一侧车轮为旋转中心
    enum class Pivot : uint8_t
    {
        LEFT,  //!< 以左轮为中心（右轮画半径 = 轮距 的圆）
        RIGHT  //!< 以右轮为中心（左轮画半径 = 轮距 的圆）
    };

    /// 一次采样窗口内的累计行程，传给内部的判停谓词
    struct Travel
    {
        double left_cm = 0.0;   //!< 左轮累计行程（cm）
        double right_cm = 0.0;  //!< 右轮累计行程（cm）
        double elapsed_s = 0.0; //!< 已过去的秒数

        /// 车体中心的行程（cm），左右轮的平均
        [[nodiscard]] inline double center_cm() const noexcept
        {
            return (left_cm + right_cm) * 0.5;
        }
    };

EMBMARTIN_DEBUGING_SPECIFIER:
    static constexpr double PI = 3.14159265358979323846;

    /// DCMotorDriver::set_speed 自身把占空比夹到 ±990，这里保持一致
    static constexpr double MAX_DUTY_PERMILLAGE = 990.0;

    /// 定时器还没初始化、查不到周期时的兜底控制周期（与工程里 TIM1 的 0.2 s 一致）
    static constexpr double DEFAULT_CONTROL_PERIOD_S = 0.2;

    DCMotorDriver &__driver;
    EXTIRotaryEncoder &__encoder_left;
    EXTIRotaryEncoder &__encoder_right;

    const double __track_width_cm; //!< 轮距（cm）
    const double __cm_per_count;   //!< 每个计数对应的行进距离 π·d/N（cm）
    const double __duty_per_speed; //!< 占空比千分率 / 移动速度（cm/s）

    const int8_t __left_polarity;  //!< 左编码器极性 ±1
    const int8_t __right_polarity; //!< 右编码器极性 ±1

    /// 转速环参数：**不拥有**，引用外部对象，随时可调
    const PIDParams &__speed_params;

    TIM_TypeDef *const __encoder_TIMX; //!< 编码器速度采样用的定时器
    const uint16_t __sample_interval;  //!< 一个采样周期包含几个定时器节拍

    // 两个转速闭环：长期持有，参数由 sync_pid_params() 从 __speed_params 同步过来
    double __speed_input_left = 0.0;    //!< 左轮 PID 反馈：-实测速度
    double __speed_input_right = 0.0;   //!< 右轮 PID 反馈：-实测速度
    double __measured_left_cm_s = 0.0;  //!< 左轮实测速度（cm/s，已带极性）
    double __measured_right_cm_s = 0.0; //!< 右轮实测速度（cm/s，已带极性）

    SingleLoopPID __pid_left;  //!< 左轮转速闭环
    SingleLoopPID __pid_right; //!< 右轮转速闭环

    Delayer __sampler; //!< 按编码器采样节拍推进的定时器节拍计数器

    // ------------------------------ 内部工具 ------------------------------

    /// 单个计数对应的行进距离（cm）；标定量非法时返回 0（此时车不会动）
    [[nodiscard]] static inline double __cm_per_count_of(double counts_per_revolution, double wheel_diameter_cm) noexcept
    {
        if (counts_per_revolution <= 0.0 || wheel_diameter_cm <= 0.0)
            return 0.0;
        return PI * wheel_diameter_cm / counts_per_revolution;
    }

    /// 当前控制周期（秒）；运行时动态查询，定时器就绪后自动跟上真实值
    [[nodiscard]] inline double __sample_period_s() const noexcept
    {
        if (__encoder_TIMX != nullptr)
        {
            const double tick = sys::InnerTimerManager::period(__encoder_TIMX);
            if (tick > 0.0)
                return tick * double(__sample_interval);
        }
        return DEFAULT_CONTROL_PERIOD_S;
    }

    /// 由累计行程算车体转角（度），正值 = 左转
    [[nodiscard]] inline double __angle_deg(const Travel &travel) const noexcept
    {
        if (__track_width_cm <= 0.0)
            return 0.0;
        return (travel.right_cm - travel.left_cm) / __track_width_cm * (180.0 / PI);
    }

    /// 已过去的秒数：优先用定时器节拍累计（sys::InnerTimerManager 是唯一时间基准）
    [[nodiscard]] inline double __elapsed_s(uint32_t begin_ticks, uint32_t steps, double sample_s) const noexcept
    {
        const double tick = (__encoder_TIMX != nullptr) ? sys::InnerTimerManager::period(__encoder_TIMX) : 0.0;
        if (tick > 0.0)
            return double(__sampler.get_ticks() - begin_ticks) * tick;
        return double(steps) * sample_s; // 节拍没跑起来时退化为按拍数折算
    }

    /// 直接下发左右轮占空比（‰），内部做限幅
    inline void __output_permillage(double duty_left, double duty_right) noexcept
    {
        __driver.set_speed(int16_t(std::clamp(duty_left, -MAX_DUTY_PERMILLAGE, MAX_DUTY_PERMILLAGE)), 0);
        __driver.set_speed(int16_t(std::clamp(duty_right, -MAX_DUTY_PERMILLAGE, MAX_DUTY_PERMILLAGE)), 1);
    }

    /// 把 PIDParams 的当前值下发给两个转速闭环（只改参数，不清积分）
    inline void sync_pid_params() noexcept
    {
        __pid_left.set(__speed_params.kp, __speed_params.ki, __speed_params.kd, __sample_period_s(),
                       __speed_params.integral_limit, __speed_params.output_limit, __speed_params.alpha);
        __pid_right.set(__speed_params.kp, __speed_params.ki, __speed_params.kd, __sample_period_s(),
                        __speed_params.integral_limit, __speed_params.output_limit, __speed_params.alpha);
    }

    /// 停车并清掉反馈量
    inline void __stop_motors() noexcept
    {
        __output_permillage(0.0, 0.0);
        __speed_input_left = 0.0;
        __speed_input_right = 0.0;
        __measured_left_cm_s = 0.0;
        __measured_right_cm_s = 0.0;
    }

    /**
     * @brief 阻塞式运动内核：按给定左右轮目标速度运行，直到判停谓词成立
     *
     * 每调用一次 __sampler.delay() 正好推进一个编码器采样周期，因此循环体
     * 与编码器的采样窗口严格一一对应：
     *
     * - get_speed() 每个窗口只读一次并累加，不会重复计入同一窗口；
     * - 再叠加 get_count()（当前窗口内尚未采完的部分计数）并扣掉起步时刻的
     *   那个残段，就得到「起步至今」的精确累计计数。
     *
     * 两个 SingleLoopPID 是成员，长期持有；每次动作开始时先 sync_pid_params()
     * 把 PIDParams 的当前值下发下去并清积分，动作执行期间每拍再同步一次增益，
     * 所以**动作执行中途调参也能立刻生效**，且不会让控制量跳变。
     *
     * @param target_left_cm_s 左轮目标速度（cm/s）
     * @param target_right_cm_s 右轮目标速度（cm/s）
     * @param should_stop 判停谓词，签名 bool(const Travel &)
     */
    template <typename StopFn>
    void __run(double target_left_cm_s, double target_right_cm_s, StopFn should_stop) noexcept
    {
        __measured_left_cm_s = 0.0;
        __measured_right_cm_s = 0.0;
        __speed_input_left = 0.0;
        __speed_input_right = 0.0;

        const Travel initial; // 起步前状态：全部为 0
        if (!is_ready() || should_stop(initial))
        {
            __stop_motors();
            return;
        }

        const double sample_s = __sample_period_s();

        // 参数下发 + 清积分：每次动作都从干净的积分状态起步
        sync_pid_params();
        __pid_left.reinit();
        __pid_right.reinit();

        const double feedforward_left = __duty_per_speed * target_left_cm_s;
        const double feedforward_right = __duty_per_speed * target_right_cm_s;

        // 起步同步：丢掉上一个采样窗口（它发生在下令之前），并记下当前窗口的残段计数
        const int16_t begin_left = __encoder_left.get_count();
        const int16_t begin_right = __encoder_right.get_count();
        (void)__encoder_left.get_speed();
        (void)__encoder_right.get_speed();
        const uint32_t begin_ticks = __sampler.get_ticks();

        int32_t summed_left = 0; // 已完成的采样窗口计数之和
        int32_t summed_right = 0;
        uint32_t steps = 0;

        // 首拍还没有任何实测值，先只给前馈，避免闭环一上来就把占空比推到饱和
        __output_permillage(feedforward_left, feedforward_right);

        while (true)
        {
            __sampler.delay(__sample_interval); // 等一个采样周期，中断照常响应
            ++steps;

            const int16_t speed_left = __encoder_left.get_speed();
            const int16_t speed_right = __encoder_right.get_speed();
            summed_left += speed_left;
            summed_right += speed_right;

            __measured_left_cm_s = double(speed_left) * __cm_per_count * __left_polarity / sample_s;
            __measured_right_cm_s = double(speed_right) * __cm_per_count * __right_polarity / sample_s;

            const Travel travel{
                double(summed_left + __encoder_left.get_count() - begin_left) * __cm_per_count * __left_polarity,
                double(summed_right + __encoder_right.get_count() - begin_right) * __cm_per_count * __right_polarity,
                __elapsed_s(begin_ticks, steps, sample_s)};

            // 反馈取反：让 SingleLoopPID 的 error 变成 (v_target - v_measured)
            __speed_input_left = -__measured_left_cm_s;
            __speed_input_right = -__measured_right_cm_s;

            // 增益每拍从 PIDParams 同步一次：动作执行中途调参也能立刻生效，
            // 只改增益、不清积分，控制量不会因为重新构造而跳变。
            __pid_left.set_gains(__speed_params.kp, __speed_params.ki, __speed_params.kd);
            __pid_right.set_gains(__speed_params.kp, __speed_params.ki, __speed_params.kd);

            __output_permillage(
                feedforward_left + __pid_left.compute(-target_left_cm_s),
                feedforward_right + __pid_right.compute(-target_right_cm_s));

            if (should_stop(travel))
                break;
        }

        __stop_motors();
    }

    /// 判停：车体中心行程的绝对值是否达到目标
    [[nodiscard]] inline bool __reached_distance(const Travel &travel, double distance_cm) const noexcept
    {
        return std::abs(travel.center_cm()) >= distance_cm;
    }

    /// 判停：转角绝对值是否达到目标
    [[nodiscard]] inline bool __reached_angle(const Travel &travel, double degrees) const noexcept
    {
        return std::abs(__angle_deg(travel)) >= degrees;
    }

public:
    /**
     * @brief 构造一辆差速小车
     *
     * @param _driver 电机驱动器（左轮用 motor_index = 0，右轮用 1）
     * @param _encoder_left 左轮编码器
     * @param _encoder_right 右轮编码器
     * @param _track_width_cm 轮距（cm），左右轮中心的横向距离
     * @param _duty_per_count_speed 占空比（千分率）与编码器速度（计数/s）的比值
     * @param _counts_per_revolution 编码器计数比（计数/圈）
     * @param _wheel_diameter_cm 轮径（cm）
     * @param encoder_TIMX 编码器测速所绑定的定时器，用于动态查询采样周期
     * @param _speed_params 左右**公用**一套的转速环 PID 参数；**引用持有**，可随时修改
     * @param _left_polarity 左编码器极性 ±1（+1：车轮往前进方向转时读数为正）
     * @param _right_polarity 右编码器极性 ±1
     * @param _sample_interval_cnt 编码器的采样分频，必须与 EXTIRotaryEncoder 构造时
     *                             传入的 sample_interval_cnt 一致，默认 1
     *
     * @note _counts_per_revolution 与 _wheel_diameter_cm 必须为正，否则车不动。
     * @note _speed_params 必须比本对象活得久。
     */
    inline Car(
        DCMotorDriver &_driver,
        EXTIRotaryEncoder &_encoder_left,
        EXTIRotaryEncoder &_encoder_right,
        double _track_width_cm,
        double _duty_per_count_speed,
        double _counts_per_revolution,
        double _wheel_diameter_cm,
        TIM_TypeDef *encoder_TIMX,
        const PIDParams &_speed_params,
        int8_t _left_polarity = 1,
        int8_t _right_polarity = 1,
        uint16_t _sample_interval_cnt = 1) noexcept
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

    Car(const Car &) = delete;
    Car &operator=(const Car &) = delete;
    Car(Car &&) = delete;
    Car &operator=(Car &&) = delete;

    // ------------------------------ 标定量查询 ------------------------------

    /// 轮距（cm）
    [[nodiscard]] inline double get_track_width_cm() const noexcept { return __track_width_cm; }

    /// 占空比千分率 / 移动速度（cm/s）
    [[nodiscard]] inline double get_duty_per_speed() const noexcept { return __duty_per_speed; }

    /// 每个计数对应的行进距离（cm）
    [[nodiscard]] inline double get_cm_per_count() const noexcept { return __cm_per_count; }

    /// 一个闭环控制周期（秒）
    [[nodiscard]] inline double get_control_period_s() const noexcept { return __sample_period_s(); }

    /// 取回当前使用的 PID 参数（可直接改字段，下一个控制周期生效）
    [[nodiscard]] inline PIDParams &get_speed_params() noexcept
    {
        return const_cast<PIDParams &>(__speed_params);
    }

    /// 主动把 PIDParams 的当前值下发给两个闭环（动作执行中途调参后可立即调用；
    /// 下一个动作开始时也会自动同步，所以一般不必手动调）
    inline void apply_pid_params() noexcept { sync_pid_params(); }

    /// 测速定时器是否已初始化并开始走拍；为 false 时所有动作都不会动车
    [[nodiscard]] inline bool is_ready() const noexcept
    {
        return __encoder_TIMX != nullptr && sys::InnerTimerManager::is_ticking(__encoder_TIMX);
    }

    // ------------------------------ 实时量查询 ------------------------------

    /// 最近一个采样窗口的左轮速度（cm/s，已带极性）
    [[nodiscard]] inline double get_left_speed_cm_s() const noexcept { return __measured_left_cm_s; }

    /// 最近一个采样窗口的右轮速度（cm/s，已带极性）
    [[nodiscard]] inline double get_right_speed_cm_s() const noexcept { return __measured_right_cm_s; }

    /// 立刻停车（等价于把当前动作提前结束）
    inline void stop() noexcept { __stop_motors(); }

    /**
     * @brief 非阻塞地直接下发左右轮目标速度（cm/s），立即返回
     *
     * 与下面五个动作的区别：
     *
     * - **不等待、不停车**：下发后函数立刻返回，电机保持这个速度一直转，
     *   直到下一次调用本函数或 stop()。适合「主循环里一边前进一边轮询传感器」；
     * - **纯前馈**：只按 duty_per_speed 换算占空比，不跑 PID 闭环，因此实际速度
     *   依赖标定比值。需要精确到 cm 或精确转角时请用阻塞动作。
     *
     * @param left_cm_s 左轮目标速度（cm/s），正值前进
     * @param right_cm_s 右轮目标速度（cm/s），正值前进
     *
     * @note 调用后 get_left_speed_cm_s() 不会更新（那是闭环动作的实测值），
     *       需要实测速度请直接读编码器。
     */
    inline void drive(double left_cm_s, double right_cm_s) noexcept
    {
        __output_permillage(__duty_per_speed * left_cm_s, __duty_per_speed * right_cm_s);
    }

    // ------------------------------ 五个运动动作 ------------------------------

    /**
     * @brief 以指定速度前进指定距离（阻塞）
     *
     * 左右轮同速，行驶过程中由闭环把实际速度校准到 speed_cm_s。
     *
     * @param speed_cm_s 前进速度（cm/s）；负值表示后退，此时距离仍取正值
     * @param distance_cm 目标距离（cm），取绝对值
     *
     * @note 停车判定精度为一个编码器采样周期内的行程。
     */
    inline void forward_distance(double speed_cm_s, double distance_cm) noexcept
    {
        const double target = std::abs(distance_cm);
        __run(speed_cm_s, speed_cm_s, [this, target](const Travel &travel)
              { return __reached_distance(travel, target); });
    }

    /**
     * @brief 以指定速度前进指定时间（阻塞）
     *
     * @param speed_cm_s 前进速度（cm/s），负值表示后退
     * @param seconds 行驶时间（秒），取绝对值
     *
     * @note 计时走 sys::InnerTimerManager 的定时器节拍（经 Delayer 累计），
     *       不占用 SysTick，也不会关中断。
     */
    inline void forward_time(double speed_cm_s, double seconds) noexcept
    {
        const double target = std::abs(seconds);
        __run(speed_cm_s, speed_cm_s, [this, target](const Travel &travel)
              { return travel.elapsed_s >= target; });
    }

    /**
     * @brief 边前进边转向：给定中心前进速度与轮速差，转到指定角度为止（阻塞）
     *
     * 左右轮目标速度：
     *
     *     v_左 = center_cm_s - speed_diff_cm_s / 2
     *     v_右 = center_cm_s + speed_diff_cm_s / 2
     *
     * 由差速运动学 ω = (v_右 - v_左) / 轮距，所以轮速差为正即左转。
     * 判停用编码器累计行程算出的真实转角 (Δs_右 - Δs_左) / 轮距，
     * 与打滑、速度误差无关。
     *
     * @param center_cm_s 车体中心的前进速度（cm/s），可为 0（等价于原地转向）
     * @param speed_diff_cm_s 轮速差（cm/s），正值 = 左转
     * @param degrees 目标转角（度），正值左转、负值右转，取绝对值判停
     */
    inline void arc_turn(double center_cm_s, double speed_diff_cm_s, double degrees) noexcept
    {
        const double target = std::abs(degrees);
        __run(center_cm_s - speed_diff_cm_s * 0.5, center_cm_s + speed_diff_cm_s * 0.5,
              [this, target](const Travel &travel)
              { return __reached_angle(travel, target); });
    }

    /**
     * @brief 原地转向：中心不动，以给定轮速差旋转指定角度（阻塞）
     *
     *     v_左 = -speed_diff_cm_s / 2
     *     v_右 = +speed_diff_cm_s / 2
     *
     * 两轮各走 轮距/2 半径的圆弧，转角 = (Δs_右 - Δs_左) / 轮距。
     *
     * @param speed_diff_cm_s 轮速差（cm/s），正值 = 左转（逆时针）
     * @param degrees 目标转角（度），正值左转、负值右转
     */
    inline void spin_turn(double speed_diff_cm_s, double degrees) noexcept
    {
        const double target = std::abs(degrees);
        __run(-speed_diff_cm_s * 0.5, speed_diff_cm_s * 0.5,
              [this, target](const Travel &travel)
              { return __reached_angle(travel, target); });
    }

    /**
     * @brief 绕单侧车轮旋转：以左轮或右轮为圆心，以给定轮速差转指定角度（阻塞）
     *
     * 内轮（圆心那一侧）停转、占空比为 0，外轮速度就是 speed_diff_cm_s，
     * 外侧车轮走半径 = 轮距 的圆弧，所以转角 = Δs_外 / 轮距。
     *
     * - Pivot::LEFT：v_左 = 0，v_右 = +speed_diff_cm_s；
     * - Pivot::RIGHT：v_右 = 0，v_左 = -speed_diff_cm_s（保证正值仍是左转）。
     *
     * @param pivot 以哪一侧车轮为旋转中心
     * @param speed_diff_cm_s 轮速差（cm/s），正值 = 左转（逆时针）
     * @param degrees 目标转角（度），正值左转、负值右转
     */
    inline void pivot_turn(Pivot pivot, double speed_diff_cm_s, double degrees) noexcept
    {
        const double target = std::abs(degrees);
        const double left_target = (pivot == Pivot::LEFT) ? 0.0 : -speed_diff_cm_s;
        const double right_target = (pivot == Pivot::LEFT) ? speed_diff_cm_s : 0.0;
        __run(left_target, right_target, [this, target](const Travel &travel)
              { return __reached_angle(travel, target); });
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END

