#include <array>
#include <iterator>
#include <numeric>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"

using namespace std;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;

OLEDConsole<> oled{PA9, PA10};

uint16_t num = 0;

// 外部时钟模式的定时器：TIM2 以 PA0 的边沿作为计数时钟，更新中断由库统一分发
OuterTimer timer{TIM2, PA0, 2, 0, 0};

int main()
{

	oled.show("Init over\n");
	while (1)
	{
		oled.show("\r", num, "|", TIM_GetCounter(TIM2));
	}

	return 0;
}
