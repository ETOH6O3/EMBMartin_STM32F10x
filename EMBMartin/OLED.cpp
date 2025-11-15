#include "OLED.h"
#include "OLED_Font.h"

using namespace EMBMartin::STM32::OLED;
using namespace EMBMartin::STM32::OLED::I2C;

/*引脚配置*/
#define OLED_W_SCL(x) GPIO_WriteBit(OLED_SCL.port, OLED_SCL.pin, (BitAction)(x))
#define OLED_W_SDA(x) GPIO_WriteBit(OLED_SDA.port, OLED_SDA.pin, (BitAction)(x))

IO::IO(const GPIOPin &SCL, const GPIOPin &SDA) noexcept : OLED_SCL(SCL), OLED_SDA(SDA), current_X(1), current_Y(1)
{
	RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SCL.port), ENABLE);
	RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SDA.port), ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = OLED_SCL.pin;
	GPIO_Init(OLED_SCL.port, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = OLED_SDA.pin;
	GPIO_Init(OLED_SDA.port, &GPIO_InitStructure);

	OLED_W_SCL(1);
	OLED_W_SDA(1);

	// 使用更可靠的延时方式防止编译器优化延时循环
	volatile uint32_t i, j;

	for (i = 0; i < 1000; i++)
	{
		for (j = 0; j < 1000; j++)
		{
			// 添加内联汇编防止编译器优化空循环
			__asm__ volatile("" ::: "memory");
		}
		__asm__ volatile("" ::: "memory");
	}

	WriteCommand(0xAE); // 关闭显示

	WriteCommand(0xD5); // 设置显示时钟分频比/振荡器频率
	WriteCommand(0x80);

	WriteCommand(0xA8); // 设置多路复用率
	WriteCommand(0x3F);

	WriteCommand(0xD3); // 设置显示偏移
	WriteCommand(0x00);

	WriteCommand(0x40); // 设置显示开始行

	WriteCommand(0xA1); // 设置左右方向，0xA1正常 0xA0左右反置

	WriteCommand(0xC8); // 设置上下方向，0xC8正常 0xC0上下反置

	WriteCommand(0xDA); // 设置COM引脚硬件配置
	WriteCommand(0x12);

	WriteCommand(0x81); // 设置对比度控制
	WriteCommand(0xCF);

	WriteCommand(0xD9); // 设置预充电周期
	WriteCommand(0xF1);

	WriteCommand(0xDB); // 设置VCOMH取消选择级别
	WriteCommand(0x30);

	WriteCommand(0xA4); // 设置整个显示打开/关闭

	WriteCommand(0xA6); // 设置正常/倒转显示

	WriteCommand(0x8D); // 设置充电泵
	WriteCommand(0x14);

	WriteCommand(0xAF); // 开启显示

	Clear(); // OLED清屏
}

inline void IO::I2C_Start(void) noexcept
{
	OLED_W_SDA(1);
	// 添加数据同步屏障防止编译器重排指令
	__DSB(); 
	__ISB();
	OLED_W_SCL(1);
	__DSB(); 
	__ISB();
	OLED_W_SDA(0);
	__DSB(); 
	__ISB();
	OLED_W_SCL(0);
	__DSB(); 
	__ISB();
}

inline void IO::I2C_Stop(void) noexcept
{
	OLED_W_SDA(0);
	__DSB(); 
	__ISB();
	OLED_W_SCL(1);
	__DSB(); 
	__ISB();
	OLED_W_SDA(1);
	__DSB(); 
	__ISB();
}

inline void IO::SetCursor(uint8_t Y, uint8_t X) noexcept
{
	WriteCommand(0xB0 | Y);					// 设置Y位置
	WriteCommand(0x10 | ((X & 0xF0) >> 4)); // 设置X位置高4位
	WriteCommand(0x00 | (X & 0x0F));		// 设置X位置低4位
}

inline void IO::I2C_SendByte(uint8_t Byte) noexcept
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		// 添加数据同步屏障防止编译器重排指令
		__DSB(); 
		__ISB();
		OLED_W_SCL(1);
		__DSB(); 
		__ISB();
		OLED_W_SCL(0);
		__DSB(); 
		__ISB();
	}
	OLED_W_SCL(1); // 额外的一个时钟，不处理应答信号
	__DSB(); 
	__ISB();
	OLED_W_SCL(0);
	__DSB(); 
	__ISB();
}

inline void IO::WriteCommand(uint8_t Command) noexcept
{
	I2C_Start();
	I2C_SendByte(0x78); // 从机地址
	I2C_SendByte(0x00); // 写命令
	I2C_SendByte(Command);
	I2C_Stop();
}

inline void IO::WriteData(uint8_t Data) noexcept
{
	I2C_Start();
	I2C_SendByte(0x78); // 从机地址
	I2C_SendByte(0x40); // 写数据
	I2C_SendByte(Data);
	I2C_Stop();
}

inline void IO::Clear(void) noexcept
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		SetCursor(j, 0);
		for (i = 0; i < 128; i++)
		{
			WriteData(0x00);
		}
	}
	this->current_X = 1;
	this->current_Y = 1;
}

void IO::ShowChar(uint8_t Line, uint8_t Column, char Char) noexcept
{
	uint8_t i;
	SetCursor((Line - 1) * 2, (Column - 1) * 8); // 设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		WriteData(OLED_F8x16[Char - ' '][i]); // 显示上半部分内容
	}
	SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8); // 设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		WriteData(OLED_F8x16[Char - ' '][i + 8]); // 显示下半部分内容
	}
}

void IO::ShowChar(char c) noexcept
{
	if (c == '\n')
	{
		current_X = 1;
		current_Y++;
		if (current_Y > 4)
		{
			current_Y = 1;
		}
		return;
	}
	if (c == '\r')
	{
		current_X = 1;
		return;
	}
	if (c == '\t')
	{
		current_X += 4;
		if (current_X > 16)
		{
			current_X = 1;
			current_Y++;
			if (current_Y > 4)
			{
				current_Y = 1;
			}
		}
		return;
	}
	ShowChar(current_Y, current_X, c);
	current_X++;
	if (current_X > 16)
	{
		current_X = 1;
		current_Y++;
		if (current_Y > 4)
		{
			current_Y = 1;
		}
	}
}
void IO::ShowNum(uintmax_t Number) noexcept
{
	char buff[64];
	auto p = std::end(buff);
	*--p = '\0';
	do
	{
		*--p = (Number % 10) + '0';
		Number /= 10;
	} while (Number != 0);
	ShowString(p);
}

void IO::ShowSignedNum(intmax_t Number) noexcept
{
	if (Number < 0)
	{
		ShowChar('-');
		ShowNum(static_cast<uintmax_t>(-Number));
	}
	else
	{
		ShowNum(static_cast<uintmax_t>(Number));
	}
}
inline uint32_t IO::Pow(uint32_t X, uint32_t Y) noexcept
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

void IO::ShowHexNum(uint32_t Number) noexcept
{
	char buff[64];
	auto p = std::end(buff);
	*--p = '\0';
	do
	{
		auto SingleNumber = Number % 16;
		Number /= 16;
		if (SingleNumber < 10)
		{
			*--p = SingleNumber + '0';
		}
		else
		{
			*--p = SingleNumber - 10 + 'A';
		}
	} while (Number != 0);
	ShowString(p);
}

void IO::ShowBinNum(uint32_t Number) noexcept
{
	char buff[64];
	auto p = std::end(buff);
	*--p = '\0';
	do
	{
		*--p = (Number & 1) + '0';
		Number >>= 1;
	} while (Number != 0);
	ShowString(p);
}