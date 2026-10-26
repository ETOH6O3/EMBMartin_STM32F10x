#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

auto SCK_pin = PA9;
auto SDA_pin = PA10;
OLEDConsole<> oled(SCK_pin, SDA_pin);

Key key{PB4, false};
DCMotorDriver driver{PWM{TIM2, 3}, PA4, PA5};

auto test = OutPin(null_pin);

int main()
{

	oled.show("Init over\n");

	driver.work();
	driver.set_mode(DCMotorDriver::Mode::FORWARD);
	driver.set_speed(100);

	while (1)
	{

		if (key.is_pressed())
		{
			driver.add_speed(100);
			oled.showlr("speed: ", driver.get_speed());
		}
	}

	return 0;
}
