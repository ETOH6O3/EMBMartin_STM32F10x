# EMBMartin 的 CMake 构建

本文说明如何用 CMake 编译本库，以及**为什么这里不推荐 vcpkg**。

---

## 一、这套 CMake 做什么、不做什么

**做：** 把 `src/` 下的 24 个源文件编译成静态库 `libEMBMartin.a`，
并导出一个可被 `find_package` 消费的 CMake 包。

**不做：** 不碰 Keil 工程。`example/*/project.uvprojx` 完全独立，
两套构建系统并存，互不干扰。

这样划分的理由：Keil 是你在用的日常开发环境，CMake 的价值在于
**CI、命令行批量编译、以及在非 Keil 工具链上复用这个库**。
让 CMake 去接管例程的固件链接，只会带来两套配置同步的负担。

---

## 二、快速开始

```powershell
# 最省事：用脚本（会自动挑一个能用的 cmake 和 ninja）
.\build-cmake.ps1

# 构建 + 安装到 ./install
.\build-cmake.ps1 -Install
```

手动方式：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armclang.cmake
cmake --build build
```

产物：`build/libEMBMartin.a`（约 124 KB，24 个目标文件）。

---

## 三、文件说明

| 文件 | 作用 |
|---|---|
| `CMakeLists.txt` | 定义库目标、头文件路径、宏、安装导出规则 |
| `cmake/toolchain-armclang.cmake` | 告诉 CMake「用哪个编译器、目标什么 CPU」 |
| `build-cmake.ps1` | 一键构建脚本 |

### 为什么需要单独的 toolchain 文件

CMake 默认假设你在编译**给本机运行**的程序。交叉编译必须显式声明目标平台，
这只能写在 toolchain 文件里（在 `project()` 之前生效），不能塞进 `CMakeLists.txt`。

关键几行：

```cmake
set(CMAKE_SYSTEM_NAME Generic)              # 裸机，没有操作系统
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)
set(CMAKE_C_COMPILER ".../armclang.exe")
set(CMAKE_C_COMPILER_TARGET arm-arm-none-eabi)   # 不加这个 armclang 会按 x86 编
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) # 裸机链接不出可执行文件
```

最后一行尤其重要：CMake 配置阶段会试着编译一个 hello-world 可执行文件来
验证编译器可用。裸机环境没有启动文件和链接脚本，这个测试必然失败，
于是 CMake 会误判「编译器坏了」。改成静态库就绕过了。

### 关于 CMP0123

CMake 会从 `CMAKE_SYSTEM_PROCESSOR` 自动推导 `-mcpu` 和 `-march`。
但 armclang 对二者并存会告警，且推导出的空 `-march=` 会直接报错：

```
armclang: error: unsupported argument '' to option '-march='
```

所以在 toolchain 里设 `cmake_policy(SET CMP0123 NEW)`，改为显式给出 `-mcpu`。

---

## 四、编译选项的取舍

| 选项 | 值 | 理由 |
|---|---|---|
| C++ 标准 | C++17 | 库的硬性要求（见 `EMBMartin.h` 注释） |
| C 标准 | C11 | 标准外设库需要 |
| `-fno-exceptions` | 开 | 裸机不展开异常表；代码中 0 处 `try`/`throw`/`catch` |
| `-fno-rtti` | 开 | 同上，0 处 `typeid`/`dynamic_cast` |

这两个 `-fno-*` 选项是 `PUBLIC` 的——它们影响 ABI，**使用本库的代码必须同样关闭**，
否则会出现难以定位的链接或运行时问题。

---

## 五、作为依赖使用

### 方式 A：源码内嵌（推荐，适合本仓库例程）

```cmake
add_subdirectory(path/to/EMBMartin_STM32F10x)
target_link_libraries(your_target PRIVATE EMBMartin::EMBMartin)
```

### 方式 B：先安装再 find_package

```powershell
.\build-cmake.ps1 -Install
```

```cmake
find_package(EMBMartin REQUIRED)
target_link_libraries(your_target PRIVATE EMBMartin::EMBMartin)
```

安装目录结构：

```
install/
├── include/
│   ├── EMBMartin/          # 本库头文件
│   ├── platform/           # stm32f10x.h、cdelay.h 等
│   ├── etl/                # ETL（随包安装）
│   └── my_etl_profile.h
└── lib/
    ├── libEMBMartin.a
    └── cmake/EMBMartin/    # 包配置
