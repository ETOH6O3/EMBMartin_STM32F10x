/**
  ******************************************************************************
  * @file    functions.h
  * @author  孙鸣淼
  * @brief   这里集中存放一些本库通用的宏
  ******************************************************************************
  * @attention
  ******************************************************************************
  */

#ifndef EMBMARTIN_MACRO_H
#define EMBMARTIN_MACRO_H

// ------------------------------------------功能宏--------------------------------------------------

#define EMBMARTIN_MACRO_STRINGIFY(x) #x
#define EMBMARTIN_MACRO_TOSTRING(x) EMBMARTIN_MACRO_STRINGIFY(x)

#define EMBMARTIN_BLOCK_BEGIN(_enable) if(_enable) {
#define EMBMARTIN_BLOCK_END }

// 防止编译器为了适配~沟槽的~乱序多发而进行代码顺序调整
#define EMBMARTIN_KEEP_CODE_ORDER __asm__ volatile("" ::: "memory");

// ------------------------------------------名字空间--------------------------------------------------

#define EMBMARTIN_BEGIN namespace EMBMartin {
#define EMBMARTIN_END }

#define STM32_BEGIN EMBMARTIN_BEGIN \
namespace STM32 {
#define STM32_END EMBMARTIN_END \
}

#define OLED_BEGIN STM32_BEGIN \
namespace OLED {
#define OLED_END STM32_END \
}

#define I2C_BEGIN OLED_BEGIN \
namespace I2C {
#define I2C_END OLED_END \
}

#define SYS_BEGIN STM32_BEGIN \
namespace sys {
#define SYS_END STM32_END \
}



// ------------------------------------------其它--------------------------------------------------

#ifndef STM32_DEVICE_HEADER // 未使用预定义宏
#define STM32_DEVICE_HEADER stm32f10x.h // 修改这个与使用预定义宏等效
#endif // STM32_DEVICE_HEADER


#endif // EMBMARTIN_MACRO_H