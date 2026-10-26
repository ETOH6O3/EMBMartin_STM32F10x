#include <cmath>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::lang;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

auto oled_scl = PB8;
auto oled_sda = PB9;
OLEDConsole<OLEDFontSize::F8x16> oled{oled_scl, oled_sda};
USBConsole<> USART_to_USB{USART3, 0, 9600}; // PB10 PB11
OutStream<> &console = USART_to_USB;

int main()
{
	USART_to_USB.printf("Init over");

	oled.printf("Hello, %s!", "world");

	while (1)
	{
	}

	return 0;
}