```

**为什么连 ETL 和 platform 一起装**：本库的公开头文件直接引用了
`stm32f10x.h`、`cdelay.h`、`etl/format.h` 等。如果只装 `include/EMBMartin`，
消费者一编译就会报 `'stm32f10x.h' file not found`。
这一点是实测发现的——最初的安装规则只装了本库头文件，`find_package`
能成功找到包，但编译立刻失败。

---

## 六、为什么不推荐用 vcpkg 管理这个库

结论：**vcpkg 在这里是负价值，用 CMake 就够了。**

### 1. vcpkg 没有裸机 ARM triplet

`vcpkg/triplets` 里所有 arm 目标都带操作系统：
`arm-android`、`arm-ios`、`arm-linux`、`arm-windows`……
没有 cortex-m / arm-none-eabi / Generic。裸机不是 vcpkg 的目标场景。

### 2. armclang 无法作为 vcpkg 的目标编译器

实测用自定义 triplet 尝试三次，都卡在编译器检测：

```
CMake Error: At least one of the variables CMAKE_SYSTEM_PROCESSOR
or CMAKE_SYSTEM_ARCH must be set for ARMClang
```

vcpkg 的 `detect_compiler` 只传 `-DCMAKE_SYSTEM_NAME=Generic`，
不传 processor/arch；而 CMake 的 ARMClang 模块强制要求二者之一。
这是 vcpkg 侧的设计缺口，配置层面绕不过去。

### 3. vcpkg 的 ETL 版本比本项目旧，且不兼容

| 来源 | 版本 | `etl::private_format::args_mask` |
|---|---|---|
| 本项目 `TPL/ETL`（submodule） | **20.49.0** | 有 |
| vcpkg 端口 `etl` | **20.48.0** | **无** |

`fmt_etl.h:102` 和 `:169` 用了它：

```cpp
return etl::vformat_to<OutputIt, etl::private_format::args_mask<Args...>::value>(...)
```

换成 vcpkg 的 20.48 后，`algorithm/PID.cpp`、`angle_filter.cpp`、
`app/balanced_car.cpp` 等直接编译失败：

```
fmt_etl.h:102:57: error: no member named 'args_mask' in namespace 'etl::private_format'
```

### 4. 概念上也反了

vcpkg 是给**消费者**管理第三方依赖的。你自己的库就在仓库里，
不需要「被安装」——`add_subdirectory()` 一行就够。
本项目唯一的第三方依赖是 ETL，header-only，已经是 submodule 了。

**如果将来要加别的第三方库**，那时再引入 vcpkg 管理那些依赖是合理的；
但本库自身不该走 vcpkg。

---

## 七、已知问题

### 本机 `C:\Program Files\CMake` 的 4.1.1 有问题

该版本在检测 armclang 时会崩溃（退出码 `0xC0000409`），
连最小 MSVC 工程也无法 configure。`cmake --version` 却正常，
所以**不能用 `--version` 判断 CMake 是否可用**。

`build-cmake.ps1` 已处理：它会用真实工具链跑一次最小 configure 来探测，
跳过坏的、选中可用的（本机是 vcpkg 自带的 4.4.0）。

建议修复系统 CMake（重装或升级到 4.4+）。

### `_syscalls.c` 在 CMake 下正常

之前用 `armclang -fsyntax-only` 单独检查时报
`redefinition of '__FILE'`，那是单文件预处理的误报。
走完整 CMake 构建（含 `-DUSE_STDPERIPH_DRIVER` 等宏）时无此问题，
已作为第 24 个目标文件正常编入静态库。

---

## 八、常用命令

```powershell
# 干净重建
.\build-cmake.ps1 -Clean

# 换芯片（例如 Cortex-M4）
cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armclang.cmake `
  -DMCU_CPU=cortex-m4 -DMCU_ARCH=armv7e-m `
  "-DMCU_FPU=-mfpu=fpv4-sp-d16 -mfloat-abi=hard"

# 换 Keil 安装位置
cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armclang.cmake `
  "-DKEIL_ARM_DIR=D:/Keil_v5/ARM/ARMCLANG/bin"

# 看完整编译命令
cmake --build build -v
```
