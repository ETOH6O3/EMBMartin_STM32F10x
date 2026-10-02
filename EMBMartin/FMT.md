# fmt 组件用户接口手册（`fmt.h`）

把文本和值拼成字符串。本文**只讲怎么用**；命名空间分层、两套后端的实现方式等设计说明见
[`fmt.h`](fmt.h) 头部注释。

---

## 速查

| 我想…… | 用 |
| --- | --- |
| 写进自己的 `char` 缓冲区 | `EMBMartin::format_to(buf, "...", args...)` |
| 最多写 n 个字符 | `EMBMartin::format_to_n(buf, n, "...", args...)` |
| 写进 `etl::istring` | `EMBMartin::format_to(istring, "...", args...)` |
| 只算要占多少字符 | `EMBMartin::formatted_size("...", args...)` |
| 写进 `String<N>` | `str.format("...", args...)` |
| 输出到串口 / OLED | `console.println("...", args...)` |
| 固定小数位 | `EMBMartin::fixed<2>{x}` |
| 让自己的类型可格式化 | 特化 `etl::formatter<T>` 或 `EMBMartin::legacy_fmt::formatter<T>` |

最小例子：

```cpp
#include "fmt.h"
#include "mstring.h"

EMBMartin::String<32> s;
s.format("x = {}, y = {}", 12, 7);        // "x = 12, y = 7"
s.assign_format("{:>8}", 3.5);            // "     3.5"
```

---

## 1. 两套底层实现与 `EMBMARTIN_FMT_USE_ETL`

`fmt.h` 有两套底层实现，由预定义宏 `EMBMARTIN_FMT_USE_ETL`（定义在 [`macro.h`](macro.h)）选择：

| 宏取值 | 底层 | 内联命名空间 |
| --- | --- | --- |
| `1`（默认） | 第三方库 ETL 的 format | `EMBMartin::etl_fmt` |
| `0` | 本库既有的旧格式化核心 | `EMBMartin::legacy_fmt` |

在 Keil 的 `Define` 栏里写 `EMBMARTIN_FMT_USE_ETL=0` 即可切换。

**要点：两套后端对外的接口完全相同**，也就是本文第 2 节的所有函数、第 2.6 / 2.7 节的成员函数，
写法都不随宏变化；变的只是**部分格式说明符的支持范围与个别语义**（见第 6 节）。

两套实现始终都存在于工程里，因此也可以显式指名后端（两者都提供第 2 节的同一套接口）：

```cpp
EMBMartin::etl_fmt::format_to(buf, "{}", 42);       // 强制走 ETL
EMBMartin::legacy_fmt::format_to(buf, "{}", 42);    // 强制走旧实现
EMBMartin::legacy_fmt::formatted_size("{}", 42);    // 同理
```

`legacy_fmt` 里另外保留了**旧接口**（`FormatError` 错误码、`FormatString<N>`、`FormatContext`），
那套写法必须写全 `EMBMartin::legacy_fmt::` 前缀，见第 7 节。

> 只要在一个工程（全部翻译单元）里统一这个宏即可；不要在不同文件里给不同的值。

---

## 2. 接口一览

以下的 `EMBMartin::` 前缀在 `using namespace EMBMartin;` 之后可以省略。

### 2.1 写入固定缓冲区

```cpp
template <typename OutputIt, typename... Args>
OutputIt format_to(OutputIt out, etl::string_view fmt, const Args &...args);

template <typename OutputIt, size_t N, typename... Args>
OutputIt format_to(OutputIt out, const char (&fmt)[N], const Args &...args);
```

返回**已推进的输出迭代器**。传给它的 `char*` 会从当前指向的位置开始写。

> ⚠️ **不会自动补 `'\0'`**（与 ETL 一致）。用 `char` 缓冲区时请自己补：
>
> ```cpp
> char buf[64];
> char *end = EMBMartin::format_to(buf, "{} + {} = {}", 1, 2, 3);
> *end = '\0';
> // buf == "1 + 2 = 3"
> ```

也可以是任意输出迭代器（`std::back_insert_iterator`、自定义迭代器等），因为返回值就是迭代器。

### 2.2 最多写 n 个字符

