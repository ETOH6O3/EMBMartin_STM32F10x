#include "EMBMartin/device/breathing_led.h"
#include "__Delay.h" // breathe() 里直接调用 platform 的 Delay_ms

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

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

EMBMARTIN_STM32F10X_NAMESPACE_END
