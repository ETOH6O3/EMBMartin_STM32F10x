#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "car.h"
#include "functional.h"
#include "PWM.h"
#include "TIM.h"
#include "ultrasonic_sensor.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

/**
 * @brief 避障小车状态机
 *
 * 三层状态：COMMON（正常行驶并监视前方）→ SCAN（舵机扫描左右）→ AVOID（绕障）
 * → COMMON。每次状态转移先跑旧状态的 exit()，再跑新状态的 init()。
 *
 * ---------------------------------- 调用上下文 ----------------------------------
 *
 * working() **必须由主循环调用，不能挂在定时器中断里**。原因：
 *
 * 1. Car 的五个运动动作（forward_distance / spin_turn / ...）全是阻塞式，
 *    内部靠 Delayer 等定时器节拍推进；
 * 2. 阻塞动作放在 init() 里、init() 由 working() 里的 transition_to() 调用；
 * 3. 若 working() 在定时器 ISR 中运行，等待的正是该定时器自己的节拍 ——
 *    中断不能重入，节拍永远不推进，直接死锁。
 *
 * 因此本类不再向 InnerTimerManager 注册任何回调。超声采样、编码器采样、
 * 电机 PWM 仍由各自的定时器中断负责，与主循环并行。
 *
 * 分工：
 *
 * - init() **只执行一次**，负责所有电机动作（阻塞）；
 * - working() 只做「需要轮询读取环境信息」的判断，绝不驱动电机。
 *
 * ---------------------------------- 绕障航迹 ----------------------------------
 *
 * 七步，以障碍物在**左侧**为例（转到右侧绕行）。四次转向都是**绕内侧轮**
 * （左转绕左轮、右转绕右轮），内轮停转、外轮走半径 = 轮距 的圆弧。
 *
 * 绕轮转向时车心也会被带出一段位移：沿当时航向的弦长
 *
 *     Δ = e^{jφ} · (轮距/2) · (sinβ + j·sign(α)·(1-cosβ))
 *
 * 其中 β = |α|，φ 为转向前的航向，α 为该次转角（正 = 左转）。
 * 轮距 17 cm、β = 45° 时，沿航向的分量 6.01 cm、横向分量 2.49 cm。
 *
 * | 步 | 动作                       | 结束时航向 | 本步车心位移 (dx, dy) cm |
 * |----|----------------------------|-----------|--------------------------|
 * | 1  | pivot_turn RIGHT  -45°     |  -45°     | (+6.01, -2.49)           |
 * | 2  | forward_distance 35 cm     |  -45°     | (+24.75, -24.75)         |
 * | 3  | pivot_turn LEFT   +45°     |    0°     | (+6.01, -2.49)           |
 * | 4  | forward_distance D cm      |    0°     | (+D, 0)                  |
 * | 5  | pivot_turn LEFT   +45°     |  +45°     | (+6.01, +2.49)           |
 * | 6  | forward_distance 35 cm     |  +45°     | (+24.75, +24.75)         |
 * | 7  | pivot_turn RIGHT  -45°     |    0°     | (+6.01, +2.49)           |
 *
 * 四次转向的横向分量 −2.49 ×2 与 +2.49 ×2 恰好抵消，纵向分量则同向累加。
 * 因此 **净横向位移 = 0、净航向 = 0°、净前进 = D + 73.5 cm**，
 * 与原地转向的版本一样回到原航线上，只是整体多前进了 24 cm（转向本身带出的行程）。
 *
 * D = 进 AVOID 前测到的正前方距离 + DIST_MARGIN_CM，保证越过障碍后仍有余量。
 * 障碍物在右侧时全部转向取反。
 */
class ObstacleAvoidingCar
{
private:
    enum class State : uint8_t
    {
        COMMON, // 非避障
        SCAN,   // 扫描
        AVOID   // 避障
    };

public:
    /**
     * @brief 校准后的机械中位角度（度）
     *
     * 本舵机按标称中位 90 度回正时会向 180 度一侧偏一些，实测 83 度才真正朝正前方。
     * 所有「回正」动作都必须引用本常量，不要再写死 80 / 85 / 90。
     */
    constexpr static inline double CENTER_DEGREE = 83.0;

