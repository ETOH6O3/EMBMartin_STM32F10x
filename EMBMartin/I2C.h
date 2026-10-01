#ifndef EMBMARTIN_I2C_H
#define EMBMARTIN_I2C_H

#include <array>
#include <cstddef>
#include <type_traits>

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
    GPIOPin _SCL; //!< I2C 时钟引脚
    GPIOPin _SDA; //!< I2C 数据引脚
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

    inline void start() noexcept
    {
        /***************************************************************************************************
        SCL
        XXXXXXXXXXXXXXXXXXXXXX
                             XX
                              X
                              XX
                               X
                               XX
                                X
        SDA                     XXXXXXXXXXXXXXXX
        XXXXXXXXXXXXXXXXXX
                         X
                         XX
                          X
                          X
                          XX
                           X
                           XXXXXXXXXXXXXXXXXXXX

        ****************************************************************************************************/
        _SDA.set();
        i2c_delay(); // 防止与终止信号混淆，同时保证 SDA 建立时间
        _SCL.set();

        i2c_delay(); // SCL 高电平保持（起始信号的建立时间）
        _SDA.reset();
        i2c_delay(); // 起始信号的数据保持时间
        _SCL.reset();
        i2c_delay(); // SCL 低电平保持
    }

    inline void stop() noexcept
    {
        /***************************************************************************************************
                          XXXXXXXXXXXXXXXX
                         XX
                        XX
                        XX
         SCL           XX
        XXXXXXXXXXXXXXXXX


                                  XXXXXXXX
                                XXX
                               XXX
                              XX
        SDA                  XX
        XXXXXXXXXXXXXXXXXXXXXXX

        ****************************************************************************************************/
        // 契约：确保 SCL 必定已经是低电平
        _SDA.reset();
        i2c_delay(); // SDA 建立时间

        _SCL.set();
        i2c_delay(); // SCL 高电平保持（停止信号的建立时间）
        _SDA.set();
        i2c_delay(); // 停止信号的数据保持时间 + 总线空闲时间
    }

    void send_byte(uint8_t data) noexcept;
    inline void send_ack(bool data) noexcept
    {
        data ? _SDA.set() : _SDA.reset();
        i2c_delay(); // SDA 建立时间
        _SCL.set();
        i2c_delay(); // SCL 高电平保持
        _SCL.reset();
        i2c_delay(); // SCL 低电平保持
    }

    uint8_t receive_byte() noexcept;
    inline void receive_ack() noexcept
    {
        bool rslt;

        _SDA.set(); // 释放 SDA
        i2c_delay();

        _SCL.set();
        i2c_delay(); // ★ 关键：必须等从机把 SDA 拉低后再采样，否则 -O1 下必然误判 NACK
        rslt = _SDA.read();
        _SCL.reset();
        i2c_delay(); // SCL 低电平保持

        EMBMARTIN_ASSERT(!rslt, "I2C req exception", &console);
    }

public:
    /**
     * @brief 自动完成需要的所有初始化
     * @param SCL I2C 时钟线引脚
     * @param SDA I2C 数据线引脚
     * @param addr 从机地址，第 0 位强制置 0
     */
    inline I2C(GPIOPin SCL, GPIOPin SDA, uint8_t addr) noexcept
        : _SCL(SCL), _SDA(SDA), _addr(addr & 0b11111110)
    {
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SCL.port), ENABLE);
        RCC_APB2PeriphClockCmd(Get_RCC_APB2Periph(SDA.port), ENABLE);
        GPIO_InitTypeDef GPIO_InitStruct = {
            .GPIO_Pin = SCL.pin,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_OD,
        };
        GPIO_Init(SCL.port, &GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = SDA.pin;
        GPIO_Init(SDA.port, &GPIO_InitStruct);

        GPIO_SetBits(SCL.port, SCL.pin);
        GPIO_SetBits(SDA.port, SDA.pin);
    }

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

class MPU6050 : public I2C
{
private:
public:
    /**
     * @brief 仿枚举类，内含 MPU6050 所有寄存器对应编号
     *
     */
    struct REGS
    {
        enum Values : uint8_t
        {
            // 辅助 I2C 电源选择
            AUX_VDDIO = 0x01,

