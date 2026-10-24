
#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin;
using namespace STM32;

// OLED 走软件 I2C：SCL - PB8，SDA - PB9
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

int main()
{
	oled.show("TheNameless>\nFarewell, Cyrene");
	int i = 0;
	for (; i < 3; i++)
	{
		oled.show(".");
		Delay_ms(1000);
	}
	// 是救世主的又一次离别

	oled.show("\rIronTomb>\n");
	constexpr const char hello_world[] = "Hello, World!";
	for (auto p = hello_world; *p != '\0'; p++)
	{
		oled.show(*p);
		Delay_ms(100);
	}
	// 是新世界的第一声啼哭

	while (1)
	{
	}

	return 0;
}