```cpp
template <typename OutputIt, typename... Args>
OutputIt format_to_n(OutputIt out, size_t n, etl::string_view fmt, const Args &...args);
```

最多写 `n` 个字符，返回**实际写入后**的迭代器（写满就是 `out + n`）。
想知道「本来需要多少字符」请配合 `formatted_size`：

```cpp
char buf[16];
char *end = format_to_n(buf, 5, "{}", 123456);
*end = '\0';                                  // buf == "12345"
size_t need = formatted_size("{}", 123456);   // need == 6
```

### 2.3 写入 `etl::istring`

```cpp
template <typename... Args>
etl::istring::iterator format_to(etl::istring &out, etl::string_view fmt, const Args &...args);
```

`etl::string<N>` / `etl::istring` 会被自动定长（超出容量按 ETL 契约截断）：

```cpp
etl::string<64> s;
EMBMartin::format_to(s, "x={}", 42);       // s == "x=42"
```

### 2.4 只计算长度

```cpp
template <typename... Args>
size_t formatted_size(etl::string_view fmt, const Args &...args);
```

```cpp
const size_t n = EMBMartin::formatted_size("{}", 12345);   // 5
```

### 2.5 运行期格式串

格式串是运行期值（`etl::string_view` 或 `const char *`）时，用上面的 `etl::string_view` 重载即可；
还有一个 `vformat_to` 名字专门给「明确是运行期串」的场合：

```cpp
template <typename OutputIt, typename... Args>
OutputIt vformat_to(OutputIt out, etl::string_view fmt, const Args &...args);
```

```cpp
etl::string_view rt = "rt={}";
EMBMartin::format_to(buf, rt, 7);      // 字符串视图
const char *rt2 = "p={}";
EMBMartin::format_to(buf, rt2, 8);     // C 字符串
```

> 字面量重载会把长度一并传给后端，因此在 ETL 的 C++20 模式下可以启用编译期格式串检查；
> 运行期串只能在运行期检查。

### 2.6 `String<N>::format` / `assign_format`（[`mstring.h`](mstring.h)）

```cpp
template <size_t M, typename... ARGS> String &format(const char (&fmt)[M], const ARGS &...args);
template <typename... ARGS, typename TEXT> String &format(TEXT fmt, const ARGS &...args);
template <typename... ARGS, typename TEXT> String &assign_format(TEXT fmt, const ARGS &...args);
```

- `TEXT` 可以是 `const char *`、`char *` 或 `etl::string_view`；
- 返回 `*this`，格式串非法由第 7 节的方式上报；
- 结果超出容量时**保留写下的部分**并置位 `truncated()`（用 `clear_truncated()` 清）；
- 每次调用都从空串开始写，不会残留旧内容。

```cpp
String<32> s;
s.format("v = {}", 42);        // "v = 42"
s.assign_format("{:>6}", 12);  // "    12"
if (s.truncated()) { /* 被截断了 */ }
```

### 2.7 `OutStream::print` / `println`（[`STREAM.md`](STREAM.md)）

```cpp
console.print("value = {}", 42);       // 不换行
console.println("value = {}", 42);     // 追加 '\n'
console.println("hello");              // 无参数：原样输出
```

`print` / `println` 的格式串同样支持字面量与 `etl::string_view` 两种形式。
它们由 [`mstring.md`](mstring.md) 里描述的 `OutStream` 缓冲区承接，缓冲区大小即
`OutStream<buffer_size>` 的模板参数。

### 2.8 类型别名

两套后端都提供下面这些名字（ETL 后端直接来自 ETL）：

| 名字 | 说明 |
| --- | --- |
| `formatter<T>` | 自定义类型的格式化器（见第 4 节） |
| `is_formattable<T>` / `is_formattable_v<T>` | 该类型是否可格式化 |
| `format_context<OutputIt>`、`basic_format_context` | 格式化上下文 |
| `format_parse_context`、`basic_format_parse_context` | 说明符解析上下文 |
| `format_string<Args...>`、`basic_format_string` | 编译期校验用的格式串包装（ETL 侧） |
| `format_args`、`format_arg`、`basic_format_arg(s)`、`make_format_args` | 参数存储（直通 ETL） |

---

## 3. 格式说明符

统一形式（`{}` 里冒号之后的部分）：

