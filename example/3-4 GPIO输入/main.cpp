
#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

// ------------------------------------------引脚作用说明--------------------------------------------------

// 蜂鸣器控制引脚：接 PA8，低电平驱动
Buzzer buzzer{PA8, false};

// 控制 8 个 LED 灯的引脚，低电平点亮
array<LED, 8> leds = {{
	LED{PA0, false},
	LED{PA1, false},
	LED{PA2, false},
	LED{PA3, false},
	LED{PA4, false},
	LED{PA5, false},
	LED{PA6, false},
	LED{PA7, false},
}};

// 控制循环亮灯方向的开关：PB4，低电平触发
Key shift_direction{PB4, false};

// 控制蜂鸣器开关的引脚：PB6，上拉输入
Sensor buzzer_enable{PB6};

int main()
{

	// ------------------------------------------GPIO--------------------------------------------------

	/***************************************************************************************************
	GPIO（General Purpose Input Output）通用输入输出口
	可配置为8种输入输出模式
	引脚电平：0V~3.3V，部分引脚可容忍5V
	输出模式下可控制端口输出高低电平，用以驱动LED、控制蜂鸣器、模拟通信协议输出时序等
	输入模式下可读取端口的高低电平或电压，用于读取按键输入、外接模块电平信号输入、ADC电压采集、模拟通信协议接收数据等
	****************************************************************************************************/

	/***************************************************************************************************
	输入模式：
	1. 上拉模式 无输入（引脚悬空）时默认输入为高电平
	2. 下拉模式 无输入（引脚悬空）时默认输入为低电平
	3. 浮空模式 无输入（引脚悬空）时默认输入为高组态
	4. 模拟输入 GPIO 无效， I/O 引脚直接接入内部ADC
	-. 输出模式 输入仍然有效，输入值就是输出值

	输出模式：
	1. 推挽模式 数据寄存器为 1 时输出高电平，数据寄存器为 0 时输出低电平
	2. 开漏模式 数据寄存器为 1 时输出高组态，数据寄存器为 0 时输出低电平
	3. 复用输出 由片上外设控制，同样有推挽模式和开漏模式
	*. 输入模式 恒定输出关闭，从 I/O 引脚读入信号
	****************************************************************************************************/

	// ------------------------------------------主逻辑--------------------------------------------------

	bool direction_right = true; // 亮灯方向，true 表示从低位到高位
	bool buzzer_enabled = false; // 蜂鸣器使能状态

	auto pin_index{0U};
	while (1)
	{
		// 读取
		direction_right ^= shift_direction.is_pressed(); // 按下按键则改变方向
		buzzer_enabled = buzzer_enable.get();

		// 输出
		buzzer.on();
		leds[pin_index].on();
		Delay_ms(100);

		if (buzzer_enabled)
			buzzer.off();
		leds[pin_index].off();
		Delay_ms(100);

		pin_index = (pin_index + (direction_right ? 1 : -1) + 8) % leds.size();
	}

	return 0;
}
