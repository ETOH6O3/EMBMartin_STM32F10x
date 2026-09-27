# 串口调试工具说明

文件：`serial_tool.py`

该工具支持两种接收数据：

- 普通文本：按原始字节流和指定编码显示。
- 结构化数据包：以 `ESC RS`（十六进制 `1B 1E`）开头，由 Python 端解析并分发。

## 运行

```powershell
python serial_tool.py COM6 -b 9600
```

常用参数：

```text
-b, --baudrate RATE       波特率，默认 9600
-e, --encoding ENCODING   文本编码，默认 utf-8
-l, --list-ports          扫描可用串口
--log-path FILE=PATH      将指定 file 字段的 Log 写入文件
--show-struct             在控制台显示已接收的 Struct 原始字节
```

例如：

```powershell
python serial_tool.py COM6 -b 115200 -e utf-8 --log-path mpu=logs/mpu.log
```

`--log-path` 可以重复使用。路径为空时不建议配置；未配置的 `file` 字段不会落盘。

## 交互命令

```text
exit             退出程序
raw              切换普通文本的十六进制显示
clear            清空上方输出区
stats            查看收发字节数
help             显示帮助
send <hex>       发送十六进制字节，例如 send 1B 1E 00
```

终端界面分为两部分：上方是输出区，下方是输入区。`raw` 只影响普通文本，数据包仍然会被解析。

默认情况下 Struct 只更新 Python 全局变量，不额外打印。调试时可以启用：

```powershell
python serial_tool.py COM6 --show-struct
```

输出示例：

```text
[STRUCT] TestStruct -> latest: 12 34 7F
```

## 普通文本

不以 `ESC RS` 开头的数据进入普通文本路径：

1. 按 `--encoding` 指定的编码解码；
2. 正常模式直接显示；
3. `raw` 模式显示十六进制；
4. 普通文本中出现 `ESC` 时，工具会等待下一个字节，以判断它是否构成包头。

普通输入命令发送的字节保持原有行为：

```text
输入文本 + 换行符 `\\n` + NUL `\\0`
```

这部分不是结构化数据包协议。

## 公共包头

*参数包方式的发送端尚未施工，暂时较难使用*。

所有结构化包都以如下字节开头：

```text
ESC RS [PacketType]
```

其中：

```text
ESC         0x1B
RS          0x1E
PacketType  Console、Struct 或 Log
```

示例：

```text
1B 1E 5B 43 6F 6E 73 6F 6C 65 5D
```

对应字符串：

```text
ESC RS [Console]
```

## 文本字段转义

文本字段使用 `escape_text()` 和 `unescape_text()` 处理。转义规则如下：

| 原字节 | 传输形式 |
| -------- | ---------- |
| `\\`   | `\\\\`   |
| `[`    | `\\[`    |
| `]`    | `\\]`    |
| NUL    | `\\0`    |

只有文本字段使用转义。Struct 的原始字节不会转义。

示例文本：

```text
原文：a[b]\\c
传输字段：a\\[b\\]\\\\c
```

## Console 包

Console **没有 Style 字段**，不要发送 `[CStyle]` 或 `[FixedLength]`。

格式：

```text
ESC RS [Console][payload]
```

`payload` 是经过文本转义的文本。包在 payload 的未转义右方括号处结束，不再发送 NUL。

STM32 端示例：

```cpp
console.print("\\x1B\\x1E[Console][Please input two integers]");
```

不要使用 `println()` 发送协议包，因为它会在 payload 后额外发送换行。也不需要再调用：

```cpp
console.show('\\0');
```

完整字节结构：

```text
ESC RS [Console][文本 payload]
```

## Struct 包

格式：

```text
ESC RS [Struct][ClassName][VarName][raw payload]
```

字段说明：

| 字段          | 说明                                                 |
| --------------- | ------------------------------------------------------ |
| `ClassName`   | Python 全局空间中的`ctypes.Structure` 类名           |
| `VarName`     | 解析成功后写入全局空间的变量名                       |
| `raw payload` | 小端原始结构体字节，长度由类的`ctypes.sizeof()` 决定 |

协议流中不携带对齐信息，布局与对齐完全由接收端已注册的 `ctypes.Structure` 决定。发送端与接收端只要保证结构体定义一致即可：C++ 端需要 `_pack_ = 1` 效果时使用 `#pragma pack(1)` 等声明，Python 端对应地设置 `_pack_ = 1`。

Struct payload 外层有方括号，但内部字节按原样读取：

```text
[<sizeof(struct) 个原始字节>]
```

示例注册类：

```python
import ctypes


class MpuSample(ctypes.Structure):
    _fields_ = [
        ("ax", ctypes.c_int16),
        ("ay", ctypes.c_int16),
        ("az", ctypes.c_int16),
    ]
```

`MpuSample` 必须位于脚本的全局空间中，解码器会自动查找同名类。收到：

```text
[Struct][MpuSample][latest_mpu][原始字节]
```

解析成功后可以访问：

```python
latest_mpu.ax
latest_mpu.ay
latest_mpu.az
```

变量名允许覆盖已有全局变量，不产生覆盖警告。

由于未注册类无法推导 payload 长度，工具会报错并尝试丢弃到下一个 `ESC RS` 包头。正常通信应确保发送端使用已注册的结构体类名。

## Log 包

格式：

```text
ESC RS [Log][level][console][file][payload]
```

字段说明：

| 字段      | 有效值或说明                                       |
| ----------- | ---------------------------------------------------- |
| `level`   | `TRACE`、`DEBUG`、`INFO`、`WARN`、`ERROR`、`FATAL` |
| `console` | `0` 不输出控制台，`1` 输出控制台                   |
| `file`    | 文件标识，同时用于匹配`--log-path FILE=PATH`       |
| `payload` | 当前实现按文本字段处理并使用转义                   |

控制台输出格式：

```text
HH:MM:SS.mmm [LEVEL] [file] message
```

当 `file` 为空时，省略 `[file]`：

```text
HH:MM:SS.mmm [LEVEL] message
```

时间戳由 PC 端收到数据包时生成，不在线上传输。

示例：

```text
ESC RS [Log][INFO][1][mpu][sample ready]
```

启动命令：

```powershell
python serial_tool.py COM6 --log-path mpu=logs/mpu.log
```

该包会显示在控制台，并追加写入 `logs/mpu.log`。文件内容不会包含 ANSI 颜色控制符。

## 常见错误

### 看不到 Console 包

确认发送端使用的是：

```cpp
console.print("\\x1B\\x1E[Console][message]");
```

不要写成：

```cpp
console.println("\\e\\0x1E[Console]message");
```

后者的问题包括：`` 写法错误、缺少 payload 方括号，并且会额外发送换行。

### Struct 变量没有出现

检查：

- Python 全局空间是否定义了同名 `ctypes.Structure`；
- `ClassName` 大小写是否完全一致；
- 两端结构体的字段顺序、类型、对齐方式是否完全一致（尤其是否都用了 `_pack_ = 1` / `#pragma pack`）；
- 原始 payload 是否正好等于 `ctypes.sizeof()`；
- 字节序是否为小端。

### 普通文本显示异常

检查 `-e` 编码参数。例如：

```powershell
python serial_tool.py COM6 -e gbk
```

`raw` 可以用于查看尚未正确解码的普通字节。结构化包不会被 `raw` 禁用。