```text
[[fill]align][sign][#][0][width][.precision][type]
```

`{` `}` 要写字面量时用 `{{` `}}`。

### 3.1 两套后端都支持的子集（推荐）

| 写法 | 含义 | 例 |
| --- | --- | --- |
| `{}` | 默认表示 | `42`、`3.5`、`abc` |
| `{:d}` `{:b}` `{:o}` `{:x}` `{:X}` | 十进制 / 二进制 / 八进制 / 小写十六进制 / 大写十六进制 | `{:x}` 对 `0x3f4` → `3f4` |
| `{:#x}` `{:#X}` | 带 `0x` / `0X` 前缀的十六进制 | `0x3f4` |
| `{:c}` | 当字符输出 | `{:c}` 对 `67` → `C` |
| `{:s}` | 当字符串输出 | `{:s}` 对 `"abc"` → `abc` |
| `{:N}` | 最小字段宽度 N | `{:6}` |
| `{:<N}` `{:>N}` `{:^N}` | 左 / 右 / 居中（数字默认右对齐） | `{:>6}` 对 `12` → `12` |
| `{:*^N}` | 填充字符 + 对齐 | `**12**` |
| `{:0N}` | 零填充 | `{:06}` 对 `12` → `000012` |
| `{:+}` `{: }` | 强制显示正号 / 用空格代替正号 | `+12`、`12` |
| `{:.Ne}` 的 `e`/`E` | 科学计数法（**precision 语义两套不同**，见第 6 节） | |

> 仓库内的调用点只用这个子集，所以换后端不需要改格式串。

### 3.2 仅 ETL 后端支持

| 写法 | 含义 |
| --- | --- |
| `{:?}` | 调试转义：**字符**、**字符串**（`{:?}` 对 `bool` 不适用，见第 6 节） |
| `{:p}` / `{:P}` | 指针（形参需是 `const void *`） |
| `{:.Ns}` | 字符串截断到 N 个字符 |
| `{0}` `{1}` | 手动参数索引（`"{1} {0}"`） |
| `{:{}}` / `{:.{}f}` | 嵌套的**动态宽度 / 精度** |
| `B` `A` `a` `F` `G` 等 | 其余表示类型 |
| `L` | 本地化标志（ETL 解析但不使用） |

在旧后端用这些写法会**断言失败**，不会静默给出错的结果；唯一的例外是**指针实参**——旧后端没有
`const void *` 的格式化器，写出来是**编译期报错**（`arg not formattable!`），不是运行期断言。

### 3.3 仅旧后端支持

| 写法 | 含义 |
| --- | --- |
| `z` | 强制把 `-0.0` 转为 `+0.0` |
| `=` 对齐 | 在符号与数字之间填充（`{:=+6}`） |
| `,` / `_` 分组 | 数值分组：`{:,}` → `1,234,567`；`{:_x}` → `dead_beef` |
| `%` | 百分比：`{:.1%}` 对 `0.5` → `50.0%` |

### 3.4 浮点小数位：`EMBMartin::fixed<N>`

**这是本节最需要注意的一条**：ETL 后端**无法控制浮点的小数位数**——ETL 20.49.0 的浮点格式化
把小数位硬编码为 6 且从不读 precision，`{:.0f}`、`{:.2f}`、`{:.4f}` 的输出完全一样
（该缺陷在上游 master 仍未修复）。所以**不要用 `{:.Nf}` 表达小数位**，改用 `fixed<N>`：

```cpp
oled.println("Dist: {}m", EMBMartin::fixed<2>{distance});     // "Dist: 1.23m"
s.format("{} / {}", EMBMartin::fixed<2>{x}, EMBMartin::fixed<3>{y});
```

`fixed<N>` 是「按恰好 N 位小数输出」的包装类型，**两套后端、各编译器的输出逐字符一致**，
不受上述 ETL 缺陷和旧后端舍入差异的影响。

| 说明符 | 含义 |
| --- | --- |
| `[[fill]align]` | `<` `>` `^`，如 `{:*^8}` |
| `sign` | `+` / `-`（默认）/ 空格 |
| `0` | 零填充，插在符号与数字之间（`{:08}`） |
| `width` | 最小字段宽度 |
| `.precision` | **覆盖** `N`：`"{:8.3}"` → 宽度 8、3 位小数 |
| `f` / `F` | 定点；`F` 让 `nan`/`inf` 变成 `NAN`/`INF` |

