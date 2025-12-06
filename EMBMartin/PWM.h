#ifndef EMBMARTIN_PWM_H
#define EMBMARTIN_PWM_H

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN



/**
 * @brief PWM 类，用于生成 PWM 信号控制设备接口
 *
 * 该类封装了 STM32 定时器的 PWM 功能，可以方便地配置和控制 PWM 输出。
 * 支持设置 PWM 频率、占空比和分辨率，并可动态调整占空比。
 *
 * @note
 * 1. 输出引脚是根据定时器和通道自动确定的，无需手动指定
 * 2. Reso_permillage 参数建议使用能被 1000 整除的值，以保证计算精度
 * 3. 占空比范围为 0-100 的整数百分比
 *
 */
class PWM
{
private:
    int _Duty_permillage;
    int _Freq_HZ;
    int _Reso_permillage;
    TIM_TypeDef *_TIMX;
    int _CHn;

    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP, GPIOPin output_pin /* 只能是规定的复用引脚 */,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept;

public:
    /**
     * @brief PWM 构造函数
     * @param TIMX 定时器外设指针
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     * @param Duty_permillage 初始占空比（0-1000），默认 0%
     * @param Freq_HZ PWM 频率（Hz），默认 1kHz
     * @param Reso_permillage PWM 分辨率（千分之一），默认 10‰
     *
     * @note 有一些重映像会导致构造函数自动关闭调试端口，具体查阅参考手册
     * @note Reso_permillage 建议能被 1000 整除，以确保计算准确性
     * @note output_pin 根据定时器、通道和重映像自动选择
     */
    PWM(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
        int Duty_permillage = 0, int Freq_HZ = 20000, int Reso_permillage = 10 /* 建议能被 1000 除尽 */
        ) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, _GEN_TIM_CH_TO_GPIOPin_REMAP[TIMx_REMAP][Get_TIM_Index(TIMX)][CHn - 1],
              Duty_permillage, Freq_HZ, Reso_permillage) {};
    /**
     * @brief 设置 PWM 占空比
     * @param Duty_permillage 占空比（0-100）
     *
     * @note 占空比为千分比值，0 表示始终低电平，100 表示始终高电平
     * @note 该函数会根据设定的分辨率自动计算并设置比较寄存器值
     */
    void set_duty(int Duty_permillage) noexcept;
    inline auto get_duty() const noexcept
    {
        return _Duty_permillage;
    }
};

/**
 * @brief 呼吸灯类，用于实现呼吸灯效果
 *
 */
class BreathingLED : public PWM
{
private:
    bool _driven_level;
    int _period_s;

public:
    /**
     * @brief 构造一个呼吸灯对象
     *
     * 呼吸灯是基于 PWM 实现的，通过周期性地改变 LED 亮度来模拟 "呼吸" 效果。
     *
     * @param TIMX 定时器外设指针，用于产生 PWM 信号
     * @param CHn 定时器通道号（1-4）
     * @param TIMx_REMAP 定时器重映像选择（0-3），默认 0（未重映像）
     *                   - 0b00: 无重映像
     *                   - 0b01: 部分重映像 01
     *                   - 0b10: 部分重映像 10
     *                   - 0b11: 完全重映像
     * @param driven_level 驱动电平，false 表示低电平点亮 LED，true 表示高电平点亮 LED
     * @param period_s 呼吸周期（秒），即从暗到亮再到暗的完整周期时间
     *
     * @note 呼吸灯初始化时会自动设置 PWM 频率为 1kHz，分辨率为 10‰
     */
    BreathingLED(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00,
                 bool driven_level = false, int period_s = 5) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, driven_level ? 0 : 1000 /* 初期关闭 */, 1000, 10),
          _driven_level(driven_level), _period_s(period_s) {};

    /**
     * @brief 执行呼吸灯效果
     *
     * 控制 LED 按照设定的周期执行呼吸效果，即亮度从暗逐渐变亮，
     * 再从亮逐渐变暗的过程。
     *
     * @param times 呼吸次数，默认为 1 次
     *
     * @note 该函数是阻塞式的，会持续执行完指定次数的呼吸效果
     * @note 呼吸周期由构造函数中设置的_period_s 决定
     */
    void breathe(int times = 1) noexcept;
};

