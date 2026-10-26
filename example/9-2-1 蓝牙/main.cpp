#include <cmath>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::lang;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

auto oled_scl = PC14;
auto oled_sda = PC15;
OLEDConsole<> oled{oled_scl, oled_sda};

auto mpu_scl = PB8;
auto mpu_sda = PB9;
MPU6050 mpu6050{mpu_scl, mpu_sda, 0}; // AD0 = 0，地址 0xD0

Buzzer buzzer = {PB14, false}; // 低电平驱动
Key key1 = {PA7, false};       // 低电平触发
Key key2 = {PA12, true};       // 高电平触发

Sensor ir1 = PB5;
Sensor ir2 = PB4;
Sensor ir3 = PB3;
Sensor ir4 = PA15;

OutPin ain1 = PB13;
OutPin ain2 = PB12;
OutPin bin1 = PB1;
OutPin bin2 = PB0;
PWM pwma{TIM1, 4}; // PA11
PWM pwmb{TIM1, 1}; // PA8
DCMotorDriver driver{pwma, ain1, ain2,
					 null_pin, // STBY
					 pwmb, bin1, bin2};
int main()
{
	// 建议放在主函数中初始化
	BluetoothConsole<> bluetooth{USART3, 0, 115200}; // PB10 PB11
	OutStream<> &console = bluetooth;

	oled.showlr("Init over");
	driver.set_speed(200, 0);
	driver.set_speed(200, 1);

	char cmd[128];
	bluetooth.showln("helloworld!");
	while (1)
	{
		bluetooth.showln(" 请输入一个字符：");
		bluetooth.scanf("%s", cmd);
		oled.printf("Received: %s\r", cmd);
	}

	return 0;
}
