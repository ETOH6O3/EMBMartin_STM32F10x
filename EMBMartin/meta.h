/**
  ******************************************************************************
  * @file    functions.h
  * @author  孙鸣淼
  * @brief   这里存放一些嵌入式开发中常用且不依赖于平台的模板元编程工具
  ******************************************************************************
  * @attention  
  * 1. 至少需要的 C++ 标准： C++17
  * 2. 建议的编译器环境： ARM compiler 6 或更高版本
  ******************************************************************************
  */
#ifndef EMBMARTIN_METAPROGRAMING
#define	EMBMARTIN_METAPROGRAMING

#include <limits>
#include <utility>
#include <type_traits>

#include"macro.h"

EMBMARTIN_BEGIN


template <typename T>
struct should_move : std::conditional_t < (sizeof(T) > sizeof(void*)), std::true_type, std::false_type > {};

// 判断类型 T 是否应该通过移动语义传递，如果 T 的大小大于指针大小则返回 true，否则返回 false
template <typename T>
bool should_move_v = should_move<T>::value;

EMBMARTIN_END

#endif // ! EMBMARTIN_METAPROGRAMING