            // 采样率分频器
            SMPLRT_DIV = 0x19,

            // 配置寄存器
            CONFIG = 0x1A,

            // 陀螺仪配置
            GYRO_CONFIG = 0x1B,

            // 加速度计配置
            ACCEL_CONFIG = 0x1C,

            // 自由落体检测
            FF_THR = 0x1D, // 自由落体加速度阈值
            FF_DUR = 0x1E, // 自由落体持续时间

            // 运动检测
            MOT_THR = 0x1F, // 运动检测阈值
            MOT_DUR = 0x20, // 运动检测持续时间

            // 零运动检测
            ZRMOT_THR = 0x21, // 零运动检测阈值
            ZRMOT_DUR = 0x22, // 零运动检测持续时间

            // FIFO 使能
            FIFO_EN = 0x23,

            // I2C 主控制
            I2C_MST_CTRL = 0x24,

            // I2C 从设备 0 控制
            I2C_SLV0_ADDR = 0x25,
            I2C_SLV0_REG = 0x26,
            I2C_SLV0_CTRL = 0x27,

            // I2C 从设备 1 控制
            I2C_SLV1_ADDR = 0x28,
            I2C_SLV1_REG = 0x29,
            I2C_SLV1_CTRL = 0x2A,

            // I2C 从设备 2 控制
            I2C_SLV2_ADDR = 0x2B,
            I2C_SLV2_REG = 0x2C,
            I2C_SLV2_CTRL = 0x2D,

            // I2C 从设备 3 控制
            I2C_SLV3_ADDR = 0x2E,
            I2C_SLV3_REG = 0x2F,
            I2C_SLV3_CTRL = 0x30,

            // I2C 从设备 4 控制
            I2C_SLV4_ADDR = 0x31,
            I2C_SLV4_REG = 0x32,
            I2C_SLV4_DO = 0x33,
            I2C_SLV4_CTRL = 0x34,
            I2C_SLV4_DI = 0x35,

            // I2C 主状态
            I2C_MST_STATUS = 0x36,

            // 中断引脚 / 旁路使能配置
            INT_PIN_CFG = 0x37,

            // 中断使能
            INT_ENABLE = 0x38,

            // 中断状态
            INT_STATUS = 0x3A,

            // 加速度计测量值
            ACCEL_XOUT_H = 0x3B,
            ACCEL_XOUT_L = 0x3C,
            ACCEL_YOUT_H = 0x3D,
            ACCEL_YOUT_L = 0x3E,
            ACCEL_ZOUT_H = 0x3F,
            ACCEL_ZOUT_L = 0x40,

            // 温度测量值
            TEMP_OUT_H = 0x41,
            TEMP_OUT_L = 0x42,

            // 陀螺仪测量值
            GYRO_XOUT_H = 0x43,
            GYRO_XOUT_L = 0x44,
            GYRO_YOUT_H = 0x45,
            GYRO_YOUT_L = 0x46,
            GYRO_ZOUT_H = 0x47,
            GYRO_ZOUT_L = 0x48,

            // 外部传感器数据 (Slave 0-3)
            EXT_SENS_DATA_00 = 0x49,
            EXT_SENS_DATA_01 = 0x4A,
            EXT_SENS_DATA_02 = 0x4B,
            EXT_SENS_DATA_03 = 0x4C,
            EXT_SENS_DATA_04 = 0x4D,
            EXT_SENS_DATA_05 = 0x4E,
            EXT_SENS_DATA_06 = 0x4F,
            EXT_SENS_DATA_07 = 0x50,
            EXT_SENS_DATA_08 = 0x51,
            EXT_SENS_DATA_09 = 0x52,
            EXT_SENS_DATA_10 = 0x53,
            EXT_SENS_DATA_11 = 0x54,
            EXT_SENS_DATA_12 = 0x55,
            EXT_SENS_DATA_13 = 0x56,
            EXT_SENS_DATA_14 = 0x57,
            EXT_SENS_DATA_15 = 0x58,
            EXT_SENS_DATA_16 = 0x59,
            EXT_SENS_DATA_17 = 0x5A,
            EXT_SENS_DATA_18 = 0x5B,
            EXT_SENS_DATA_19 = 0x5C,
            EXT_SENS_DATA_20 = 0x5D,
            EXT_SENS_DATA_21 = 0x5E,
            EXT_SENS_DATA_22 = 0x5F,
            EXT_SENS_DATA_23 = 0x60,

