# 定长字符串 mstring.h

对应头文件：`EMBMartin_STM32F10x/EMBMartin/mstring.h`

本模块提供适用于单片机的**定长字符串** `String<N>` 及其配套容器，并提供 C++ `std::string`
风格与 Python `str` 风格的常用接口。

**不做任何动态内存分配，不抛出异常**。

> 头文件名为 `mstring.h` 而非 `string.h`：若命名为 `string.h`，在 ARM 等工具链下会遮蔽
> C 库的 `string.h`，使 `<cstring>` 无法再引入标准声明。

## 容量语义

容量语义与标准库一致：

| 成员 | 含义 |
| --- | --- |
| `size()` | 当前字符个数（不含结尾 `'\0'`） |
| `capacity()` | 字符个数上限，恒为 `N - 1` |
| `buffer_size()` | 内部缓冲区总字节数，恒为 `N` |

`String<N>` 的 `N` 是**内部缓冲区总字节数**，因此最多容纳 `N - 1` 个字符。
对象大小为 `N` 字节缓冲区加上长度字段与对齐填充，在 32 位目标上即 `N + 4` 字节：

```cpp
String<8>  a;   // 缓冲区 8 字节，最多 7 个字符
String<16> b;   // 缓冲区 16 字节，最多 15 个字符

a.capacity();      // 7
a.buffer_size();   // 8
```

`size() <= capacity()` 恒成立；内部恒保持以 `'\0'` 结尾。

## 溢出行为：覆写为错误标记

**任何会使 `size()` 超过 `capacity()` 的操作都不是截断，而是把整个内容覆写为溢出错误标记。**

标记格式为 `<可用字符数上限><<至少还需的字符数>`，例如 `"7<9"` 表示
`capacity()` 为 7 而本次操作至少还需要 9 个字符：

```cpp
String<8> s{"12345678"};   // 需要 9 个字符，超出 7
s.c_str();                 // "7<9"
s.size();                  // 3
```

这样设计的目的是：溢出结果**不可能**被误当成正常内容使用，显示到控制台时也能立即暴露，
而不是静默丢数据。

标记本身必须能在最小容量（`capacity()` 为 7 的 `String<8>`）内完整写出，因此默认不带前缀。
可通过预定义宏 `EMBMARTIN_STRING_ERROR_TAG` 添加前缀（例如 `"E:"`），
但需自行确认在最小容量的串里数字仍能完整显示。

对返回新串的接口（如 `substr`），溢出时**返回的串**是错误标记，源串不变；
对就地修改的接口（如 `append`、`insert`），**自身**被覆写为错误标记。

> 注意：`format` 系列接口在溢出时给出的第二个数字是“至少还需”的**下界**，
> 而非精确长度，因为格式化的完整长度无法预知。

## 基本示例

```cpp
#include "tools.h"

using namespace EMBMartin;
using namespace STM32;

int main()
{
    String<32> message;

    message = "temp=";
    message.append("25");               // 追加文本；追加整数请先格式化
    message.format("{} {:g}", "value", 3.14159f);

    console.showln(message);            // 可直接交给 OutStream
    console.println("len = {}", message.size());

    if (message.starts_with("temp"))
    {
        console.println("id = {}", message.substr(5, 2));
    }

    while (true) {}
}
```

---

## 类型别名

与 `std::basic_string` 同名成员：

| 别名 | 实际类型 |
| --- | --- |
| `value_type` | `char` |
| `traits_type` | `std::char_traits<char>` |
| `size_type` | `std::size_t` |
| `difference_type` | `std::ptrdiff_t` |
| `reference` / `const_reference` | `char &` / `const char &` |
| `pointer` / `const_pointer` | `char *` / `const char *` |
| `iterator` / `const_iterator` | `const char *` |
| `reverse_iterator` / `const_reverse_iterator` | `std::reverse_iterator<const char *>` |

`iterator` 是**只读指针**：内容只能经由成员函数修改，以保证 `'\0'` 结尾不变量。

`String<N>::npos` 为 `static constexpr size_type`，取值同 `std::string::npos`。

---

## 构造与赋值

