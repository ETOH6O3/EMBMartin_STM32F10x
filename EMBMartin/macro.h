/**
 ******************************************************************************
 * @file    macro.h
 * @author  孙鸣淼
 * @brief   这里集中存放一些本库通用的宏
 ******************************************************************************
 * @attention
 ******************************************************************************
 */

#ifndef EMBMARTIN_MACRO_H
#define EMBMARTIN_MACRO_H

 // ------------------------------------------未分类功能--------------------------------------------------

#define EMBMARTIN_MACRO_STRINGIFY(x) #x
#define EMBMARTIN_MACRO_TOSTRING(x) EMBMARTIN_MACRO_STRINGIFY(x)

#define EMBMARTIN_BLOCK_BEGIN(_enable) \
  if (_enable)                         \
  {
#define EMBMARTIN_BLOCK_END }

// 防止编译器为了适配~沟槽的~乱序多发而进行代码顺序调整
#define EMBMARTIN_KEEP_CODE_ORDER __asm__ volatile("" ::: "memory");

// if consteval
#if defined(_HAS_CXX23) && (_HAS_CXX23 == 1)
#define EMBMARTIN_IF_CONSTEVAL if consteval
#define EMBMARTIN_STATIC_ASSERT_WITH_VAR(_PRED, _MSG) void(0);
#else
#if defined(__GNUC__) || defined(__clang__)
#define EMBMARTIN_IF_CONSTEVAL if (__builtin_is_constant_evaluated())
#elif defined(_MSC_VER)
#define EMBMARTIN_IF_CONSTEVAL if (0)
#else
#define EMBMARTIN_IF_CONSTEVAL               \
  do                                         \
  {                                          \
    static_assert(0, "compiler not support") \
  } while (0);                               \
  if (0)
#endif // defined(__GNUC__) || defined(__clang__)
#define EMBMARTIN_STATIC_ASSERT_WITH_VAR(_PRED, _MSG)
#endif // defined(__HAS_CXX20) && (__HAS_CXX20 == 1)

// ------------------------------------------调试宏------------------------------------------
#define EMBMARTIN_DEBUGING 1

#if EMBMARTIN_DEBUGING

#define EMBMARTIN_DEBUGING_SPECIFIER public

#ifndef EMBMARTIN_DEBUGING_EXTERN_CONSOLE // 允许通过预定义宏修改
#define EMBMARTIN_DEBUGING_EXTERN_CONSOLE extern EMBMartin::OutStream<128> &console;
#endif // EMBMARTIN_DEBUGING_EXTERN_OLED

/**
 * @brief 断言宏定义函数
 * @param _PRED 断言条件
 * @param _MSG  断言失败时显示的信息
 * @param _CONSOLE_POINTER 指向 OutStream 或其子类对象的指针，用于显示断言失败信息
 */
#define EMBMARTIN_ASSERT(_PRED, _MSG, _CONSOLE_POINTER)                                         \
  do                                                                                            \
  {                                                                                             \
    EMBMARTIN_IF_CONSTEVAL                                                                      \
    {                                                                                           \
      EMBMARTIN_STATIC_ASSERT_WITH_VAR((_PRED), _MSG)                                         \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
      if (!(_PRED))                                                                             \
      {                                                                                         \
        (_CONSOLE_POINTER)->show("ASSERTION FAILED:", EMBMARTIN_MACRO_TOSTRING(_PRED), (_MSG)); \
        while (1)                                                                               \
        {                                                                                       \
          EMBMARTIN_KEEP_CODE_ORDER;                                                            \
        }                                                                                       \
      }                                                                                         \
    }                                                                                           \
  } while (0)

 /**
  * @brief 非阻塞式断言宏定义函数，可用于常量表达式函数
  * @param _PRED 断言条件
  * @param _MSG  断言失败时显示的信息
  * @param _CONSOLE_POINTER 指向 OutStream 或其子类对象的指针，用于显示断言失败信息
  */
#define EMBMARTIN_NON_BLOCKING_ASSERT(_PRED, _MSG, _CONSOLE_POINTER)                            \
  do                                                                                            \
  {                                                                                             \
    EMBMARTIN_IF_CONSTEVAL                                                                      \
    {                                                                                           \
      EMBMARTIN_STATIC_ASSERT_WITH_VAR(_PRED, _MSG)                                         \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
      if (!(_PRED))                                                                             \
      {                                                                                         \
        (_CONSOLE_POINTER)->show("ASSERTION FAILED:", EMBMARTIN_MACRO_TOSTRING(_PRED), (_MSG)); \
      }                                                                                         \
    }                                                                                           \
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
#define EMBMARTIN_NON_BLOCKING_ASSERT(_PRED, _MSG, _CONSOLE_POINTER) ((void)0)

#endif // EMBMARTIN_DEBUGING

// ------------------------------------------名字空间--------------------------------------------------

// 细节命名空间, 嵌套在其它命名空间中
#define EMBMARTIN_DETAIL_NAMESPACE_BEGIN \
  namespace detail                       \
  {
#define EMBMARTIN_DETAIL_NAMESPACE_END \
  }

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

// ------------------------------------------版本控制--------------------------------------------------
#define EMBMARTIN_USING_OLD_OLED_VERSION 0
#define EMBMARTIN_ENCODING_UTF8 1

// ------------------------------------------其它--------------------------------------------------

#ifndef STM32_DEVICE_HEADER             // 未使用预定义宏
#define STM32_DEVICE_HEADER stm32f10x.h // 修改这个与使用预定义宏等效
#endif                                  // STM32_DEVICE_HEADER

#endif // EMBMARTIN_MACRO_H