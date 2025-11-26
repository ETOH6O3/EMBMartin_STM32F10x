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

#include <type_traits>

#include"macro.h"

EMBMARTIN_NAMESPACE_BEGIN


template <typename T>
struct should_move : std::conditional_t < (sizeof(T) > sizeof(void*)), std::true_type, std::false_type > {};

// 判断类型 T 是否应该通过移动语义传递，如果 T 的大小大于指针大小则返回 true，否则返回 false
template <typename T>
constexpr inline bool should_move_v = should_move<T>::value;

template<typename T, typename = void>
struct has_iterator : std::false_type {};

template<typename T>
struct has_iterator<T, std::void_t<
    typename T::iterator,
    typename T::const_iterator,
    decltype(std::declval<T>().begin()),
    decltype(std::declval<T>().end())
>> : std::true_type {};

template<typename T>
constexpr inline bool has_iterator_v = has_iterator<T>::value;

EMBMARTIN_NAMESPACE_END

#endif // ! EMBMARTIN_METAPROGRAMING