| 接口 | 说明 |
| --- | --- |
| `String()` | 构造空串 |
| `String(const char *text)` | 从 C 字符串构造 |
| `template <size_t M> String(const char (&text)[M])` | 从字符数组（字面量）构造，长度取到首个 `'\0'` 。若存在溢出的可能，必须显式转换。 |
| `String(const std::string_view &text)` | 从字符串视图构造 |
| `template <size_t M> String(const String<M> &text)` | 从另一个定长字符串构造，容量不同亦可 |
| `template <typename TEXT> explicit String(TEXT &&text)` | 从其它“字符串类”类型构造，例如 `std::string` |
| `String(size_type count, char c)` | 构造 `count` 个字符 `c` |
| `String &operator=(...)` | 接受 C 字符串、字符数组、字符串视图及其它“字符串类”类型 |

```cpp
String<16> a;                        // 空串
String<16> b{"hello"};               // 4 个字符
String<16> c{std::string_view("hi")};
String<16> d{3, 'x'};                // "xxx"
String<16> e{b};                     // 跨容量复制
String<16> f{std::string("std")};    // 显式
```

## 容量

| 接口 | 说明 |
| --- | --- |
| `size_type size()` | 当前字符个数 |
| `size_type length()` | 同 `size()` |
| `size_type capacity()` | 字符个数上限，恒为 `N - 1` |
| `size_type max_size()` | 同 `capacity()` |
| `size_type buffer_size()` | 缓冲区总字节数，恒为 `N` |
| `bool empty()` | 是否为空串 |
| `void shrink_to_fit()` | 无操作，仅用于与标准库容器接口对齐 |

## 元素访问

| 接口 | 说明 |
| --- | --- |
| `const char &operator[](size_type index)` | 只读访问；`index == size()` 时返回结尾 `'\0'` |
| `const char &front()` | 首字符，空串返回结尾 `'\0'` |
| `const char &back()` | 末字符，空串返回结尾 `'\0'` |
| `const char *data()` | 只读裸指针，等价于 `c_str()` |
| `const char *c_str()` | 以 `'\0'` 结尾的只读裸指针 |
| `char *data_mutable()` | 可写裸指针，见下方说明 |
| `void commit()` | 重新按 `'\0'` 计算长度 |

`data_mutable()` 用于把缓冲区交给 C 接口（如 `sprintf`、`InStream::getline`）。
本类无法得知外部写了什么，因此约定：**写完后必须立即调用 `commit()`**，
或自行保证不破坏“以 `'\0'` 结尾”与 `size()` 的一致性：

```cpp
String<32> line;
console.getline(line.data_mutable());
line.commit();

char buffer[16];
std::sprintf(fmt.data_mutable(), "%d", 42);
fmt.commit();
```

## 迭代器

| 接口 | 说明 |
| --- | --- |
| `begin()` / `end()` | 正向迭代器，可用于范围 for |
| `cbegin()` / `cend()` | 同上 |
| `rbegin()` / `rend()` | 反向迭代器 |
| `crbegin()` / `crend()` | 同上 |

```cpp
for (char c : text) { console.show(c); }
```

## 视图与转换

| 接口 | 说明 |
| --- | --- |
| `std::string_view view()` | 长度为 `size()` 的内容视图 |
| `std::string_view buffer()` | 长度为 `N` 的整个缓冲区视图 |
| `operator std::string_view()` | 隐式转换，使既有流接口可直接使用 |

因为存在隐式转换，`console.show(text)`、`console.println("{}", text)` 等可以直接工作，
无需为本模块改动 `stream.h`。

---

## 修改