            // 运动检测状态
            MOT_DETECT_STATUS = 0x61,

            // I2C 从设备数据输出
            I2C_SLV0_DO = 0x63,
            I2C_SLV1_DO = 0x64,
            I2C_SLV2_DO = 0x65,
            I2C_SLV3_DO = 0x66,

            // I2C 主延迟控制
            I2C_MST_DELAY_CTRL = 0x67,

            // 信号路径复位
            SIGNAL_PATH_RESET = 0x68,

            // 运动检测控制
            MOT_DETECT_CTRL = 0x69,

            // 用户控制
            USER_CTRL = 0x6A,

            // 电源管理
            PWR_MGMT_1 = 0x6B,
            PWR_MGMT_2 = 0x6C,

            // FIFO 计数
            FIFO_COUNTH = 0x72,
            FIFO_COUNTL = 0x73,

            // FIFO 读写
            FIFO_R_W = 0x74,

            // 设备 ID
            WHO_AM_I = 0x75
        };
    };

    struct Data
    {
        int16_t acc_x, acc_y, acc_z;
        int16_t gyro_x, gyro_y, gyro_z;
    };
    inline MPU6050(
        GPIOPin SCL, GPIOPin SDA, bool AD0 = 0, /* 是否更改地址 */
        bool gyroscope_enable = 1, uint8_t SMPRT_DIV = 8 /* 分频数 */,
        uint8_t ACCEL_AFS_SCL = 0b01 /* 加速度满量程选择 0-3*/,
        uint8_t GYRO_FS_SEL = 0b10 /* 角速度满量程选择 0-3*/) noexcept
        : I2C{SCL, SDA, uint8_t(0xD0 + (AD0 << 1))}
    {
        write_reg(REGS::PWR_MGMT_1, gyroscope_enable);     // 解除休眠，是否启用陀螺仪作为时钟源
        write_reg(REGS::PWR_MGMT_2, 0x00);                 // 六轴都工作
        write_reg(REGS::SMPLRT_DIV, SMPRT_DIV - 1);        // 设置采样率
        write_reg(REGS::CONFIG, 0x00);                     // 平滑地滤波
        write_reg(REGS::ACCEL_CONFIG, ACCEL_AFS_SCL << 3); // 4g 满量程
        write_reg(REGS::GYRO_CONFIG, GYRO_FS_SEL << 3);    // 1000°/s 满量程
    };
    inline void awake() noexcept
    {
        write_reg(REGS::PWR_MGMT_1, read_reg(REGS::PWR_MGMT_1) & 0b1011'1111);
    }

    inline void sleep() noexcept
    {
        write_reg(REGS::PWR_MGMT_1, read_reg(REGS::PWR_MGMT_1) | 0b0100'0000);
    }

    inline auto get_id() noexcept
    {
        return read_reg(REGS::WHO_AM_I);
    }

    inline Data get_data() noexcept
    {
        Data merge_rslt;

        auto data_acc = read_reg<6>(REGS::ACCEL_XOUT_H);
        auto data_gyro = read_reg<6>(REGS::GYRO_XOUT_H);

        merge_rslt.acc_x = (uint16_t(std::get<0>(data_acc)) << 8) | std::get<1>(data_acc);
        merge_rslt.acc_y = (uint16_t(std::get<2>(data_acc)) << 8) | std::get<3>(data_acc);
        merge_rslt.acc_z = (uint16_t(std::get<4>(data_acc)) << 8) | std::get<5>(data_acc);

        merge_rslt.gyro_x = (uint16_t(std::get<0>(data_gyro)) << 8) | std::get<1>(data_gyro);
        merge_rslt.gyro_y = (uint16_t(std::get<2>(data_gyro)) << 8) | std::get<3>(data_gyro);
        merge_rslt.gyro_z = (uint16_t(std::get<4>(data_gyro)) << 8) | std::get<5>(data_gyro);

        return merge_rslt;
    }
};

EMBMARTIN_STM32F10X_NAMESPACE_END

#endif // EMBMARTIN_I2C_H