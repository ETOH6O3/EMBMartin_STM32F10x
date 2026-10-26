#include <cmath>
#include <string_view>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"
#include "EMBMartin/lang/fmt.h"

using namespace std;
using namespace EMBMartin::lang;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;
using namespace EMBMartin::algorithm;

auto oled_scl = PB8;
auto oled_sda = PB9;
OLEDPlayerBase oled{oled_scl, oled_sda};
USBConsole<> USART_to_USB{USART3, 0, 9600}; // PB10 PB11
OutStream<> &console = USART_to_USB;

int main()
{
	USART_to_USB.showln("\nInit over");

	IterableUInt8 x = 0b10101100;
	x[0] = 1;
	for (bool i : x)
	{
		USART_to_USB.show(i);
	}
	USART_to_USB.showln("");

	OLEDBitMap<10, 5> pic1 = {{0xff, 0xff, 0b10101010, 0, 0, 1, 2, 3, 4, 5}, BWPixel::Transparent};
	oled.show_pic(pic1, {54, 120});
	oled.show_pic(WesternFont<>('b'), {10, 40});
	oled.show_pic(WesternFont<>('d'), {40, 45});
	oled.show_pic(ExplicitFunc<20, 20>([](double x)
									   { return x; }),
				  {20, 0});
	auto func2 = ExplicitFunc<30, 30>([](double x)
									  { return std::pow(x - 15, 2) * 0.15; });
	oled.show_pic(func2, {30, 50});

	auto func3 = ExplicitFunc<21, 40>([](double x)
									  { return std::sin(x / 10 * 3.14) * 10 + 10; });
	oled.show_pic(func3, {50, 0});

	string_view sv1("114514");

	console.showln(sv1[0]);

	// 新版格式化入口：format_to / formatted_size，不再有 format<N>()
	etl::string<64> formatted;
	etl_fmt::format_to(formatted, "Value: {}, Hex: {:x}", 42, 255);

	for (auto c : formatted)
	{
		console.show(c);
	}
	console.showln("");

	while (1)
	{
		USART_to_USB.showlr("compute over");
		Delay_ms(1000);
	}

	return 0;
}
