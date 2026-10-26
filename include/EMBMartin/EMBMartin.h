/**
 ******************************************************************************
 * @file    EMBMartin.h
 * @author  孙鸣淼
 * @brief   EMBMartin 嵌入式开发库总入口 （基于 STM32 标准外设库）
 ******************************************************************************
 * @attention
 * 0. 需要预定义宏 STM32_DEVICE_HEADER 为对应的 STM32 设备头文件名，如 stm32f10x.h （不得含引号）
 *    或者在 macro.h 中定义 STM32_DEVICE_HEADER 宏，这也是等价的
 * 1. 至少需要的 C++ 标准： C++17
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本
 ******************************************************************************
 */

#pragma once

// ------------------------------------------------ algorithm：算法
#include "EMBMartin/algorithm/PID.h"
#include "EMBMartin/algorithm/angle_filter.h"

// ------------------------------------------------ lang：平台无关基础设施
#include "EMBMartin/lang/mstring.h"
#include "EMBMartin/lang/stream.h"

// ------------------------------------------------ hal：纯外设驱动
#include "EMBMartin/hal/system.h"

// ------------------------------------------------ device：具体器件驱动

#include "EMBMartin/device/key.h"
#include "EMBMartin/device/sensor.h"
#include "EMBMartin/device/led.h"
#include "EMBMartin/device/buzzer.h"
#include "EMBMartin/device/rotary_encoder.h"
#include "EMBMartin/device/counter_sensor.h"
#include "EMBMartin/device/steering_engine.h"
#include "EMBMartin/device/dc_motor_driver.h"
#include "EMBMartin/device/console.h"
#include "EMBMartin/device/breathing_led.h"
#include "EMBMartin/device/OLED.h"
#include "EMBMartin/device/mpu6050.h"
#include "EMBMartin/device/ultrasonic_sensor.h"

