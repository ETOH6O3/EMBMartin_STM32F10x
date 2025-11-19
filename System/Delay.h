#ifndef __DELAY_H
#define __DELAY_H

extern "C"
{
    volatile void Delay_us(uint32_t us);
    volatile void Delay_ms(uint32_t ms);
    volatile void Delay_s(uint32_t s);
}
#endif
