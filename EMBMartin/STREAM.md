# Stream 流操作接口

对应头文件：`EMBMartin_STM32F10x/EMBMartin/stream.h`

该头文件提供三类抽象类接口：

- `OutStream`：统一的输出接口，用于只读终端
- `InStream`：统一的输入接口，用于只写终端
- `IOStream`：同时支持输入和输出的接口，用于交互式终端

**这些都是抽象类，不可实例化!!!** 实际使用时，通常通过具体设备类创建对象，例如 `USBConsole`，再调用这些接口。
若想定制其他类型的输入输出设备，可以继承这些接口并实现纯虚函数。

## 基本示例

```cpp
#include "tools.h"

using namespace EMBMartin;
using namespace STM32;

USBConsole console{USART3, 0, 9600};

int main()
{
    console.println("system ready");
    console.println("value = {}", 42);

    while (true)
    {
        Delay_ms(1000);
    }
}
```

## OutStream

`OutStream` 用于向目标设备输出字符、数字、容器和格式化文本。

### 自定义输出设备

`OutStream` 负责格式化与输出缓冲区，派生类只需要实现「输出一个字符」这一个接口：

```cpp
class MyConsole : public EMBMartin::OutStream<64>
{
public:
    void output_char(char c) noexcept override
    {
        // 把字符 c 写到你的设备，例如串口、OLED、蓝牙……
    }
};
```

调用 `show`、`showln`、`showsep`、`printf`、`print`、`println` 时，文本先写入 `OutStream` 的内部缓冲区，再统一通过 `flush()` 逐字符调用 `output_char`。派生类不需要（也不应该）再访问缓冲区或重写 `flush()`。

- `output_char(char c)`：唯一需要实现的纯虚函数，输出单个字符。
- `flush()`：由 `OutStream` 提供，把缓冲区内容逐字符送出，并清空缓冲区；重复调用不会重复输出。

库内 `USBConsole`、`OLEDConsole` 都遵循这一约定：前者把字符交给 USART，后者把字符交给 OLED 显示驱动。

### `printf`

使用 C 风格格式字符串：

```cpp
console.printf("value = %d\\r\\n", 42);
console.printf("name = %s\\r\\n", "Martin");
```

没有额外参数时，格式字符串会按普通字符串输出：

```cpp
console.printf("hello\\r\\n");
```

### `show`

输出单个值或一组值，不自动添加换行：

```cpp
console.show('A');
console.show("hello");
console.show(123);
console.show(3.14f);
```

可以连续输出多个参数：

```cpp
console.show("x = ", 10, ", y = ", 20);
```

支持具有迭代器的容器：

```cpp
std::array<int, 3> values{1, 2, 3};
console.show(values);
```

### `showln`

输出一个或多个值，并在最后追加换行符：

```cpp
console.showln("x = ", 10);
console.showln("x = ", 10, ", y = ", 20);
```

### `showlr`

输出一个或多个值，并在最后追加回车符 `\\r`：

```cpp
console.showlr("progress: ", 50, "%");
```

### `showsep`

输出多个值，并在参数之间插入空格：

```cpp
console.showsep("position", 10, 20);
```

输出：

```text
position 10 20
```

### `print`

使用格式化语法输出文本。完整的格式说明符清单与两套后端差异见
[FMT.md](FMT.md)（fmt 组件用户接口手册）。

```cpp
console.print("value = {}", 42);
console.print("name = {:>10}, score = {:<6}", "Martin", 20);
console.print("Hex: {:#010x}", 0xfffff);
```

没有参数时，可以直接输出字符串：

```cpp
console.print("hello");
```

*不支持参数位置设定，不支持变量名传参。*

### `println`

使用格式化语法输出并追加换行符：

```cpp
console.println("value = {}", 42);
```

### 常用格式说明

> **格式化说明符的语法取决于 `fmt.h` 的底层实现**，由预定义宏 `EMBMARTIN_FMT_USE_ETL`
> （见 `macro.h`）选择，详见 `fmt.h` 头部的设计说明：
>
> - `EMBMARTIN_FMT_USE_ETL = 1`（**默认**）：底层是第三方库 ETL，语法同 C++20 `std::format`。
>   对齐只有 `<` `>` `^`（**没有** `=`），支持 `+` `-`（空格）`#` `0` `L`、`{}` 形式的嵌套
>   动态宽度/精度、`{0}` 手动参数索引；表示类型为 `s ? b B c d o x X a A e E f F g G p P`。
>   **不支持** `z`、数值分组（`,` / `_`）、`%`。
> - `EMBMARTIN_FMT_USE_ETL = 0`：底层是本库旧格式化核心，语法同 Python 的
>   format-specification mini-language（即本文档历史版本描述的那一套），额外支持 `z`、
>   `=` 对齐、数值分组与 `%`，但不支持 `{0}` 与嵌套 `{}`。
>
> **工程内调用点只使用下面这个公共子集**（标注了「仅…后端」的除外，其余两套后端一致）：

