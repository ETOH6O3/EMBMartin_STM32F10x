/**
 ******************************************************************************
 * @file    functional.h
 * @author  孙鸣淼
 * @brief   EMBMartin 的函数式编程库
 *
 * 本头文件提供了三类核心能力：
 * 1. Python 风格的 Slice / Range 语义，适合对序列区间做轻量级描述；
 * 2. 广义 tuple 视图与操作，统一的广义元组接口
 * 3. 管道式编程接口 `pipe`，允许像 Unix 管道一样串联函数调用，并按需解包返回值。
 ******************************************************************************
 * @attention
 * 1. 至少需要的 C++ 标准： C++17（库中使用了 lambda 的显式模板参数列表，属 C++20 语法；
 *    ARM Compiler 6 在 C++17 模式下以扩展接受，MSVC 需启用 /std:c++20）
 * 2. 建议的编译器环境： ARM compiler 6 或更高版本; 或 MSVC 14+
 ******************************************************************************
 */

#pragma once

// 本文件为函数式编程组件的总入口，按依赖顺序汇总各拆分文件：
//
//   slice.h           Slice<T> 与随机访问迭代器（Python 风格半开区间）
//   range.h           Range<T> 别名与 range() 便捷工厂（依赖 slice.h）
//   tuple_view.h      广义 tuple 视图 + 索引元编程 + std::tuple_size/tuple_element 特化
//   tuple_utility.h   广义元组操作与高级解包（get / to_tuple / apply / unpack / split ...）
//   pipe.h            管道式编程接口 pipe
//
// 拆分只为减少单文件体积，对外接口与包含方式完全不变：包含 "functional.h" 即可。

#include "slice.h"
#include "range.h"
#include "tuple_view.h"
#include "tuple_utility.h"
#include "pipe.h"
