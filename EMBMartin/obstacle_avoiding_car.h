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
 * working() 每调用一次推进一步，**全类上下文中不允许出现任何阻塞等待**：
 *
 * - 电机动作一律走 Car 的 *_async() 非阻塞接口，由 Car::update() 每拍推进；
 * - 需要等待的场合一律用 Delayer 的 start() / done() 两段式，不用 delay()。
 *
 * 这一点是硬约束：若在 ISR 里等自己所在定时器的节拍，中断不能重入，
 * 节拍永远不推进，会直接死锁（早期版本就是这样挂住的）。
 *
 * 分工：
 *
 * - init() **只执行一次**，负责准备与启动第一步动作（不等待）；
 * - working() 每拍推进一步，**不阻塞**，只做轮询与判停。
 *
 * 驱动方式由调用方决定：
 *
 * - **中断驱动**：attach_to(TIMX)，此后 working() 每拍由定时器更新中断调用；
 * - **主循环驱动**：直接反复调用 working()（与循迹共享电机时用这种）。
 *
 * 两种方式只能选一种：同时用会让同一拍被推进两次。
 *
 * 注意：早期版本把七步写在 init() 里连续阻塞执行，在 ISR 里等自己所在定时器
 * 的节拍会直接死锁，因此本类所有动作都拆成了步进式。
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
    constexpr static inline double SIDE_SHIFT_CM = 3.0;

    /// 绕障时每次转向的角度（度）
    constexpr static inline double TURN_DEG = 40.0;

    /// 穿越障碍时在实测距离之上多走的余量（cm）
    constexpr static inline double DIST_MARGIN_CM = 0.0;

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

    Delayer delayer;                                      // 延时器（两段式，绝不阻塞）
    sys::InnerTimerManager::Handle __working_handle = {}; // 工作回调在定时器队列里的句柄

    bool obstacle_on_left = false; // 障碍物在左侧还是右侧
    uint8_t __step = 0;            // scan 状态机当前步进下标
    bool __common_armed = false;   // COMMON 状态的起步稳定等待是否已装载

    /// 绕障七步：一步要么是转向、要么是直行
    struct AvoidStep
    {
        bool is_turn; //!< true = 转向（绕内侧轮），false = 直行
        double param; //!< 转向：带符号的角度（度）；直行：距离（cm）
    };

    std::array<AvoidStep, 7> __plan = {}; //!< avoid_init() 按 dir / D 填好
    uint8_t __avoid_step = 0;             //!< 当前执行到第几步

