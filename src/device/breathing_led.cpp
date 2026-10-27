#include "EMBMartin/device/breathing_led.h"
#include "cdelay.h" // breathe() 里直接调用 platform 的 Delay_ms

EMBMARTIN_DEVICE_NAMESPACE_BEGIN

BreathingLED::BreathingLED(TIM_TypeDef *TIMX, int CHn, uint8_t TIMx_REMAP,
                 bool driven_level, int period_s) noexcept
        : hal::PWM(TIMX, CHn, TIMx_REMAP, driven_level ? 0 : 1000 /* 初期关闭 */, 1000, 10),
          __driven_level(driven_level), __period_s(period_s) {};

void BreathingLED::breathe(int times) noexcept
{
    for (int t = 0; t < times; t++)
    {
        if (__driven_level)
        {
            int i;
            for (i = 0; i <= 1000; i++)
            {
                this->set_duty(i);
                Delay_ms(__period_s * 1000 / 2 / 1000);
            }
            for (; i >= 0; i--)
            {
                this->set_duty(i);
                Delay_ms(__period_s * 1000 / 2 / 1000);
            }
        }
        else
        {
            int i;
            for (i = 1000; i >= 0; i--)
            {
                this->set_duty(i);
                Delay_ms(__period_s * 1000 / 2 / 1000);
            }
            for (; i <= 1000; i++)
            {
                this->set_duty(i);
                Delay_ms(__period_s * 1000 / 2 / 1000);
            }
        }
    }
}

EMBMARTIN_DEVICE_NAMESPACE_END
