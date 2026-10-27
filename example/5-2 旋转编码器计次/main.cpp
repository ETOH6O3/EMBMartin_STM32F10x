#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

OLEDConsole<> oled{PB8, PB9};

Buzzer buzzer{PA8, false};

Key shiftled{PB4, false};
Sensor buzzer_enable{PB6};

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

// 外部中断方式的正交编码器：A/B 两相各挂一个 EXTI，中断服务函数由库内部完成
EXTIRotaryEncoder rotary_encoder{PB0, PB1};

// 对射式红外传感器：下降沿触发计数，同样由库内部完成中断
CounterSensor infrared_sensor{PB14, false, 1, 1};

int main()
{
	/***************************************************************************************************
	# 中断

	* 中断：在主程序运行过程中，出现了特定的中断触发条件（中断源），使得 CPU 暂停正在运行的程序，
	  转而去处理中断程序，处理完成后又返回原来被暂停的位置继续运行

	* 中断优先级：当有多个中断源同时申请中断时，CPU 会根据中断源的轻重缓急进行裁决，优先响应更加紧急的中断源

	* 中断嵌套：当一个中断程序正在运行时，又有新的更高优先级的中断源申请中断，
	  CPU 再次暂停当前中断程序，转而去处理新的中断程序，处理完成后依次进行返回

	* NVIC 的中断优先级由优先级寄存器的 4 位（0~15）决定，这 4 位可以进行切分，
	  分为高 n 位的抢占优先级和低 4-n 位的响应优先级

	* 抢占优先级高的可以中断嵌套，响应优先级高的可以优先排队，抢占优先级和响应优先级均相同的按中断号排队

	****************************************************************************************************/
	/***************************************************************************************************
	# EXTI（Extern Interrupt）外部中断

	* EXTI 可以监测指定 GPIO 口的电平信号，当其指定的 GPIO 口产生电平变化时，
	  EXTI 将立即向 NVIC 发出中断申请，经过 NVIC 裁决后即可中断 CPU 主程序，使 CPU 执行 EXTI 对应的中断程序

	* 支持的触发方式：上升沿 / 下降沿 / 双边沿 / 软件触发

	* 支持的 GPIO 口：所有 GPIO 口，** 但相同的 Pin 不能同时触发中断 **(例如 GPIOA_PIN0 和 GPIOB_PIN0)

	* 通道数：16 个 GPIO_Pin，外加 PVD 输出、RTC 闹钟、USB 唤醒、以太网唤醒

	* 触发响应方式：中断响应 ( 触发中断 ) / 事件响应（触发别的外设操作）

	****************************************************************************************************/

	oled.show("Init over\n");
	while (1)
	{
		oled.show("\rencoder: ", rotary_encoder.get_count(), "  ");
	}

	return 0;
}
