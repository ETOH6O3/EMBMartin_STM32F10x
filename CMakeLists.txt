# ---------------------------------------------------------------------------
# EMBMartin — CMake 构建脚本
# ---------------------------------------------------------------------------
# 只做一件事：把 src/ 下的源码编译成静态库 libEMBMartin.a。
# Keil 工程（example/*/project.uvprojx）完全独立，两者并存互不干扰。
#
# 用法：
#   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armclang.cmake
#   cmake --build build
#
# 安装（可选）：
#   cmake --install build --prefix <目标目录>
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)

project(EMBMartin
  VERSION 0.9.3
  DESCRIPTION "EMBMartin 嵌入式开发库（STM32F10x + 标准外设库）"
  LANGUAGES C CXX)

# 默认按 Release 优化（对应 Keil 工程的 -O1）
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------- 路径布局
set(EMBMARTIN_ROOT      "${CMAKE_CURRENT_SOURCE_DIR}")
set(EMBMARTIN_INCLUDE   "${EMBMARTIN_ROOT}/include")
set(EMBMARTIN_PLATFORM  "${EMBMARTIN_ROOT}/platform")
set(EMBMARTIN_TPL       "${EMBMARTIN_ROOT}/TPL")

# ETL 是 git submodule，未初始化时要给出明确提示而不是一堆编译错误
if(NOT EXISTS "${EMBMARTIN_TPL}/ETL/include/etl/version.h")
  message(FATAL_ERROR
    "TPL/ETL 子模块未初始化。请先执行：\n"
    "    git submodule update --init --recursive\n")
endif()

# ---------------------------------------------------------------- 源文件
# 用 GLOB 而不是逐个列举：新增 .cpp 时不必改构建脚本。
# CONFIGURE_DEPENDS 让 ninja 在文件增删后自动重新配置。
file(GLOB_RECURSE EMBMARTIN_SOURCES CONFIGURE_DEPENDS
  "${EMBMARTIN_ROOT}/src/*.cpp"
  "${EMBMARTIN_ROOT}/src/*.c")

if(NOT EMBMARTIN_SOURCES)
  message(FATAL_ERROR "src/ 下没有找到任何源文件。")
endif()

# ---------------------------------------------------------------- 库目标
add_library(EMBMartin STATIC ${EMBMARTIN_SOURCES})
add_library(EMBMartin::EMBMartin ALIAS EMBMartin)

# ---------------------------------------------------------------- 头文件路径
# PUBLIC：所有依赖本库的目标自动继承这些路径。
# BUILD_INTERFACE / INSTALL_INTERFACE 成对出现——前者用于源码树内构建，
# 后者用于 find_package 之后，二者必须覆盖同一组路径，
# 否则会出现「源码树能编、安装后编不过」的不一致。
target_include_directories(EMBMartin PUBLIC
  $<BUILD_INTERFACE:${EMBMARTIN_INCLUDE}>
  $<BUILD_INTERFACE:${EMBMARTIN_PLATFORM}/Start>
  $<BUILD_INTERFACE:${EMBMARTIN_PLATFORM}/User>
  $<BUILD_INTERFACE:${EMBMARTIN_PLATFORM}/Library>
  $<BUILD_INTERFACE:${EMBMARTIN_PLATFORM}/System>
  $<BUILD_INTERFACE:${EMBMARTIN_TPL}/ETL/include>
  $<BUILD_INTERFACE:${EMBMARTIN_TPL}>            # my_etl_profile.h 在这里
  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:include/platform/Start>
  $<INSTALL_INTERFACE:include/platform/User>
  $<INSTALL_INTERFACE:include/platform/Library>
  $<INSTALL_INTERFACE:include/platform/System>)

# ---------------------------------------------------------------- 宏定义
# 与 example/*/project.uvprojx 的 <Define> 保持一致。
target_compile_definitions(EMBMartin PUBLIC
  USE_STDPERIPH_DRIVER
  STM32F10X_MD
  STM32_DEVICE_HEADER=stm32f10x.h
  ETL_PROFILE=my_etl_profile.h)

# ---------------------------------------------------------------- 语言标准
set_target_properties(EMBMartin PROPERTIES
  C_STANDARD          11
  C_STANDARD_REQUIRED ON
  CXX_STANDARD        17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS      OFF)

# ---------------------------------------------------------------- 编译选项
# 裸机环境不启用异常与 RTTI（代码中也没有任何使用）。
# 这两个选项必须 PUBLIC：它们影响 ABI，使用本库的代码必须同样关闭。
target_compile_options(EMBMartin PUBLIC
  -fno-exceptions
  -fno-rtti)

# ---------------------------------------------------------------- 安装导出
# 注意：本库的公开头文件直接引用了三类外部头文件——
#   stm32f10x.h 等（platform/Start）、cdelay.h（platform/System）、
#   etl/*.h（TPL/ETL）。因此安装时不能只装 include/，
#   否则 find_package 的消费者会因缺头文件而编译失败。
# 下面把这几组头文件一并装进 include/，形成一个自包含的包。
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS EMBMartin
  EXPORT  EMBMartinTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY "${EMBMARTIN_INCLUDE}/EMBMartin"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 平台头文件：保持与源码树一致的扁平布局，
# 因为代码里是 #include "cdelay.h" / "stm32f10x.h" 这样直接引用的。
foreach(_pdir Start User Library System)
  install(DIRECTORY "${EMBMARTIN_PLATFORM}/${_pdir}"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/platform"
    FILES_MATCHING PATTERN "*.h")
endforeach()

# ETL 与它的 profile：ETL 是 git submodule，版本需与源码树一致，
# 所以随包安装而不是交给消费者的 find_package 去碰运气。
install(DIRECTORY "${EMBMARTIN_TPL}/ETL/include/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")
install(FILES "${EMBMARTIN_TPL}/my_etl_profile.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT EMBMartinTargets
  NAMESPACE   EMBMartin::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EMBMartin)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/EMBMartinConfigVersion.cmake"
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

# 手写 Config 文件：它只需把 Targets 引进来。
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/EMBMartinConfig.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/EMBMartinTargets.cmake\")\n")

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/EMBMartinConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/EMBMartinConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EMBMartin)

# ---------------------------------------------------------------- 摘要
message(STATUS "")
message(STATUS "EMBMartin ${PROJECT_VERSION}")
message(STATUS "  编译器   : ${CMAKE_CXX_COMPILER}")
message(STATUS "  目标       : ${MCU_CPU} (${MCU_ARCH})")
list(LENGTH EMBMARTIN_SOURCES _n)
message(STATUS "  源文件数   : ${_n}")
message(STATUS "  构建类型   : ${CMAKE_BUILD_TYPE}")
message(STATUS "")