| 接口 | 说明 |
| --- | --- |
| `void clear()` | 清空内容 |
| `void resize(size_type count, char c = '\0')` | 调整字符个数，变长部分用 `c` 填充 |
| `void push_back(char c)` | 在末尾追加一个字符 |
| `void pop_back()` | 删除末尾字符，空串时为无操作 |
| `String &append(char c)` | 追加一个字符 |
| `String &append(size_type count, char c)` | 追加 `count` 个字符 `c` |
| `String &append(const char *text)` | 追加 C 字符串 |
| `template <size_t M> String &append(const char (&text)[M])` | 追加字符数组 |
| `String &append(const std::string_view &text)` | 追加字符串视图 |
| `template <size_t M> String &append(const String<M> &text)` | 追加另一个定长字符串 |
| `template <typename TEXT> String &append(TEXT &&text)` | 追加其它“字符串类”类型 |
| `String &operator+=(...)` | 接受字符、C 字符串、字符串视图、定长字符串 |
| `String &insert(size_type position, size_type count, char c)` | 在 `position` 处插入 `count` 个字符 `c` |
| `String &insert(size_type position, const char *text)` | 插入 C 字符串 |
| `template <size_t M> String &insert(size_type position, const char (&text)[M])` | 插入字符数组 |
| `String &insert(size_type position, const std::string_view &text)` | 插入字符串视图 |
| `template <size_t M> String &insert(size_type position, const String<M> &text)` | 插入定长字符串 |
| `iterator insert(const_iterator position, char c)` | 插入单个字符，返回插入位置 |
| `String &erase(size_type position = 0, size_type count = npos)` | 删除 `[position, position + count)` |
| `iterator erase(const_iterator position)` | 删除单个字符，返回该位置 |
| `String &replace(size_type position, size_type count, size_type count_replacement, char c)` | 用 `count_replacement` 个 `c` 替换区间 |
| `String &replace(size_type position, size_type count, const std::string_view &text)` | 用字符串视图替换区间 |
| `String &replace(size_type position, size_type count, const char *text)` | 用 C 字符串替换区间 |
| `template <size_t M> String &replace(size_type position, size_type count, const char (&text)[M])` | 用字符数组替换区间 |
| `template <size_t M> String &replace(size_type position, size_type count, const String<M> &text)` | 用定长字符串替换区间 |
| `size_type replace(const std::string_view &old_text, const std::string_view &new_text)` | 替换**全部** `old_text`，返回替换次数 |
| `void swap(String &other)` | 交换两个同型字符串的内容 |

```cpp
String<32> s{"abc"};
s.append("de");            // "abcde"
s.insert(2, "--");         // "ab--cde"
s.erase(2, 2);             // "abcde"
s.replace(1, 2, "ZZZ");    // "aZZZde"
s.replace("de", "!");      // 返回 1，结果 "aZZZ!"
```

## 比较

| 接口 | 说明 |
| --- | --- |
| `int compare(const std::string_view &other)` | 按字典序比较，返回 `-1` / `0` / `1` |
| `int compare(const char *other)` | 同上 |
| `template <size_t M> int compare(const String<M> &other)` | 同上 |
| `bool starts_with(const std::string_view &prefix)` | 是否以 `prefix` 开头 |
| `bool ends_with(const std::string_view &suffix)` | 是否以 `suffix` 结尾 |
| `bool contains(const std::string_view &needle)` | 是否包含 `needle` |

同时提供全套比较运算符：`==`、`!=`、`<`、`>`、`<=`、`>=`，
左右操作数可以是 `String<N>`、`std::string_view`、C 字符串，且两侧类型可不同容量：

```cpp
String<8>  a{"abc"};
String<32> b{"abc"};
a == b;                       // true
a == "abc";                   // true
"abc" == a;                   // true
a == std::string_view("abc");  // true
a < "abd";                    // true
```

## 查找

失败时一律返回 `String<N>::npos`（与 `std::string` 一致）：

| 接口 | 说明 |
| --- | --- |
| `size_type find(const std::string_view &needle, size_type position = 0)` | 由前向后查找子串 |
| `size_type find(char c, size_type position = 0)` | 由前向后查找字符 |
| `size_type find(const char *needle, size_type position = 0)` | 由前向后查找 C 字符串 |
| `size_type rfind(const std::string_view &needle, size_type position = npos)` | 由后向前查找子串 |
| `size_type rfind(char c, size_type position = npos)` | 由后向前查找字符 |
| `size_type find_first_of(const std::string_view &set, size_type position = 0)` | 集合中任一字符首次出现的位置 |
| `size_type find_first_not_of(const std::string_view &set, size_type position = 0)` | 不属于集合的字符首次出现的位置 |
| `size_type find_last_of(const std::string_view &set, size_type position = npos)` | 集合中任一字符末次出现的位置 |
| `size_type find_last_not_of(const std::string_view &set, size_type position = npos)` | 不属于集合的字符末次出现的位置 |
| `size_type count(const std::string_view &needle, size_type position = 0, bool overlap = false)` | 统计出现次数；`overlap` 为真时统计可重叠次数 |
| `size_type count(char c, size_type position = 0)` | 统计字符出现次数 |
| `String substr(size_type position = 0, size_type count = npos)` | 截取子串 |
| `size_type copy(char *destination, size_type count, size_type position = 0)` | 拷贝到外部缓冲区，返回拷贝的字符个数 |

```cpp
String<32> s{"abcabc"};
s.find("bc");            // 1
s.find("bc", 2);         // 4
s.find("zz") == s.npos;  // true
s.rfind("bc");           // 4
s.count("bc");           // 2

String<16> t{"aaaa"};
t.count("aa");           // 2（不重叠贪心匹配）
t.count("aa", 0, true);  // 3（可重叠）
```