    /// 行驶与转向的统一速度（cm/s)
    constexpr static inline double DRIVE_SPEED_CM_S = 70.0;

    /// 绕障时侧移段的直行距离（cm）
    constexpr static inline double SIDE_SHIFT_CM = 15.0;

    /// 绕障时每次转向的角度（度）
    constexpr static inline double TURN_DEG = 40.0;

    /// 穿越障碍时在实测距离之上多走的余量（cm）
    constexpr static inline double DIST_MARGIN_CM = 15.0;

protected:
    Car &car;
    SteeringEngine &steering_engine;
    UltrasonicSensor &ultrasonic_sensor;

private:
    constexpr static inline double CRITICAL_DIST = 0.5; // 临界距离（米）

    /// 起步后等超声稳定的毫秒数
    constexpr static inline uint16_t COMMON_SETTLE_MS = 500;

    std::array<double, 5> dist_datas = {}; // 0, 45, 中位, 135, 180 度的距离数据
    State state = State::COMMON;

    Delayer delayer; // 延时器（挂 delay_TIMX，只用于等待，不驱动电机）

    bool obstacle_on_left = false; // 障碍物在左侧还是右侧
    uint8_t __step = 0;            // scan 状态机当前步进下标
    bool __common_armed = false;   // COMMON 状态的起步稳定等待是否已装载

public:
    /**
     * @brief 构造避障小车
     *
     * @param _car 差速小车（提供 forward_distance / spin_turn / drive / stop）
     * @param _steering_engine 超声波转向舵机
     * @param _ultrasonic_sensor 超声波传感器
     * @param delay_TIMX 提供等待节拍的定时器（只要中断已开启即可，不驱动电机）
     *
     * @note 构造期间不驱动电机：全局构造时定时器尚未建立，任何阻塞动作都会
     *       走退化路径。真正的起步由第一次 common_working() 完成。
     */
    ObstacleAvoidingCar(
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

    virtual ~ObstacleAvoidingCar() noexcept = default;

    ObstacleAvoidingCar(const ObstacleAvoidingCar &) = delete;
    ObstacleAvoidingCar &operator=(const ObstacleAvoidingCar &) = delete;

    // ------------------------------ 主循环入口 ------------------------------

    /**
     * @brief 状态机主循环入口，**必须由主循环反复调用**
     *
     * 本函数内部可能执行阻塞动作（进入 AVOID 时一次跑完七步），但绝不会
     * 在中断上下文里等节拍，因此不会死锁。
     */
    void working() noexcept
    {
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

    /// 当前状态（调试用）
    [[nodiscard]] inline State get_state() const noexcept { return state; }

protected:
    //-------------------------------- common ---------------------------------

    /// 进入正常行驶：持续前进 + 装载起步稳定等待
    virtual void common_init() noexcept
    {
        car.drive(DRIVE_SPEED_CM_S, DRIVE_SPEED_CM_S); // 非阻塞，一直跑
        __common_armed = false;
    }

    // 非避障时的执行函数：只轮询超声，不驱动电机
    virtual void common_working() noexcept
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

    virtual void common_exit() noexcept
    {
        car.stop(); // 刹车
    }

private:
    //--------------------------------- scan ----------------------------------

    /// 一个扫描点：写入 dist_datas 的下标、舵机角度、到位后等待的毫秒数
    struct ScanPoint
    {
        uint8_t index;
        uint8_t angle;
        uint16_t wait_ms;
    };

    /// 0/45/135/180 度；中位（下标 2）在 common_working() 里已经测过，跳过
    static constexpr std::array<ScanPoint, 4> SCAN_POINTS = {{
        {0, 10, 1200},  // 0 度：多等一会，减弱互补滤波影响
        {1, 45, 850},  // 45 度
        {3, 135, 850}, // 135 度
        {4, 180, 850}, // 180 度
    }};

    /// 进入扫描点：摆舵机并装载等待
    void scan_enter_point(uint8_t __idx) noexcept
    {
        steering_engine.set_degree(SCAN_POINTS[__idx].angle);
        delayer.start_ms(SCAN_POINTS[__idx].wait_ms);
    }

    void scan_init() noexcept
    {
        __step = 0;
        scan_enter_point(__step);
    }

    // 扫描期间只轮询舵机是否到位，到位就取一个点，然后切下一个角度
    void scan_working() noexcept
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

    void scan_exit() noexcept
    {
        steering_engine.set_degree(CENTER_DEGREE); // 回正
    }

    //--------------------------------- avoid ---------------------------------

    /**
     * @brief 绕内侧轮转向
     *
     * 「内侧轮」= 转弯方向那一侧的车轮：左转绕左轮、右转绕右轮。
     * 内轮停转、外轮走半径 = 轮距 的圆弧，因此转完后**车心也会被带出一段位移**，
     * 沿当时航向约 6.0 cm、朝弯外约 2.5 cm（45°、轮距 17 cm 时），
     * 这是绕轮转向与原地转向的本质区别。
     *
     * @warning Car::pivot_turn 的**转向方向由 speed_diff_cm_s 的符号决定**，
     *          它的 degrees 参数内部取绝对值、只管角度大小。两者必须同号，
     *          否则四次转向会全部朝同一个方向（曾经踩过这个坑）。
     *
     * @param degrees 目标转角（度），正值左转、负值右转
     */
    void turn(double degrees) noexcept
    {
        const bool left = degrees > 0.0;
        // 左转（正）绕左轮、速度取正；右转（负）绕右轮、速度取负 —— 都是绕内侧轮
        car.pivot_turn(left ? Car::Pivot::LEFT : Car::Pivot::RIGHT,
                       left ? DRIVE_SPEED_CM_S : -DRIVE_SPEED_CM_S,
                       degrees);
    }

    /**
     * @brief 进入绕障：一次性执行七步阻塞动作
     *
     * 全程约 (D + 73.5) / 25 秒。返回时车已停下、航向已回正。
     *
     * @note 四次转向都是绕内侧轮。横向分量两两相消，因此仍然回到原航线；
     *       但每次转向本身还带出约 6 cm 的前进量，整体比原地转向多走 24 cm。
     */
    void avoid_init() noexcept
    {
        // 判断障碍物在左还是右：左侧更近 => 障碍在左 => 向右绕（负角度）
        auto right_dist = (dist_datas[0] + dist_datas[1] + dist_datas[2]) / 3.0; // 中位数据会略微受到右侧滤波影响，故算入
        auto left_dist = (dist_datas[3] + dist_datas[4]) / 2.0;
        this->obstacle_on_left = left_dist < right_dist;

        // 转向符号：障碍在左则先右转（负），障碍在右则先左转（正）
        const double dir = obstacle_on_left ? -1.0 : 1.0;

        // 穿越距离：实测前方距离（米 → cm）+ 余量
        const double forward_cm = dist_datas[2] * 100.0 + DIST_MARGIN_CM;

        // 转 → 跑 35 → 转 → 跑 D → 转 → 跑 35 → 转
        turn(dir * TURN_DEG);                                  // 1 驶离障碍物一侧
        car.forward_distance(DRIVE_SPEED_CM_S, SIDE_SHIFT_CM); // 2 偏航
        turn(-dir * TURN_DEG);                                 // 3 摆回原航向
        car.forward_distance(DRIVE_SPEED_CM_S, forward_cm);    // 4 沿偏移车道越过障碍
        turn(-dir * TURN_DEG);                                 // 5 再摆向另一侧
        car.forward_distance(DRIVE_SPEED_CM_S, SIDE_SHIFT_CM); // 6 回航
        turn(dir * TURN_DEG);                                  // 7 回正
    }

    // 七步已在 avoid_init() 里一次性跑完，这里只负责切回 COMMON
    void avoid_working() noexcept
    {
        transition_to(State::COMMON);
    }

    void avoid_exit() noexcept
    {
        car.stop();
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END