```text
{}             默认格式
{:d}           十进制整数
{:b}           二进制整数
{:o}           八进制整数
{:x}           小写十六进制
{:X}           大写十六进制
{:f}           定点浮点数
{:g}           常规浮点数
{:10}          宽度为 10
{:^10}         宽度为 10，居中
{:<10}         宽度为 10，左对齐
{:>10}         宽度为 10，右对齐
{:.3s}         字符串取前 3 个字符（仅 ETL 后端）
{:#x}          十六进制并显示前缀
{:,d}          十进制使用逗号分组（仅旧后端）
{:_x}          十六进制使用下划线分组（仅旧后端）
```

组合示例：

```cpp
console.println("{:^12}", "hello");
console.println("{:08X}", 0x2A);
```

> **浮点小数位**：ETL 后端的浮点格式化器忽略 `precision`（`{:.2f}`、`{:.0f}`、`{:.4f}`
> 输出完全相同，都是 6 位小数）——这是 ETL 上游未实现的功能，不是用法问题。
> 需要固定小数位时请用定点包装类型 `EMBMartin::fixed<N>`，两套后端输出一致：
>
> ```cpp
> console.println("Dist: {}m", EMBMartin::fixed<2>{distance});   // "Dist: 1.23m"
> console.println("Dist: {:8.3}m", EMBMartin::fixed<2>{distance});// 说明符 precision 覆盖 N
> ```

所有可解包类型（*C++17 及以上标准中可以使用结构化绑定的类型*）都有内置格式化配置。
例如二元类型的默认格式化输出 `x, y`：

```cpp
console.println("coor: {}", Coordinate<int>(0xff, 0b101010));   // "255, 42"
```

两套后端都额外支持广义 pair 的**元素级**说明符：`{:(x规格)}`、`{:[:x规格,,y规格]}`，
子说明符按所选后端的语法解释；旧后端另支持 `<...>` 括号与 `;` 分隔（`{::#X;:#b}`），
ETL 后端不支持这两种写法（`<` 会与 ETL 的对齐符冲突）。默认分隔符为 `, `。

自定义格式化的方式：ETL 后端特化 `etl::formatter<T>`，旧后端特化
`EMBMartin::legacy_fmt::formatter<T>`（两者 parse / format 签名不同）。

**格式化功能依赖大量的模板实例化，不加节制的使用会导致代码体积增大**。

## InStream

`InStream` 用于从输入设备读取一段输入。具体输入设备需要提供输入实现，使用者通常直接调用具体设备类提供的输入接口。

### `scanf`

C 风格格式化输入函数

#### 原始 C 风格形式

```cpp
int value = 0;
console.scanf("%d", &value);
console.showln("value = ", value);
```

多个参数：

```cpp
int x = 0;
int y = 0;
console.scanf("%d %d", &x, &y);
```

#### 引用参数重载

也可以直接按引用传入变量：

```cpp
int x = 0;
int y = 0;
console.scanf("%d %d", x, y);
```

### `scan`

C 风格的格式化输入, 但使用 python 风格的多返回值处理：元组。
`scan` 会格式化读取的输入流，返回一个编译时元组，适合配合结构化绑定使用：

```cpp
auto [x, y] = console.scan<int, int>("%d %d");

console.showln("x = ", x);
console.showln("y = ", y);
```

### `getline`

读取一行文本到字符数组：

```cpp
char line[128]{};
console.getline(line);
console.showln("input: ", line);
// 或：
auto fixed_str1 = console.getline(); // 返回自实现的定长字符串
```

## IOStream

`IOStream` 同时提供输入和输出能力。继承自 `InStream` 和 `OutStream`，可以同时使用输入和输出接口。

```cpp
char line[128]{};
console.input("input: ", line);
console.showln("you input: ", line);
// 或：
auto fixed_str2 = console.input("input: "); // 返回自实现的定长字符串
```
