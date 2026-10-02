/**
 ******************************************************************************
 * @file    tools.h
 * @author  孙鸣淼
 * @brief   这里存放一些嵌入式开发中常用的工具 （基于 STM32 标准外设库）
 ******************************************************************************
 * @attention
 * 0. 需要预定义宏 STM32_DEVICE_HEADER 为对应的 STM32 设备头文件名，如 stm32f10x.h （不得含引号）
 *    或者在 macro.h 中定义 STM32_DEVICE_HEADER 宏，这也是等价的
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#ifndef EMBMARTIN_TOOLS_H
#define EMBMARTIN_TOOLS_H

#include "basic_tools.h"
#include "mstring.h"
#include "outpin.h"
#include "inpin.h"
#include "EXTI.h"
#include "stream.h"
#include "TIM.h"
#include "PWM.h"
#include "OLED.h"
#include "I2C.h"
#include "PID.h"
#include "USART.h"
#include "ultrasonic_sensor.h"

#endif // EMBMARTIN_TOOLS_H