`substr` 的 `position` 超出 `size()` 属于调用方错误：与 `std::string` 抛出 `out_of_range`
不同，此处返回内容为溢出错误标记的串，使该误用不会被静默忽略。

---

## 格式化

语法与 `OutStream::print` / `println` 完全相同，见 [STREAM.md](STREAM.md)。
依赖 `fmt.h` 的模板实例化，**程序存储器开销不可忽略**。

| 接口 | 说明 |
| --- | --- |
| `template <typename... ARGS> String &format(const char (&fmt)[M], ARGS &&...args)` | 格式串为字符串字面量 |
| `template <typename... ARGS> String &format(TEXT fmt, ARGS &&...args)` | 格式串为运行期 `const char *` |
| `template <typename... ARGS> String &assign_format(TEXT fmt, ARGS &&...args)` | 先清空再格式化写入 |

写入前内容即被视为空串，故**不会残留旧内容**。

```cpp
String<32> s;
s.format("value = {}", 42);          // "value = 42"
s.format("{} {:g}", "pi", 3.14159f);
s.assign_format("Hex: {:#06X}", 0x2A);

const char *runtime_fmt = "{}";
s.format(runtime_fmt, 7);            // 运行期格式串亦可
```

格式串非法时内容被覆写为 `"E:fmt"`；结果超出容量时被覆写为溢出错误标记。

> 运行期格式串会被截入栈上 `N` 字节缓冲区，超长部分丢弃。

> 精度说明：本库中 `{:.Nf}` 的 `N` 表示**有效数字位数**，而非小数位数。
> 例如 `{:.2f}` 对 `3.14159` 输出 `3.14`，`{:.3f}` 输出 `3.142`；
> 需要完整精度请用 `{}` 或 `{:g}`。详见 [STREAM.md](STREAM.md)。

---

## Python `str` 风格接口

### 大小写

| 接口 | 对应 Python |
| --- | --- |
| `String lower()` | `str.lower()` |
| `String upper()` | `str.upper()` |
| `bool isupper()` | `str.isupper()`，无字母时为假 |
| `bool islower()` | `str.islower()`，无字母时为假 |

### 裁剪

| 接口 | 对应 Python |
| --- | --- |
| `String strip()` | `str.strip()`，去除首尾空白 |
| `String strip(const std::string_view &characters)` | `str.strip(chars)` |
| `String lstrip()` | `str.lstrip()` |
| `String lstrip(const std::string_view &characters)` | `str.lstrip(chars)` |
| `String rstrip()` | `str.rstrip()` |
| `String rstrip(const std::string_view &characters)` | `str.rstrip(chars)` |

### 判定

| 接口 | 对应 Python |
| --- | --- |
| `bool isdigit()` | `str.isdigit()` |
| `bool isalpha()` | `str.isalpha()` |
| `bool isalnum()` | `str.isalnum()` |
| `bool isspace()` | `str.isspace()` |
| `bool has_digit()` | `any(c.isdigit() for c in s)` |

以上判定对**空串一律返回假**，与 Python 一致。

### 数字解析

| 接口 | 说明 |
| --- | --- |
| `FloatingResult to_float()` | 转换为浮点数 |
| `int atoi()` | 语义同 C 的 `atoi()`，返回 `int` |

`to_float()` 接受首尾空白、可选 `+`/`-`、可选小数点与指数部分，例如 `"  -1.5e-3  "`。
不接受 `inf` / `nan` / 十六进制浮点 / 数字分隔符。
内容非法或数值超出 `float` 范围时 `ok` 为假。

```cpp
const auto result = String<32>("  -1.5e-3  ").to_float();
if (result) { console.println("{}", result.value); }
```

### 拼接运算符

| 接口 | 说明 |
| --- | --- |
| `template <size_t N, size_t M> String<N + M> operator+(const String<N> &, const String<M> &)` | 结果缓冲区长度为两者之和 |
| `String<N + 1> operator+(const String<N> &, const std::string_view &)` | 与视图拼接 |
| `String<N + 1> operator+(const String<N> &, const char *)` | 与 C 字符串拼接 |
| `String<N + 1> operator+(const String<N> &, char)` | 与字符拼接 |

以上均有左右操作数对称的重载：

