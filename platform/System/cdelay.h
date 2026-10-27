/**
 ******************************************************************************
 * @file    cdelay.h
 * @author  孙鸣淼
 * @brief   基于 SysTick 的 C 语言阻塞延时（裸机教学用，不依赖本库）
 ******************************************************************************
 * @attention
 * 1. 本文件属于 platform 层，是纯 C 接口，不参与 EMBMartin 命名空间。
 * 2. 文件名前缀 c 表示「C 实现」，与库内 hal/delay.h 的 C++ Delayer 区分：
 *    - cdelay.h  : 阻塞延时，直接占用 SysTick
 *    - hal/delay.h: Delayer，用普通定时器分频计时，不占用 SysTick
 * 3. 函数名沿用 Delay_us / Delay_ms / Delay_s 以兼容既有例程。
 ******************************************************************************
 */

#ifndef CDELAY_H
#define CDELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void Delay_us(uint32_t us);
    void Delay_ms(uint32_t ms);
    void Delay_s(uint32_t s);

#ifdef __cplusplus
}
#endif

#endif // CDELAY_H
