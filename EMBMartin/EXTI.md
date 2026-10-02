# EXTI 外部中断接口

对应头文件：`EMBMartin_STM32F10x/EMBMartin/EXTI.h`

## EXTIManager

`EXTIManager` 为一个 GPIO 引脚配置外部中断，并在中断触发时调用用户提供的回调。

### 构造

```cpp
EXTIManager manager(
    GPIOPin pin,
    EXTIManager::Callback callback,
    EXTITrigger_TypeDef trigger = EXTI_Trigger_Falling,
    uint8_t preemption_priority = 0,
    uint8_t sub_priority = 0,
    EXTIMode_TypeDef mode = EXTI_Mode_Interrupt);
```

示例：

```cpp
using namespace EMBMartin::STM32;

EXTIManager key(PA6, [] {
    // 处理按键中断
});
```

回调也可以捕获对象，用于转发到成员函数：

```cpp
class Controller
{
public:
    EXTIManager key{PA6, [this] { on_key_pressed(); }};

private:
    void on_key_pressed();
};
```

### 成员函数

- `set_operate(callback)`：替换中断回调。
- `software_trig()`：软件触发当前 EXTI line。
- `get_pin()`：返回当前使用的 `Inpin`，可用于读取输入电平。
- `instance(index)`：按 EXTI line 序号获取已注册的 `EXTIManager`；未注册或序号不在 `0` 到 `15` 时返回 `nullptr`。

对象销毁后，对应 EXTI line 不再调用该对象的回调。

## EXTIRotaryEncoder

`EXTIRotaryEncoder` 使用两个正交编码器相位输入进行方向计数。

### 构造

```cpp
EXTIRotaryEncoder encoder(
    GPIOPin pin_a,
    GPIOPin pin_b,
    uint8_t preemption_priority = 0,
    uint8_t sub_priority = 0,
    EXTIMode_TypeDef mode = EXTI_Mode_Interrupt);
```

示例：

```cpp
EXTIRotaryEncoder encoder{PA0, PA1};
```

### 成员函数

- `get_count()`：读取当前累计计数，不清零。
- `get_speed()`：读取自上次调用以来的计数，并将计数清零。
