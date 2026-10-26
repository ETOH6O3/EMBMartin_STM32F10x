#include <cstring>

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
	USBConsole<> USART_to_USB{USART3, 0, 9600}; // PB10 PB11
	OutStream<> &console = USART_to_USB;

	oled.showlr("Init over");

	USART_to_USB.showln("Init over");
	USART_to_USB.showln("Hello Amphoreus!");

	int i = 0;
	while (1)
	{
		i++;
		char buffer[128];
		USART_to_USB.getline(buffer);
		if (i % 2)
			oled.showlr(buffer);
		if (strcmp(buffer, "Hello world!") == 0)
		{
			USART_to_USB.showln("Hello Amphoreus!");
		}
		else if (strcmp(buffer, "33550336") == 0)
		{
			USART_to_USB.showln("众人将与一人离别，唯其人可觐见奇迹,此乃命运使然");
		}
		else if (strcmp(buffer, "PoleMos600") == 0)
		{
			USART_to_USB.showln("Mydeimos,become king.\n加冕为王吧，迈德莫斯。");
		}
		else if (strcmp(buffer, "ApoRia432") == 0)
		{
			USART_to_USB.showln("The place you reside is the gentle sea of flowers.\n汝栖身于温柔的花海。");
		}
		else if (strcmp(buffer, "KaLos618") == 0)
		{
			USART_to_USB.showln("Thank you for your hard work all these while.\n一直以来辛苦你了，阿格莱雅。");
		}
		else if (strcmp(buffer, "HapLotes405") == 0)
		{
			USART_to_USB.showln("From now, no more goodbyes.\n从今往后，不会再有离别。");
		}
		else if (strcmp(buffer, "SkeMma720") == 0)
		{
			USART_to_USB.showln("The seeds you've sown are already sprouting.\n你所播下的种子已然萌芽。");
		}
		else if (strcmp(buffer, "EleOs252") == 0)
		{
			USART_to_USB.showln("It's you who healed the sky.\n是你治愈了天空。");
		}
		else if (strcmp(buffer, "OreXis945") == 0)
		{
			USART_to_USB.showln("Homeward you come distant wind.\n你从远方的风中归来。");
		}
		else if (strcmp(buffer, "NeiKos496") == 0)
		{
			USART_to_USB.showln("Did you see? Amphoreus has ushered in the dawn.\n你看见了吗，卡厄斯兰那？翁法罗斯已经迎来了她的黎明。");
		}
		else if (strcmp(buffer, "ApoRia432") == 0)
		{
			USART_to_USB.showln("May the next feast be eternal.\n愿下一场欢宴永恒。");
		}
		else if (strcmp(buffer, "HubRis504") == 0)
		{
			USART_to_USB.showln("The stars shall sing of the imperator's great journey.\n群星将奏响凯撒的征程。");
		}
		else if (strcmp(buffer, "Evernight") == 0)
		{
			USART_to_USB.showln("We were never apart.\n我们从未分离。");
		}
		else if (strcmp(buffer, "DanHeng") == 0)
		{
			USART_to_USB.showln("Let's walk towards tomorrow together.\n让我们一起走向明天。");
		}
		else if (strcmp(buffer, "Cyrene") == 0)
		{
			USART_to_USB.showln("十三声心跳后，光会创造天地。");
		}
		buffer[0] = '\0';
	}

	return 0;
}