public:
    /**
     * @brief 构造避障小车（**不注册任何中断**）
     *
     * @param _car 差速小车（提供 *_async / update / drive / stop）
     * @param _steering_engine 超声波转向舵机
     * @param _ultrasonic_sensor 超声波传感器
     * @param delay_TIMX 提供等待节拍（两段式 start/done）的定时器
     *
     * @note 构造期间不驱动电机：全局构造时定时器尚未建立，任何动作都会
     *       走退化路径。真正的起步由第一次 common_working() 完成。
     * @note working() 由谁驱动、何时驱动，由调用方决定：
     *       - 主循环驱动：直接反复调用 working()（与循迹共享电机时用这种）；
     *       - 中断驱动：调用 attach_to(TIMX) 把 working() 挂到定时器更新中断。
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

    virtual ~ObstacleAvoidingCar() noexcept { detach(); }

    ObstacleAvoidingCar(const ObstacleAvoidingCar &) = delete;
    ObstacleAvoidingCar &operator=(const ObstacleAvoidingCar &) = delete;

    // ------------------------------ 中断挂载 ------------------------------

    /**
     * @brief 把 working() 挂到指定定时器的更新中断，此后由中断每拍推进
     *
     * 挂上以后**不要**再从主循环调用 working()，否则同一拍会被推进两次。
     *
     * @param TIMX 提供节拍的定时器，一般与编码器采样同一个
     * @return true 表示挂载成功
     *
     * @note 重复调用会先解除旧的挂载。
     */
    bool attach_to(TIM_TypeDef *TIMX) noexcept
    {
        detach();
        if (TIMX == nullptr)
            return false;
        __working_handle = sys::InnerTimerManager::add(TIMX, [this]()
                                                       { this->working(); });
        return __working_handle.valid();
    }

    /// 解除中断挂载（未挂载时无副作用）
    void detach() noexcept
    {
        if (__working_handle.valid())
        {
            sys::InnerTimerManager::remove(__working_handle);
            __working_handle = {};
        }
    }

    /// 是否已挂到中断上
    [[nodiscard]] inline bool is_attached() const noexcept { return __working_handle.valid(); }

    // ------------------------------ 状态机入口 ------------------------------

    /**
     * @brief 状态机入口，每调用一次推进一步，**全函数不阻塞**
     *
     * 电机动作走 Car 的 *_async() + update()，等待走 Delayer 的
     * start() / done() 两段式，因此在中断上下文里调用也安全。
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

protected:
    /**
     * @brief 启动一次绕内侧轮转向（非阻塞）
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
        car.pivot_turn_async(left ? Car::Pivot::LEFT : Car::Pivot::RIGHT,
                             left ? DRIVE_SPEED_CM_S : -DRIVE_SPEED_CM_S,
                             degrees);
    }

    /// 启动第 __idx 步（转向或直行），不等待
    void avoid_enter_step(uint8_t __idx) noexcept
    {
        const AvoidStep &step = __plan[__idx];
        if (step.is_turn)
            turn(step.param);
        else
            car.forward_distance_async(DRIVE_SPEED_CM_S, step.param);
    }

    /**
     * @brief 第 4 步「沿偏移车道越过障碍」在 __plan 里的下标
     *
     * 下标从 0 起，所以第 N 步对应 N - 1。第 4 步完成时 __avoid_step 已被
     * 推进到 4，因此用 `__avoid_step > CROSS_STEP_INDEX` 判断。
     */
    constexpr static inline uint8_t CROSS_STEP_INDEX = 3;

    /**
     * @brief 当前是否允许打断绕障
     *
     * **只允许在第 4 步「沿偏移车道越过障碍」完成之后打断**。
     * 在此之前车还贴着障碍物、或正横穿它：
     *
     * - 第 1～3 步只完成了「驶离 → 偏航 → 摆正」，车仍在障碍物侧方；
     * - 第 4 步尚未跑完时车正横穿障碍所在的车道，中途停车会直接卡在障碍路径上。
     *
     * 另外第 5～7 步只做「再摆 → 回航 → 回正」，此时车已越过障碍，
     * 即使停下也不会撞上，才允许打断。
     *
     * @return true 表示可以安全打断
     */
    [[nodiscard]] inline bool avoid_can_be_interrupted() const noexcept
    {
        return state == State::AVOID && __avoid_step > CROSS_STEP_INDEX;
    }

    /**
     * @brief 中止绕障并立即回到 COMMON（不可打断期间调用将被忽略）
     *
     * 供子类在 avoid_working() 的覆写里使用：判定需要提前结束时调用本函数，
     * 它会先检查 avoid_can_be_interrupted()，不满足条件就原样返回。
     *
     * @return true 表示已中止并回到 COMMON；false 表示当前不允许打断
     */
    bool interrupt_avoid() noexcept
    {
        if (!avoid_can_be_interrupted())
        {
            return false;
        }

        car.stop();
        transition_to(State::COMMON);
        return true;
    }

    /**
     * @brief 进入绕障：算好航向与穿越距离，把七步排成表，启动第 1 步
     *
     * 全程约 (D + 3.0 + 6.3) / 70 秒。七步由 avoid_working() 逐拍推进，
     * 本函数不等待、不停留。
     *
     * @note 四次转向都是绕内侧轮，横向分量两两相消，因此仍然回到原航线，
     *       但每次转向本身会带出约 6 cm 的前进量。
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

    /**
     * @brief 绕障的状态机钩子，每拍调用一次
     *
     * 默认行为：推进当前步，一步走完就进下一步，七步全部完成才回 COMMON。
     *
     * 子类若要「按需打断」，**必须**先过 avoid_can_be_interrupted() 这一关，
     * 或直接调用 interrupt_avoid()（内部已含该判断）。第 4 步「沿偏移车道
     * 越过障碍」完成之前一律不允许打断：那时车还在障碍物旁边或正横穿它，
     * 半途中止会把车停在障碍路径上，且航向尚未摆正。
     *
     * @note 基类的推进与转移逻辑不要漏掉，覆写后应调用基类版本或自行转移。
     */
    virtual void avoid_working() noexcept
    {
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

    void avoid_exit() noexcept
    {
        car.stop();
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END
