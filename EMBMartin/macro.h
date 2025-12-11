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

#define EMBMARTIN_BLOCK_BEGIN(_enable) \
  if (_enable)                         \
  {
#define EMBMARTIN_BLOCK_END }

// 防止编译器为了适配~沟槽的~乱序多发而进行代码顺序调整
#define EMBMARTIN_KEEP_CODE_ORDER __asm__ volatile("" ::: "memory");

// ------------------------------------------调试宏------------------------------------------
#define EMBMARTIN_DEBUGING 1

#if EMBMARTIN_DEBUGING

#define EMBMARTIN_DEBUGING_SPECIFIER public

#ifndef EMBMARTIN_DEBUGING_EXTERN_CONSOLE // 允许通过预定义宏修改
#define EMBMARTIN_DEBUGING_EXTERN_CONSOLE extern EMBMartin::OutStream<128>& console;
#endif // EMBMARTIN_DEBUGING_EXTERN_OLED

/**
 * @brief 断言宏定义函数
 * @param _PRED 断言条件
 * @param _MSG  断言失败时显示的信息
 * @param _CONSOLE_POINTER 指向 OutStream 或其子类对象的指针，用于显示断言失败信息
 */
#define EMBMARTIN_ASSERT(_PRED, _MSG, _CONSOLE_POINTER)                                       \
  do                                                                                          \
  {                                                                                           \
    if (!(_PRED))                                                                             \
    {                                                                                         \
      (_CONSOLE_POINTER)->show("ASSERTION FAILED:", EMBMARTIN_MACRO_TOSTRING(_PRED), (_MSG)); \
      while (1)                                                                               \
      {                                                                                       \
        EMBMARTIN_KEEP_CODE_ORDER;                                                            \
      }                                                                                       \
    }                                                                                         \
  } while (0)

#else

#define EMBMARTIN_DEBUGING_SPECIFIER private

#ifndef EMBMARTIN_DEBUGING_EXTERN_CONSOLE
#define EMBMARTIN_DEBUGING_EXTERN_CONSOLE
#else                                    // EMBMARTIN_DEBUGING_EXTERN_OLED
#undef EMBMARTIN_DEBUGING_EXTERN_CONSOLE // 失能预定义宏
#define EMBMARTIN_DEBUGING_EXTERN_CONSOLE
#endif // EMBMARTIN_DEBUGING_EXTERN_OLED

#define EMBMARTIN_ASSERT(_PRED, _MSG, _OLED_POINTER) ((void)0)

#endif // EMBMARTIN_DEBUGING

// ------------------------------------------名字空间--------------------------------------------------

#define EMBMARTIN_NAMESPACE_BEGIN \
  namespace EMBMartin             \
  {
#define EMBMARTIN_NAMESPACE_END }

#define EMBMARTIN_STM32F10X_NAMESPACE_BEGIN \
  EMBMARTIN_NAMESPACE_BEGIN                 \
  namespace STM32                           \
  {
#define EMBMARTIN_STM32F10X_NAMESPACE_END \
  EMBMARTIN_NAMESPACE_END                 \
  }

#define EMBMARTIN_OLED_NAMESPACE_BEGIN \
  EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

#define EMBMARTIN_OLED_NAMESPACE_END \
  EMBMARTIN_STM32F10X_NAMESPACE_END

#define EMBMARTIN_SYS_NAMESPACE_BEGIN \
  EMBMARTIN_STM32F10X_NAMESPACE_BEGIN \
  namespace sys                       \
  {
#define EMBMARTIN_SYS_NAMESPACE_END \
  EMBMARTIN_STM32F10X_NAMESPACE_END \
  }

#define EMBMARTIN_BALANCED_CAR_NAMESPACE_BEGIN \
  EMBMARTIN_STM32F10X_NAMESPACE_BEGIN
#define EMBMARTIN_BALANCED_CAR_NAMESPACE_END \
  EMBMARTIN_STM32F10X_NAMESPACE_END

// ------------------------------------------版本控制宏--------------------------------------------------
#define EMBMARTIN_USING_OLD_OLED_VERSION 0
#define EMBMARTIN_ENCODING_UTF8 1 
// ------------------------------------------其它--------------------------------------------------

#ifndef STM32_DEVICE_HEADER             // 未使用预定义宏
#define STM32_DEVICE_HEADER stm32f10x.h // 修改这个与使用预定义宏等效
#endif                                  // STM32_DEVICE_HEADER

#endif // EMBMARTIN_MACRO_H