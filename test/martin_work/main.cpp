
#include <cmath>
#include <memory>
#include <string_view>

#include <etl/function.h>
#include <etl/unordered_map.h>
#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"
#include "EMBMartin/device/debugger.h"

using namespace std;
using namespace EMBMartin::lang;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;
/***************************************************************************************************
IO与外设分配：


	- 8 路红外: B12 B13 B14 B15 A8 A9 A10 A11

	- PA15 PB3 PB4 不得使用

	- 编码器：
		- Left Encoder: PB5 PB6,
		- Right Encoder: PB8 PB7,
	- 超声波传感器转向舵机：PB9 (TIM4_CH4)

	- OLED: SCL - PB1, SDA - PA12
	- 超声波传感器: TRIG - PA6  , ECHO - PA2

	- UART（USB/Bluetooth）: TX - PB10, RX - PB11
	- A7 禁用
	- DCMotorDriver: PA0-5, TIM2
		- Left Motor PWM - PA0（TIM2_CH1）, IN1 - PA4, IN2 - PA5
		- Right Motor PWM - PA1（TIM2_CH2）, IN1 - PA3, IN2 - PB0
		- STBY：3V3
TIM：
- 定期调试信息通讯、电机测速读取、超声发送 TIM1
- 电机 TIM2
- 舵机 TIM4
- ~~电机测速编码器接口 TIM3 TIM4~~(TODO:改为外部中断实现)
- 超声计时接收 TIM3

U[S]ART: USART3

禁用 JTAG，保留 SWD


未说明的模块引脚，可参见江协科技资料包中的接线图
其它注意事项：STM32 在 PCB 上每个引脚额外引出一根扩展排针用于示波器调试
****************************************************************************************************/

struct UARTConsoleDebugArgs
{
	// 电机速率
	int16_t v_left;
	int16_t v_right;
	// Key 状态
	uint8_t keys : 2;
	// 8 路红外传感器状态
	uint8_t sensors;
	// 超声测距
	// TODO:
	// 主程序状态机信息
	// TODO:
};

Sensor sensor1{PB12};
Sensor sensor2{PB13};
Sensor sensor3{PB14};
Sensor sensor4{PB15};
Sensor sensor5{PA8};
Sensor sensor6{PA9};
Sensor sensor7{PA10};
Sensor sensor8{PA11};

PWM pwma = {TIM2, 1}; // A0, left motor
PWM pwmb = {TIM2, 2}; // A1, right motor

OutPin ain1 = PA4;
OutPin ain2 = PA5;
OutPin bin1 = PA3;
OutPin bin2 = PB0;

DCMotorDriver driver{
	pwma,
	ain1,
	ain2,
	OutPin{null_pin},
	pwmb,
	bin1,
	bin2,
};

constexpr double TIM1_TIME_S = 0.2;
constexpr double TIM3_TIME_S = 0.0001;

volatile InnerTimerManager timer3{TIM3, TIM3_TIME_S, true, 0, 0};
volatile InnerTimerManager timer1{TIM1, TIM1_TIME_S, true, 0, 0};

// TIM3（0.1 ms）负责回波采样，TIM1（0.2 s）负责触发测距——两者节拍频率相差四个数量级，必须分开挂
UltrasonicSensor ultrasonic_sensor{PA2, PA6, TIM3_TIME_S, 300, TIM3, TIM1}; // TRIG - PA6, ECHO - PA2

auto oled_scl = PB1;
auto oled_sda = PA12;
BluetoothConsole<> usart{USART3, 0, 9600}; // PB10 PB11
OutStream<> &console = usart;
OLEDConsole<> oled{oled_scl, oled_sda};
SteeringEngine servo_pwm{TIM4, 4}; // PB9, TIM4_CH4, 50 Hz, center position

// 速度由 TIM1（0.2 s）自动锁存，get_speed() 只读不清零
EXTIRotaryEncoder left_encoder{PB5, PB6, 0, 0, EXTI_Mode_Interrupt, nullptr};
EXTIRotaryEncoder right_encoder{PB8, PB7, 0, 0, EXTI_Mode_Interrupt, nullptr};

// 基于 TIM3 节拍的阻塞延时（1 拍 = 0.1 ms）
Delayer tick_delayer{TIM3};

int main()
{

	usart.println("\ninit over\n");
	oled.println("\rinit over\r\r");

	// driver.set_mode(DCMotorDriver::Mode::FORWARD, 0);
	// driver.set_mode(DCMotorDriver::Mode::FORWARD, 1);
	// driver.set_speed(800, 0);
	// driver.set_speed(800, 1);

	usart.plot_clear();
	console.printf("compute over\n");
	int angle = 0;
	// while (1)
	// {
	// 	// servo_pwm.set_duty(75);
	// 	// servo_pwm.set_degree(0);
	// 	// delay_ms(1000);
	// 	servo_pwm.set_degree(85);
	// 	// servo_pwm.set_degree(180);
	// 	// delay_ms(1000);
	// 	tick_delayer.delay(100);
	// 	usart.plot(90, ultrasonic_sensor.get_distance() * 100, 90, 50);
	// }
	while (1)
	{
		// oled.printf("%d", PB5.read());
		// oled.printf("%d%d%d%d%d%d%d%d\n", bool(sensor1), bool(sensor2), bool(sensor3), bool(sensor4), bool(sensor5), bool(sensor6), bool(sensor7), bool(sensor8));
		// oled.printf("left: %d\n", left_encoder.get_speed());
		// oled.printf("right: %d\n", right_encoder.get_speed());
	// 	oled.println("Dist: {}cm", EMBMartin::fixed<2>{ultrasonic_sensor * 100.0});
	// 	servo_pwm.set_degree(angle += 1);
	// 	if (angle == 180)
	// 	{
	// 		servo_pwm.set_degree(90);
	// 		oled.printf("angle: %d\n", 90);
	// 		tick_delayer.delay(1000);
	// 		for (auto i : range(0, 100))
	// 		{
	// 			usart.plot(90, ultrasonic_sensor.get_distance() * 100, 90, 50);
	// 			tick_delayer.delay(5);
	// 		}
	// 		angle = 0;
	// 		servo_pwm.set_degree(angle);
	// 		tick_delayer.delay(1000);
	// 	}
	// 	oled.printf("angle: %d\n", angle);

	// 	tick_delayer.delay(100);
	// 	usart.plot(angle, ultrasonic_sensor.get_distance() * 100, 90, 50);

	oled.printf("pace: %d %d \n", left_encoder.get_count(), right_encoder.get_count());
		oled.set_coordinate(0, 0);
	}
	return 0;
}
