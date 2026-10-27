#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

OLEDConsole<> oled{PA9, PA10};

BreathingLED led{TIM2, 1, 0b01};

int main()
{

	oled.show("Init over\n");

	while (1)
	{
		led.breathe();
	}

	return 0;
}