```cpp
String<8> a{"ab"};
String<8> b{"cd"};

String<16> s = a + b;        // "abcd"，容量 15
String<16> t = a + "Z";      // "abZ"
String<16> u = "Z" + a;      // "Zab"
String<16> v = a + '!';      // "ab!"
```

拼接结果类型是 `String<N + 1>`（或 `String<N + M>`），**不是** `String<N>`，
赋值给更大容量的串时请依赖隐式转换或显式构造。

---

## 配套容器

### `StringArray<N, M>`

`String<N>` 的定长数组，用于接收 `split` 等多值结果。`N` 为每个元素的缓冲区总字节数，
`M` 为元素个数上限。

| 接口 | 说明 |
| --- | --- |
| `static size_type capacity()` | 元素个数上限 `M` |
| `size_type size()` | 已压入的元素个数 |
| `bool empty()` | 是否为空 |
| `size_type overflow_count()` | 溢出次数，非 0 表示有元素未能压入 |
| `void clear()` | 清空 |
| `void push_back(const String<N> &element)` | 压入一个元素 |
| `void push_back(const std::string_view &element)` | 压入一个视图 |
| `reference operator[](size_type index)` | 访问元素 |
| `begin()` / `end()` / `cbegin()` / `cend()` | 迭代器，可用于范围 for |

### `StringViewArray<M>`

字符串切分结果视图：元素是指向源字符串内部的 `std::string_view`，**不持有内容**，
只要源字符串仍存活结果即可用，适合零拷贝地遍历字段。`M` 为元素个数上限。
接口与 `StringArray` 一致（`value_type` 为 `std::string_view`）。

压入超过上限的元素记为溢出：此时数组内容被**清空**（`size()` 为 0），
溢出次数由 `overflow_count()` 报告。这与 `String` 把内容覆写为错误标记的策略一致：
溢出结果一定不会被误当成正常结果使用。

### 切分与拼接（自由函数）

| 接口 | 对应 Python |
| --- | --- |
| `size_t split(std::string_view source, CONTAINER &container, const std::string_view &separator, int limit = -1)` | `str.split(sep, maxsplit)` |
| `size_t split_whitespace(std::string_view source, CONTAINER &container)` | 无参 `str.split()` |
| `template <size_t N, typename CONTAINER> String<N> join(const std::string_view &separator, const CONTAINER &container)` | `separator.join(container)` |
| `template <size_t N, typename CONTAINER> String<N> join(const char *separator, const CONTAINER &container)` | 同上，分隔符为 C 字符串 |

`container` 需支持 `clear()`、`push_back(std::string_view)` 与 `size()`，
例如 `StringViewArray<M>`；`limit` 为负表示不限制切分次数。
**相邻分隔符之间会产生空字段**（`"a,,b"` 切分为 3 个元素），与 Python 一致。

`join` 的容器元素需可构造 `std::string_view`，且必须显式给出结果容量：

```cpp
String<16> text{"a,bb,,ccc"};

StringViewArray<15> fields;                            // 元素上限取源串容量
const size_t n = split(text.view(), fields, ",");      // 4
// fields[0] = "a", fields[1] = "bb", fields[2] = "", fields[3] = "ccc"

for (const std::string_view field : fields)
{
    console.println("[{}]", field);
}

StringArray<8, 4> parts;
parts.push_back(String<8>("ab"));
parts.push_back(String<8>("cd"));
String<16> joined = join<16>(std::string_view("-"), parts);   // "ab-cd"

String<16> spaced{"  a   bb c  "};
StringViewArray<8> words;
split_whitespace(spaced.view(), words);                // 3 个字段，不含空字段
```

---

## 使用注意事项

1. **`String<N>` 的 `N` 是缓冲区总字节数**，可存字符数为 `N - 1`。
2. **溢出不会截断**，而是把内容覆写为 `"<容量><<至少还需>"` 形式（如 `"7<9"`），
   因此判断操作是否成功，可靠的做法是检查内容是否形如错误标记，或直接给足容量。
3. **迭代器只读**：修改内容必须经过成员函数，以保证 `'\0'` 结尾不变量。
4. **`substr` 越界不抛异常**，而是返回溢出错误标记。
5. **`format` 相关接口的程序存储器开销较大**，且依赖 `fmt.h`，请按需使用。
6. 本模块接口大量使用 `constexpr`，可在编译期构造与运算字符串；
   为此内部未使用 `std::memcpy` / `std::memmove`（它们不是常量表达式）。
