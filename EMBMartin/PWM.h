#pragma once

#include <cmath>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"
#include "outpin.h"

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
    int __Duty_permillage;
    int __Freq_HZ;
    int __Reso_permillage;
    TIM_TypeDef *_TIMX;
    int __CHn;

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
        return __Duty_permillage;
    }
};


class SteeringEngine : public PWM
{
private:
    constexpr static int duty_min = 30;                          //!< 占空比最小值（左闭）
    constexpr static int duty_max = 120;                         //!< 占空比最大值（右开）
    constexpr static int delta_duty = duty_max - duty_min;       //!< 占空比上下界差值
    constexpr static int degree_min = 0;                         //!< 百分度最小值（左闭）
    constexpr static int degree_max = 200;                       //!< 百分度最大值（右开）
    constexpr static int delta_degree = degree_max - degree_min; //!< 百分度上下界差值

    int16_t __degree_gon; //!< 角度（格恩 / 百分度 / 0.9 度）

    inline static int16_t truncate_degree_gon(int16_t degree_gon) noexcept
    {
        // return std::abs(degree_gon) % degree_max + degree_min;
        return std::clamp<int>(degree_gon, degree_min, degree_max);
    }

public:
    inline SteeringEngine(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP = 0b00) noexcept
        : PWM(TIMX, CHn, TIMx_REMAP, duty_min, 50, 10), __degree_gon(0) {};

    /**
     * @brief 设置舵机角度（百分度）
     *
     * @param degree_gon 百分度（1 百分度 = 0.9 度）
     */
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
    DCMotorDriver(
        PWM _PWMA, OutPin _AIN1, OutPin _AIN2,
        OutPin STBY,
        PWM _PWMB, OutPin _BIN1, OutPin _BIN2) noexcept;

    DCMotorDriver(PWM _PWMA, OutPin _AIN1, OutPin _AIN2, OutPin STBY = null_pin /* 直接接到了 3V */) noexcept;

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