约束：

- `N` 取 0 ~ 9；
- 只支持 `f` / `F`，给 `e` / `g` / `a` 会断言失败；
- `NaN` 不输出符号（`fixed<2>{std::numeric_limits<double>::quiet_NaN()}` → `nan`）；
  `±Inf` 保留符号（`-inf`）；
- `|值| × 10^N` 超出 `unsigned long long` 时按上限截断。

> ⚠️ `fixed` 与 `std::fixed`（I/O 操纵符）同名。若本翻译单元里同时有 `using namespace std;`
> 和 `using namespace EMBMartin;`，请写限定名 `EMBMartin::fixed<N>{...}`，否则在某些标准库
> （如 armclang 的 libc++）下会二义。

---

## 4. 让自己的类型可格式化

自定义点在后端各自的命名空间里，**签名不同**，所以要支持两套后端需要各写一份（用宏分开）。

### ETL 后端：特化 `etl::formatter<T>`

```cpp
struct Point { int x, y; };

namespace etl
{
    template <>
    struct formatter<Point>
    {
        format_parse_context::iterator parse(format_parse_context &ctx)
        {
            return ctx.begin();           // 说明符由 ETL 统一解析，这里通常不用做事
        }

        template <typename OutputIt>
        typename format_context<OutputIt>::iterator
        format(const Point &p, format_context<OutputIt> &ctx)
        {
            OutputIt out = ctx.out();
            out = etl::formatter<int>().format(p.x, ctx);
            ctx.advance_to(out);
            *out = ','; ++out; *out = ' '; ++out;
            ctx.advance_to(out);
            out = etl::formatter<int>().format(p.y, ctx);
            ctx.advance_to(out);
            return out;
        }
    };
}
```

用 `etl::formatter<元素类型>` 去格式化成员，就不用自己重写数字格式化。

### 旧后端：特化 `EMBMartin::legacy_fmt::formatter<T>`

```cpp
namespace EMBMartin { namespace legacy_fmt
{
    template <>
    struct formatter<Point, void>
    {
        // 返回 0 表示成功，负数是 FormatError 的值
        int parse(FormatParseContext &ctx)
        {
            return ctx.empty() ? 0 : static_cast<int>(FormatError::InvalidFormatSpec);
        }

        int format(const Point &p, FormatContext &ctx) const
        {
            if (!ctx.write_safe('(')) return static_cast<int>(FormatError::BufferOverflow);
            // …写 p.x / p.y…
            return static_cast<int>(FormatError::Success);
        }
    };
}}
```

写法与 ETL 差别较大（返回 `int` 错误码、参数是具体的 `FormatContext`）。**只有确实需要
换后端才写这一份**；两套后端的渲染结果应当保持一致。

> 已经在 ETL 里实现好的类型（整数、浮点、字符、字符串、指针、`bool` 等）不用迁移，直接用。

---

## 5. 广义 pair 的格式化

「广义 pair」指能结构化绑定出两个成员的类型：`std::pair`、`std::tuple<T, U>`、
[`Coordinate<T,…>`](coordinate.h)、以及任意二成员聚合（如 `struct { int a; bool b; }`）。
它们**开箱即可**用 `{}` 格式化，默认输出 `x, y`：

```cpp
s.format("{}", Coordinate<int>(1, 2));         // "1, 2"
```

元素级说明符（两套后端都支持这两种括号）：

```text
{:(x规格)}            两个元素都用 x规格
{:[:x规格,,y规格]}    分别指定
```

```cpp
s.format("{:(:#x)}", Coordinate<int>(255, 42));       // "(0xff, 0x2a)"
s.format("{:[:#x,,:#b]}", Coordinate<int>(255, 42));  // "[0xff, 0b101010]"
```

旧后端**额外**支持 `<...>` 括号与 `;` 分隔：`{:<:#x>}`、`{::#x;:#b}`
（ETL 后端不支持这两种，因为 `<` 在 ETL 里是对齐符）。

---

## 6. 两套后端的行为差异

换后端时**只有下面这些地方会不一样**；其余写法输出一致。

