#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

OLEDConsole<> oled{PA9, PA10};

Key key{PB4, false};

SteeringEngine steer_engine{TIM2, 1, 0b00};

int main()
{

	oled.show("Init over\n");

	while (1)
	{
		Delay_ms(500);
		steer_engine.anticlockwise_rotate(30);
	}

	return 0;
}
