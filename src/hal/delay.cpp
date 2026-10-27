#include "EMBMartin/hal/delay.h"

EMBMARTIN_HAL_NAMESPACE_BEGIN

void Delayer::start(uint32_t cnt) noexcept
{
    duration = cnt;

    if (!is_ready())
    {
        running = false; // 节拍不会自增，跳过等待
        deadline = ticks;
        return;
    }

    deadline = ticks + cnt;
    running = true;
}

uint32_t Delayer::ticks_of_ms(double ms) const noexcept
{
    const double __tick_ms = tick_ms();
    if (__tick_ms <= 0.0)
    {
        return 0; // 定时器还没初始化，无法换算
    }
    return uint32_t(ms / __tick_ms + 0.999);
}

void Delayer::start_ms(double ms) noexcept
{
    start(ticks_of_ms(ms));
}

void Delayer::delay_ms(double ms) noexcept
{
    delay(ticks_of_ms(ms));
}

void Delayer::delay(uint32_t cnt) noexcept
{
    start(cnt);

    if (running)
    {
        while (!done())
        {
            EMBMARTIN_KEEP_CODE_ORDER;
        }
        return;
    }

    // 定时中断没跑起来，退化为阻塞延时
    const double __second = tick_second();
    if (__second > 0.0 && cnt > 0)
    {
        delay_ms(intmax_t(double(cnt) * __second * 1000.0 + 0.5));
    }
}

EMBMARTIN_HAL_NAMESPACE_END
