# ---------------------------------------------------------------------------
# EMBMartin — ARM Compiler 6 (armclang) 裸机工具链
# ---------------------------------------------------------------------------
# 用法：
#   cmake -S . -B build -G Ninja ^
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armclang.cmake
#
# 说明：
#   1. 编译器取自 Keil MDK 安装目录（ARM Compiler for Embedded 6.x）。
#      若你的 Keil 不在默认位置，用 -DKEIL_ARM_DIR=<路径> 覆盖。
#   2. 目标固定为 Cortex-M3 / Thumb，对应 STM32F103（本库的既定平台）。
#      换芯片时覆盖 -DMCU_CPU=cortex-m4 -DMCU_ARCH=armv7e-m 等。
#   3. 本文件只负责「怎么编译」，不管「编译什么」——那些在 CMakeLists.txt。
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)

# CMP0123：ARMClang 的 cpu/arch 选项由我们显式给出，不让 CMake 从
# CMAKE_SYSTEM_PROCESSOR 自动推导（自动推导会同时塞 -mcpu 和 -march，
# 而 armclang 对两者并存会告警，且空 -march 会直接报错）。
cmake_policy(SET CMP0123 NEW)

# ---------------------------------------------------------------- 目标平台
set(MCU_CPU "cortex-m3" CACHE STRING "armclang -mcpu 取值")
set(MCU_ARCH "armv7-m"  CACHE STRING "armclang -march 取值")
set(MCU_FPU  ""         CACHE STRING "FPU 选项，如 -mfpu=fpv4-sp-d16 -mfloat-abi=hard；Cortex-M3 留空")

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ${MCU_CPU})

# ---------------------------------------------------------------- 编译器位置
set(KEIL_ARM_DIR "C:/Keil_v5/ARM/ARMCLANG/bin" CACHE PATH "Keil ARM Compiler 6 的 bin 目录")

set(_armclang "${KEIL_ARM_DIR}/armclang.exe")
set(_armar    "${KEIL_ARM_DIR}/armar.exe")
set(_armlink  "${KEIL_ARM_DIR}/armlink.exe")
set(_fromelf  "${KEIL_ARM_DIR}/fromelf.exe")

foreach(_tool ${_armclang} ${_armar})
  if(NOT EXISTS "${_tool}")
    message(FATAL_ERROR
      "找不到 ${_tool}\n"
      "请用 -DKEIL_ARM_DIR=<Keil>/ARM/ARMCLANG/bin 指定实际路径。")
  endif()
endforeach()

set(CMAKE_C_COMPILER   "${_armclang}")
set(CMAKE_CXX_COMPILER "${_armclang}")
set(CMAKE_ASM_COMPILER "${_armclang}")

set(CMAKE_AR     "${_armar}")
set(CMAKE_LINKER "${_armlink}")
set(CMAKE_OBJCOPY "${_fromelf}")

# armclang 必须知道目标三元组，否则按宿主 x86 处理
set(CMAKE_C_COMPILER_TARGET   arm-arm-none-eabi)
set(CMAKE_CXX_COMPILER_TARGET arm-arm-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET arm-arm-none-eabi)

# 裸机环境无法链接出可执行文件来做「编译器自检」，
# 改用静态库作为 try_compile 目标。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ---------------------------------------------------------------- 编译选项
set(_mcu_flags "-mcpu=${MCU_CPU} -mthumb")
if(MCU_FPU)
  string(APPEND _mcu_flags " ${MCU_FPU}")
endif()

set(CMAKE_C_FLAGS_INIT   "${_mcu_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_mcu_flags}")
set(CMAKE_ASM_FLAGS_INIT "${_mcu_flags}")

# 裸机不链接标准库、不做依赖查找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
