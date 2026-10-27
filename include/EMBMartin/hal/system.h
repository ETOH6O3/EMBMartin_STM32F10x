#pragma once

// hal 层总闸：只汇总纯外设驱动（hal/peripheral + hal/gpio + hal/delay）。
// 具体器件驱动（编码器 / 舵机 / 直流电机 / 计数传感器 / 串口终端 / OLED / 传感器）
// 属于 device 层，请包含 "EMBMartin/device/xxx.h" 或直接用 "EMBMartin/EMBMartin.h"。

#include "EMBMartin/hal/gpio.h"
#include "EMBMartin/hal/peripheral/EXTI.h"
#include "EMBMartin/hal/peripheral/I2C.h"
#include "EMBMartin/hal/peripheral/PWM.h"
#include "EMBMartin/hal/peripheral/TIM.h"
#include "EMBMartin/hal/peripheral/USART.h"
#include "EMBMartin/hal/delay.h"
#include "EMBMartin/hal/irq.h"
