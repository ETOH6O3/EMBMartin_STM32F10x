#include <cmath>
#include <string_view>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"
#include "EMBMartin/lang/fmt.h"

using namespace std;
using namespace EMBMartin;
using namespace STM32;

auto oled_scl = PB8;
auto oled_sda = PB9;
OLEDConsole<> oled{oled_scl, oled_sda};
USBConsole<> USART_to_USB{USART3, 0, 9600}; // PB10 PB11
OutStream<> &console = USART_to_USB;

struct ClassA
{
	int x = 0;
	int &y;
};

struct ClassB
{
	const int x;
	const int &y = 114;
};

struct ClassC
{
	int &&x = 10;
	const int &&y = 999;
};

struct ClassE
{
	int &x;
};

struct ClassF : ClassE
{
	int y = 5;
};
int main()
{
	console.printf("\ninit over\n");

	console.println("coor: {::#11X;:#10b}", Coordinate<int>(0xff, 0b101010));
	console.println("coor: {:(:#10X)}", Coordinate<int>(0xff, 0b101010));
	console.println("coor: {:[:#10X,,:#10b]}\n", Coordinate<int>(0xff, 0b101010));
	console.println("stru: {:[]}\n", add_result<int>{20, false});
	console.println("DEC: {:|^#10,d}", -42000);
	console.println("Hex: {:|^#10_x}", 0xfffff);
	console.println("f:   {:|^10_.2_g}", 3.1415);
	console.println("char:{:|^10}", 'm');
	console.println("str: {:|^10}", "aminaoc");
	console.println("sv:  {:|^10}", etl::string_view("wwwwwww"));

	// 旧版 is_tuple_v / is_pair_v 已并入 meta.h 的广义 pair / tuple 协议
	constexpr auto t4 = generalized_tuple_size_v<std::array<int, 4>> == 4;
	constexpr auto t5 = is_pair_protocol_v<add_result<int>>;
	constexpr auto t6 = is_formattable_v<add_result<int>>;

	console.showln(t4, t5, t6);

	static_assert(is_pair_protocol_v<ClassA>);
	static_assert(is_pair_protocol_v<ClassB>);
	static_assert(is_pair_protocol_v<ClassC>);
	static_assert(!is_pair_protocol_v<ClassE>);
	static_assert(is_pair_protocol_v<ClassF>);

	while (1)
	{
		// console.printf("compute over\r");
		Delay_ms(1000);
	}

	return 0;
}