class SteeringEngine : public PWM
{
private:
    const static int duty_min = 30;                          //!< 占空比最小值（左闭）
    const static int duty_max = 130;                         //!< 占空比最大值（右开）
    const static int delta_duty = duty_max - duty_min;       //!< 占空比上下界差值
    const static int degree_min = 0;                         //!< 百分度最小值（左闭）
    const static int degree_max = 200;                       //!< 百分度最大值（右开）
    const static int delta_degree = degree_max - degree_min; //!< 百分度上下界差值

    int16_t __degree_gon; //!< 角度（格恩 / 百分度 / 0.9 度）

    inline static int16_t truncate_degree_gon(int16_t degree_gon) noexcept
    {
        return std::abs(degree_gon) % degree_max + degree_min;
    }

public:
    inline SteeringEngine(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, duty_min, 50, 10), __degree_gon(0) {};

    inline void set_degree_gon(int16_t degree_gon) noexcept
    {
        this->__degree_gon = degree_gon;
        this->set_duty(duty_min + truncate_degree_gon(degree_gon) * delta_duty / delta_degree);
    }

    inline void set_degree(double degree) noexcept
    {
        set_degree_gon(int16_t(degree / 0.9));
    }

    inline void clockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon - int16_t(degree / 0.9));
    }

    inline void anticlockwise_rotate(double degree) noexcept
    {
        set_degree_gon(__degree_gon + int16_t(degree / 0.9));
    }
};

class DCMotorDriver
{
EMBMARTIN_DEBUGING_SPECIFIER:
    PWM PWMA;
    PWM PWMB;
    OutPin STBY;
    OutPin AIN1;
    OutPin BIN1;
    OutPin AIN2;
    OutPin BIN2;

    int16_t current_left_speed_permillage = 0;
    int16_t current_right_speed_permillage = 0;

public:
    enum class Mode
    {
        STOP,
        BRAKE,
        FORWARD,
        REVERSE
    };
    inline DCMotorDriver(
        PWM _PWMA, OutPin _AIN1, OutPin _AIN2,
        OutPin STBY,
        PWM _PWMB, OutPin _BIN1, OutPin _BIN2) noexcept
        : PWMA{_PWMA}, AIN1{_AIN1}, AIN2{_AIN2},
          STBY{STBY},
          PWMB{_PWMB}, BIN1{_BIN1}, BIN2{_BIN2}
    {
        work();
        set_mode(Mode::FORWARD,0);
        set_mode(Mode::FORWARD,1);
    };

    inline DCMotorDriver(PWM _PWMA, OutPin _AIN1, OutPin _AIN2, OutPin STBY = null_pin /* 直接接到了 3V */) noexcept
        : DCMotorDriver(_PWMA, _AIN1, _AIN2, STBY,
                        PWM{nullptr, 0, 0}, OutPin{null_pin}, OutPin{null_pin}) {};

    inline void standby() noexcept
    {
        STBY.reset();
    }
    inline void work() noexcept
    {
        STBY.set();
    }
    void set_mode(Mode mode, uint8_t motor_index = 0) noexcept;
    void set_speed(int16_t speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept;
    inline void add_speed(int16_t delta_speed_permillage /* 速度占最大速度千分比 */, uint8_t motor_index = 0) noexcept
    {
        set_speed(
            get_speed(motor_index) + delta_speed_permillage,
            motor_index);
    }
    inline int16_t get_speed(uint8_t motor_index = 0) noexcept
    {
        return motor_index == 0 ? current_left_speed_permillage : current_right_speed_permillage;
    }

};




EMBMARTIN_STM32F10X_NAMESPACE_END



#endif // EMBMARTIN_PWM_H