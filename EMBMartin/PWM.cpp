#include "PWM.h"

#include <algorithm>
#include <cmath>

extern uint32_t SystemCoreClock;

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

PWM::PWM(
    TIM_TypeDef *__TIMX, int CHn, uint8_t TIMx_REMAP, const GPIOPin output_pin,
    int Duty_permillage, int Freq_HZ, int Reso_permillage) noexcept
    : _Duty_permillage(Duty_permillage), _Freq_HZ(Freq_HZ), _Reso_permillage(Reso_permillage),
      _TIMX(__TIMX), _CHn(CHn)

{
    SystemCoreClockUpdate();
    if (__TIMX != TIM1)
        RCC_APB1PeriphClockCmd(get_TIM_RCC_APB1Periph(__TIMX), ENABLE);
    else
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(output_pin.port), ENABLE);

    if (TIMx_REMAP) // 复用模式
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

        if (output_pin == PB4)
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
        else if ((output_pin == PA15) || (output_pin == PB3))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
        else if ((output_pin == PA13) || (output_pin == PA14))
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        if (__TIMX == TIM1)
        {
            if (TIMx_REMAP == 0b01)
                GPIO_PinRemapConfig(GPIO_PartialRemap_TIM1, ENABLE);
            else if (TIMx_REMAP == 0b11)
                GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);
        }
        else if (__TIMX == TIM2)
        {
            if (TIMx_REMAP == 0b01)
                GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
            else if (TIMx_REMAP == 0b10)
                GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM2, ENABLE);
            else if (TIMx_REMAP == 0b11)
                GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);
        }
        else if (__TIMX == TIM3)
        {
            if (TIMx_REMAP == 0b10)
                GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);
            else if (TIMx_REMAP == 0b11)
                GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);
        }
        else if (__TIMX == TIM4)
        {
            if (TIMx_REMAP == 0b01)
                GPIO_PinRemapConfig(GPIO_Remap_TIM4, ENABLE);
        }
    }

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Pin = output_pin.pin,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_AF_PP,
    };
    GPIO_Init(output_pin.port, &GPIO_InitStructure);

    TIM_InternalClockConfig(__TIMX);

    const uint16_t ARR_plus1(1000 / Reso_permillage);
    const uint16_t CCR(Duty_permillage * ARR_plus1 / 1000);
    const uint16_t PSC_plus1(SystemCoreClock / (Freq_HZ * ARR_plus1));

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {
        .TIM_Prescaler = uint16_t(PSC_plus1 - 1),
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = uint16_t(ARR_plus1 - 1),
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0};
    TIM_TimeBaseInit(__TIMX, &TIM_TimeBaseInitStructure);

    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = CCR;
    switch (_CHn)
    {
    case 1:
        TIM_OC1Init(__TIMX, &TIM_OCInitStructure);
        break;
    case 2:
        TIM_OC2Init(__TIMX, &TIM_OCInitStructure);
        break;
    case 3:
        TIM_OC3Init(__TIMX, &TIM_OCInitStructure);
        break;
    case 4:
        TIM_OC4Init(__TIMX, &TIM_OCInitStructure);
        break;
    default:
        break;
    }

    TIM_Cmd(__TIMX, ENABLE);
}

void PWM::set_duty(int Duty_permillage) noexcept
{
    // 限制在 0-1000 之间
    if (Duty_permillage < 0)
        Duty_permillage = 0;
    else if (Duty_permillage > 1000)
        Duty_permillage = 1000;

    this->_Duty_permillage = Duty_permillage;
    switch (this->_CHn)
    {
    case 1:
        TIM_SetCompare1(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    case 2:
        TIM_SetCompare2(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    case 3:
        TIM_SetCompare3(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    case 4:
        TIM_SetCompare4(_TIMX, Duty_permillage * (1000 / _Reso_permillage) / 1000);
        break;
    default:
        break;
    }
}

void BreathingLED::breathe(int times) noexcept
{
    for (int t = 0; t < times; t++)
    {
        if (_driven_level)
        {
            int i;
            for (i = 0; i <= 1000; i++)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
            for (; i >= 0; i--)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
        }
        else
        {
            int i;
            for (i = 1000; i >= 0; i--)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
            for (; i <= 1000; i++)
            {
                this->set_duty(i);
                Delay_ms(_period_s * 1000 / 2 / 1000);
            }
        }
    }
}

void DCMotorDriver::set_mode(Mode mode, uint8_t motor_index) noexcept
{
    OutPin *IN1;
    OutPin *IN2;
    PWM *PWm;
    if (motor_index == 0)
    {
        IN1 = &AIN1;
        IN2 = &AIN2;
        PWm = &PWMA;
    }
    else
    {
        IN1 = &BIN1;
        IN2 = &BIN2;
        PWm = &PWMB;
    }

    switch (mode)
    {
    case Mode::STOP:
        IN1->reset();
        IN2->reset();
        PWm->set_duty(0);
        break;
    case Mode::BRAKE:
        PWm->set_duty(0);
        break;
    case Mode::FORWARD:
        IN1->set();
        IN2->reset();
        break;
    case Mode::REVERSE:
        IN1->reset();
        IN2->set();
        break;
    default:
        break;
    }
}

void DCMotorDriver::set_speed(int16_t speed_permillage, uint8_t motor_index) noexcept
{
    speed_permillage = std::clamp<int16_t>(speed_permillage, -990, 990);
    PWM *PWm;
    if (motor_index == 0)
    {
        current_left_speed_permillage = speed_permillage;
        PWm = &PWMA;
    }
    else
    {
        current_right_speed_permillage = speed_permillage;
        PWm = &PWMB;
    }

    if (speed_permillage < 0)
        set_mode(Mode::REVERSE, motor_index);
    else
        set_mode(Mode::FORWARD, motor_index);

    PWm->set_duty(std::abs(speed_permillage));
}

EMBMARTIN_STM32F10X_NAMESPACE_END
