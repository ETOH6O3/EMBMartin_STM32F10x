#pragma once

#include <array>
#include <cstddef>

#include "macro.h"
#include EMBMARTIN_MACRO_TOSTRING(STM32_DEVICE_HEADER)
#include "basic_tools.h"
#include "system.h"
#include "meta.h"
#include "stream.h"

/**
 * @brief 位翻转 I2C 的位延时循环次数（按主频标定）
 *
 * @warning EMBMARTIN_KEEP_CODE_ORDER 只是空的 inline asm（`asm volatile("" ::: "memory")`），
 *          它不产生任何指令，**不提供任何延时**，只能阻止编译器重排，不能当时序手段用。
 *
 * 位翻转 I2C 必须靠真实延时保证 SCL 的高/低电平时间。否则在 -O1（叠加 -flto 会把
 * GPIO_WriteBit 内联成一条 BSRR 写）下一次 SCL 翻转会被压缩到几十纳秒，导致：
 *   1. SCL 实测约 4.5MHz，超出 I2C/SSD1306 规格约 10 倍；
 *   2. receive_ack() 在 SCL 上升沿后约 30~60ns 就采样 SDA（反汇编里"STR 拉高 SCL"与
 *      "LDR 读 IDR"紧邻，中间 0 条指令），从机来不及应答，必然读回被上拉拉高的电平而
 *      被误判为 NACK，进而触发 EMBMARTIN_ASSERT。这正是"开 -O1 就进 HardFault、
 *      把 SDA 拔掉反而正常"的根本原因。
 *
 * 72MHz 下每轮循环实测约 12 个周期（-O1），默认 6 轮 ≈ 1.1µs，对应：
 *   tHIGH ≈ 1.1µs（规格 ≥ 0.6µs）
 *   tLOW  ≈ 2.2µs（规格 ≥ 1.3µs；每个 bit 有两段延时）
 *   SCL   ≈ 300kHz（规格 ≤ 400kHz）
 *   应答采样前延时 ≈ 1.1µs（修复前只有 30~60ns）
 *
 * 如需提速可减小该值，但必须保证 tHIGH ≥ 0.6µs、tLOW ≥ 1.3µs（实测下限约 4 轮）；
 * 更换主频后务必重新标定（有示波器的话直接量 PB8 的 SCL）。
 *
 * @note 该延时会让 OLED 全屏刷新耗时约 130ms，主循环整体刷新率约 10~15Hz。
 *       若嫌慢，正确做法是改用硬件 I2C1（重映射后正好是 PB8/PB9），而不是压缩延时。
 */
#ifndef EMBMARTIN_I2C_DELAY_LOOPS
#define EMBMARTIN_I2C_DELAY_LOOPS 6
#endif

EMBMARTIN_STM32F10X_NAMESPACE_BEGIN

class I2C
{
private:
    GPIOPin __SCL; //!< I2C 时钟引脚
    GPIOPin __SDA; //!< I2C 数据引脚
protected:
    uint8_t _addr; //!< 从机地址

    /**
     * @brief 位翻转 I2C 的位延时（真实延时，同时充当编译器屏障）
     *
     * 既提供电平保持时间，也通过内存 clobber 保证 SDA/SCL 的访问不会被编译器
     * 重排到延时两侧。循环变量为 volatile，因此不会被优化掉。
     *
     * @note 不要在实时性敏感的路径上滥用；一次 SSD1306 全屏刷新约需上百毫秒。
     */
    static inline void i2c_delay() noexcept
    {
        EMBMARTIN_KEEP_CODE_ORDER; // 保持原有的防乱序语义
        for (volatile uint32_t i = 0; i < EMBMARTIN_I2C_DELAY_LOOPS; ++i)
        {
            __NOP();
        }
    }

    void start() noexcept;

    void stop() noexcept;

    void send_byte(uint8_t data) noexcept;
    void send_ack(bool data) noexcept;

    uint8_t receive_byte() noexcept;
    void receive_ack() noexcept;

public:
    /**
     * @brief 自动完成需要的所有初始化
     * @param SCL I2C 时钟线引脚
     * @param SDA I2C 数据线引脚
     * @param addr 从机地址，第 0 位强制置 0
     */
    I2C(GPIOPin SCL, GPIOPin SDA, uint8_t addr) noexcept;

    template <typename... Args>
    void write_reg(uint8_t first_reg_addr, Args... data) noexcept
    {
        start();
        send_byte(_addr);
        receive_ack();
        send_byte(first_reg_addr);
        receive_ack();

        ((send_byte(*reinterpret_cast<const uint8_t *>(&data) /* 按位映射为 8 位无符号数 */), receive_ack()), ...);

        stop();
    }

    template <typename _Container_OR_NUM>
    void write_reg(uint8_t first_reg_addr, const _Container_OR_NUM &data) noexcept
    {
        if constexpr (has_iterator_v<_Container_OR_NUM>)
        {
            start();
            send_byte(_addr);
            receive_ack();
            send_byte(first_reg_addr);
            receive_ack();

            for (auto byte : data)
            {
                send_byte(*reinterpret_cast<const uint8_t *>(&byte));
                receive_ack();
            }

            stop();
        }
        else if constexpr (std::is_arithmetic_v<_Container_OR_NUM>)
        {
            const auto &reg_addr = first_reg_addr;
            start();
            send_byte(_addr);
            receive_ack();
            send_byte(reg_addr);
            receive_ack();
            send_byte(*reinterpret_cast<const uint8_t *>(&data));
            receive_ack();
            stop();
        }
        else
        {
            static_assert(false, "必须传入算术或可迭代类型");
        }
    }

    uint8_t read_reg(uint8_t reg_addr) noexcept;

    template <size_t N>
    std::array<uint8_t, N> read_reg(uint8_t reg_addr) noexcept
    {
        std::array<uint8_t, N> result;

        // 指定寄存器地址
        start();
        send_byte(_addr);
        receive_ack();
        send_byte(reg_addr);
        receive_ack();

        // 进入读模式
        start();
        send_byte(_addr | 0x01);
        receive_ack();

        // 读取多个字节
        for (size_t i = 0; i < N; ++i)
        {
            bool send_nack = (i == N - 1);
            result[i] = receive_byte();
            send_ack(send_nack);
        }

        stop();
        return result;
    }

#if EMBMARTIN_USING_OLD_OLED_VERSION == 0
    friend class OLEDBase;
#endif
};


EMBMARTIN_STM32F10X_NAMESPACE_END
