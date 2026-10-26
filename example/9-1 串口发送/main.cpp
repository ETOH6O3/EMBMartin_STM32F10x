#include <cmath>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::lang;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

auto oled_scl = PB8;
auto oled_sda = PB9;
OLEDConsole<> oled{oled_scl, oled_sda};

int main()
{
	// 建议放在主函数中初始化
	USBConsole<> USART_to_USB{USART1, 0, 9600}; // PA9 PA10
	OutStream<> &console = USART_to_USB;

	oled.showlr("Init over");
	USART_to_USB.showln("你好，世界！");
	for (int i = 0; i < 1000; i++)
	{
		oled.showlr(i);
		USART_to_USB.showlr(i);
		Delay_ms(200);
	}

	// USART_to_USB.show('A');

	while (1)
	{
		USART_to_USB.show('A');
	}

	return 0;
}
