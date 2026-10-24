# 旋转编码器驱动

对应头文件：`include/EMBMartin/device/rotary_encoder.h`

本文件提供两种旋转编码器实现：

- `EXTIRotaryEncoder` —— 双相外部中断型，占用 2 个 EXTI line，适合机械编码器；
- `RotaryEncoder` —— 硬件定时器编码器接口型（TIM CH1/CH2），不占中断。

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
