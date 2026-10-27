#include <cmath>
#include <string_view>
#include <utility>

#include "stm32f10x.h" // Device header

#include "EMBMartin/EMBMartin.h"
#include "EMBMartin/lang/functional.h"

using namespace std;
using namespace EMBMartin::lang;
using namespace EMBMartin::hal;
using namespace EMBMartin::device;
using namespace EMBMartin::algorithm;

auto oled_scl = PB8;
auto oled_sda = PB9;
OLEDConsole<> oled{oled_scl, oled_sda};
USBConsole USART_to_USB{USART3, 0, 9600}; // PB10 PB11
OutStream<> &console = USART_to_USB;

struct ClassA
{
	int a;
	float b;
	char c;
	string_view str;
};

template <auto T>
void loli()
{
	console.showln(__PRETTY_FUNCTION__);
}
int main()
{
	// foo<(GPIO_TypeDef* )114>();

	detail::_to_tuple(std::make_tuple(1));

	USBConsole USART_to_USB{USART3, 0, 9600}; // PB10 PB11

	constexpr auto x = generalized_tuple_size_v<add_result<int>>;
	// constexpr auto z = std::tuple_size_v<add_result<int>>;
	// constexpr auto y = EMBMartin::tuple_size_v<std::tuple<int, tuple<int, int>>>;
	// constexpr auto z = EMBMartin::is_N_list_constructible_v<0, CompileTimeAny>;
	// constexpr auto a = EMBMartin::is_N_list_constructible_v<1, CompileTimeAny>;
	// constexpr auto b = EMBMartin::is_N_list_constructible_v<2, CompileTimeAny>;
	// using t1 = EMBMartin::tuple_element_t<0, add_result<int>>;
	// using t2 = EMBMartin::tuple_element_t<1, add_result<int>>;
	// using t3 = EMBMartin::tuple_element_t<0, tuple<AngleComplementaryFilter, int, pair<int, bool>>>;
	// using t4 = EMBMartin::tuple_element_t<1, tuple<AngleComplementaryFilter, int, pair<int, bool>>>;
	// using t5 = EMBMartin::tuple_element_t<2, tuple<AngleComplementaryFilter, int, pair<GPIOPin, bool>>>;

	// const t5 e{PA12, false};

	// console.showln(x, y, z, a, b, __LINE__);

	const auto &[y1, x1] = unpack_post<1>(std::make_tuple(2, 1, 3));
	const auto &[y10, y11] = y1;

	constexpr static ClassA objA{10, 3.14, 'm', "mmd"};
	// constexpr auto tup = objA | to_tuple<const ClassA &>;
	constexpr auto tup1 = emplace_back<pair<char, char>>(objA, pair<char, char>{'a', 'v'});
	constexpr auto tup2 = push_back(objA, 12345);
	constexpr auto tup3 = swap<0, 1>(objA);
	// auto objb = objA;
	console.showsep(x1, y10, y11, get<4>(tup1).second);
	console.showln();

	using tyt = corresponding_std_tuple_t<std::tuple<int, int>>;

	auto _ = pipe(114)
	| [](auto){ return std::make_tuple(1,1); }
	| [](auto, auto){ return std::make_tuple(1); }
	| [](auto){ return std::make_tuple(1,2,3); }
	| [](auto, auto, auto){ return std::make_tuple(0); }
	| [](auto){ return std::make_tuple(1,2,3,4,5,6,7); }
	| pipe_split<4>()
	| [&](auto&&... args){ console.showsep(args...); }
	;
	// console.showsep(_0,_1,_2,_3,_4);
	console.showln();

	const auto [_10,_11,_12,_13] = pipe(1, 2, 3, 4)
	| unpack_tuples_concept
	| [](auto,auto,auto,auto){return ClassA{};}
	;

	std::tuple<>();

	for(auto i:range(1, 4, 1))
	{
		console.show(i, ' ');
	}
	console.showln();

	Slice<int>(1, 4, 1);

	while (1)
	{
		// console.printf("compute over\r");
		Delay_ms(1000);
	}

	return 0;
}