| 场景 | `EMBMARTIN_FMT_USE_ETL=1`（ETL） | `=0`（旧核心） |
| --- | --- | --- |
| `{:.Nf}` 浮点 | **忽略 N**，固定 6 位小数 | N = 小数点后位数 |
| `{:.Ne}` 浮点 | **忽略 N**，6 位 | N = 尾数小数位 |
| `{:g}` 浮点 | 固定 6 位小数（不去尾零） | 去尾零 |
| `{}` 浮点 | 去尾零，至少留 1 位：`1.0` → `"1.0"` | 去尾零：`1.0` → `"1"` |
| `{}` 布尔 | `"true"` / `"false"` | `"1"` / `"0"` |
| `{:d}` 布尔 | `"1"` | `"1"`（一致） |
| `{:?}` 布尔 | **输出空串**（ETL 的 `bool` 不支持 `?`，而它的报错在本工程被编译掉） | 断言失败 |
| 零实参且格式串含 `{{` | 处理转义：`"{{}}"` → `"{}"` | 原样输出：`"{{}}"` → `"{{}}"` |
| `{:.2f}` 传 **int** | `"42"` | `"42.00"` |
| `{:.Ns}` 字符串 | 截断到 N | 断言失败（不支持） |
| `{0}`、`{:?}`、`{:p}`、`{:{}}`、`z`、`=`、`,`、`_`、`%` | 见第 3.2 / 3.3 节 | 见第 3.2 / 3.3 节 |

另外，旧后端的浮点舍入用 `long double`，**被丢弃部分恰好以 5 开头**时末位可能随编译器不同
（例如 `{:.3e}` 对 `1234.5f` 可能是 `1.235e+003` 也可能是 `1.234e+003`）。
**需要小数位就用 `fixed<N>`**，它是确定性的。

> ⚠️ 上表里「输出空串」这一格是**静默错误**的典型：ETL 后端在遇到它不支持的说明符时，报错路径
> 被编译掉了（见第 7 节），于是什么都不输出而不是报错。写格式串时请只用第 3.1 节的公共子集，
> 或第 3.2 / 3.3 节里对应后端的语法。

---

## 7. 错误处理

**本组件自身的检查**（适配层、`fixed<N>`、广义 pair）在两套后端下行为一致：
非法格式串、无法满足的说明符**不返回错误码，而是断言**。上报点是宏 `EMBMARTIN_FMT_ASSERT`，
默认是 `assert`：

```cpp
// 想在格式化出错时走自己的处理逻辑，在包含 fmt.h 之前定义：
#define EMBMARTIN_FMT_ASSERT(_EXPR) my_fmt_error_handler(_EXPR)
#include "fmt.h"
```

会触发本组件断言的典型情况：

- 旧后端下使用了它不支持的写法（`{0}`、`{:{}}`、`{:.Ns}`、`{:?}`、`z`、`=`、`,`、`_`、`%` 等）；
- `fixed<N>` 配了 `f`/`F` 以外的表示类型，或带了 `#`、`z`、数值分组；
- 旧后端下单次格式化结果超过缓冲区上限（见第 8 节）。

> ⚠️ **ETL 后端还有一层 ETL 自己的运行时检查，而它在当前工程配置下是「空操作」。**
> 本工程的 ETL profile（[`TPL/my_etl_profile.h`](../TPL/my_etl_profile.h)）只定义了
> `ETL_NO_EXCEPTIONS`，没有定义 `ETL_DEBUG` / `ETL_LOG_ERRORS` / `ETL_USE_ASSERT_FUNCTION`，
> 于是 ETL 的 `ETL_ASSERT(b, e)` 展开成 `static_cast<void>(sizeof(b))`——**整条检查被编译掉**。
>
> 实测后果：把**旧语法**的格式串用在 ETL 后端，ETL 解析到不认识的位置后不会再报错，而是继续
> 向后推进解析指针，**越界读取并崩溃**（在 PC 上是访问违例，在 MCU 上通常是 HardFault 或乱码）。
> 例如下面这三条都会崩：
>
> ```cpp
> console.println("DEC: {:|^#10,d}", -42000);      // ',' 分组：旧语法
> console.println("Hex: {:|^#10_x}", 0xfffff);     // '_' 分组：旧语法
> console.println("f:   {:|^10_.2_g}", 3.1415);    // '_' 分组：旧语法
> ```
>
> 如果需要 ETL 的运行时检查（让上面这些变成干净的断言而不是越界），选一个：
>
> - 在 ETL profile 里加 `#define ETL_LOG_ERRORS`（走 `etl::error_handler`，可挂自定义处理器）；
> - 或加 `#define ETL_DEBUG`（退回 `assert`）；
> - 或用 C++20 模式编译（`ETL_USING_CPP20` 时 `etl::format_string` 会在**编译期**校验字面量格式串）。
>
> 注意这些只影响 ETL 内部的检查，不影响 `EMBMARTIN_FMT_ASSERT`。**最省事的做法是只用第 3.1 节的
> 公共子集**，或在玩旧语法时把 `EMBMARTIN_FMT_USE_ETL` 设为 `0`。

- `assert` 在 `NDEBUG` 下会被移除，发布构建里这些检查会消失；需要保留就覆盖 `EMBMARTIN_FMT_ASSERT`。

### 需要错误码的场合

如果不想断言、想要**可恢复的错误码**，请用旧接口（它不随宏变化，任何时候都可用，且只走旧实现）：

```cpp
#include "fmt.h"

EMBMartin::legacy_fmt::FormatString<5> fmt_str("x={}");   // 模板参数 = 字面量长度 + 1
char buf[32];
const int result = EMBMartin::legacy_fmt::format_to(buf, sizeof(buf), fmt_str, 42);
if (result < 0)
{
    const auto err = static_cast<EMBMartin::legacy_fmt::FormatError>(result);
    // 例如 BufferOverflow(-1) / InvalidFormatSpec(-2) / MismatchedBraces(-5) / TooMuchContext(-6)
}
else
{
    // 成功：buf 已经是 '\0' 结尾的 "x=42"
}
```

关于这套旧接口，有三点必须知道：

1. **返回值不是写入长度**。成功时它是 `0`（有占位符），或不含占位符时的剩余格式串长度；
   想知道长度请用 `String<N>::format` 或 `EMBMartin::formatted_size`。它只用来区分「成功 / 失败」。
2. 成功时缓冲区**会**以 `'\0'` 结尾（与 `format_to` 不同）；缓冲区不足时返回 `BufferOverflow`。
3. `FormatString<N>` 的模板参数必须**恰好**等于字面量长度 + 1（`"x={}"` → `FormatString<5>`），
   写大了编译不过。这是这套旧接口的历史包袱，也是新代码推荐用 `String<N>::format` 的原因之一。

`FormatError` 的取值：`Success`、`BufferOverflow`、`InvalidFormatSpec`、`TypeNotFormattable`、
`EmptyFormatString`、`MismatchedBraces`、`TooMuchContext`。

---

## 8. 已知限制与常见坑

| 坑 | 说明 |
| --- | --- |
| `format_to` 不补 `'\0'` | 用 `char` 缓冲区时记得 `*end = '\0';` |
| `format_to_n` 不告诉你「本应多长」 | 用 `formatted_size` 再算一次 |
| `{:.2f}` 在 ETL 后端无效 | 用 `fixed<N>`，见 3.4 |
| `fixed` 与 `std::fixed` 同名 | 有 `using namespace std;` 时写 `EMBMartin::fixed<N>` |
| 自定义类型在两套后端要各写一份 formatter | 见第 4 节；已有的内置类型不用管 |
| 旧后端单次结果不得超过 512 字节 | 含实参的格式化结果超过该值会断言；可用预定义宏 `EMBMARTIN_FMT_LEGACY_SCRATCH_SIZE` 调大（占用栈） |
| 旧后端不支持 `{0}` 与嵌套 `{}` | 需要这些就把宏设为 1 |
| 格式化的程序存储器 / 栈开销不小 | 按需使用；`fixed<N>` 比直接格式化 `double` 更省（不需要 C 库浮点） |

---

## 9. 相关文档

- [`mstring.md`](mstring.md) —— `String<N>`
- [`STREAM.md`](STREAM.md) —— `OutStream` / `InStream` 与 `print` / `println`
- [`fmt.h`](fmt.h) 头部注释 —— 双后端设计、命名空间分层、实现取舍
