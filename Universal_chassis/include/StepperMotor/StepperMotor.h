#pragma once

#include <cstdint>
#include <vector>
#include "HardwareSerial.h"   // ESP32平台的串口对象头文件

/**
 * @brief 步进电机控制类
 *
 * 本类实现了通过串口通讯控制步进电机的各项指令功能，
 * 包括电机使能、速度模式、位置模式、立即停止、多机同步运动等功能；
 * 另外还提供了读取电机参数（如固件版本、相电阻、编码器数值、PID参数等）的接口。
 * 所有命令数据均按 Emm42_V5.0 闭环驱动通讯协议进行组织与解析。
 * 项目基于 ESP32 PIO 框架开发，适用于嵌入式环境。
 */

// 定义校验方式枚举，明确校验类型
enum class ChecksumType {
    FIXED,  // 固定校验，默认使用 0x6B
    XOR, 
    CRC8
};

// 驱动配置参数结构体封装
// 注意：返回的命令包含 21 个配置参数，其中包括通讯校验方式（固定为 0x6B）
struct DriverConfig {
    uint8_t motorType;                     // 电机类型: 25表示1.8°电机, 50表示0.9°电机
    uint8_t pulseControlMode;              // 脉冲端口控制模式，如 PUL_FOC
    uint8_t commPortMode;                  // 通讯端口复用模式，如 UART_FUN
    uint8_t enPinEffectiveLevel;           // En引脚的有效电平
    uint8_t dirPinEffectiveDirection;      // Dir引脚的有效方向
    uint16_t subdivision;                  // 细分值，0x00 表示256，其它表示对应数值
    bool subdivisionInterpolation;         // 细分插补功能是否使能
    bool autoSleep;                        // 自动熄屏功能状态
    uint16_t openLoopCurrent;              // 开环模式工作电流 (mA)
    uint16_t closedLoopMaxCurrent;         // 闭环模式堵转时的最大电流 (mA)
    uint16_t maxOutputVoltage;             // 闭环模式最大输出电压 (mV)
    uint32_t serialBaudRate;               // 串口波特率
    uint32_t canCommRate;                  // CAN通讯速率
    uint8_t id;                            // 电机ID地址
    uint8_t commChecksum;                  // 通讯校验方式（固定为0x6B）
    uint8_t cmdResponse;                   // 控制命令应答设置（只回复确认收到命令）
    bool stallProtectionEnabled;           // 堵转保护功能是否启用
    uint16_t stallThresholdSpeed;          // 堵转保护转速阈值 (RPM)
    uint16_t stallThresholdCurrent;        // 堵转保护电流阈值 (mA)
    uint16_t stallDetectionTime;           // 堵转保护检测时间阈值 (ms)
    float positionArrivalWindow;           // 位置到达窗口 (角度)
};

// 系统状态参数结构体封装
// 根据协议，返回的数据包含 9 个参数，其中将状态标志拆分为就绪状态和电机状态
struct SystemStatus {
    uint16_t busVoltage;                 // 总线电压 (mV)
    uint16_t phaseCurrent;               // 总线相电流 (mA)
    uint16_t calibratedEncoderValue;     // 校准后编码器值
    int32_t targetPosition;              // 电机目标位置
    int16_t realTimeSpeed;               // 电机实时转速 (RPM)
    int32_t realTimePosition;            // 电机实时位置
    int32_t positionError;               // 电机位置误差
    uint8_t readyStatus;                 // 就绪状态标志位（编码器就绪、校准表就绪、正在/回零状态等）
    uint8_t motorStatus;                 // 电机状态标志位（使能、到位、堵转、堵转保护等）
};

class StepperMotor {
public:
    /**
     * @brief 构造函数
     * @param motorAddr 电机地址，范围 1-255（0 表示广播地址）
     * @param serial ESP32硬件串口对象，用于后续串口通讯
     * @param checksumType 校验方式，默认使用固定校验（FIXED，校验字节为 0x6B）
     * @param timeout_ms 超时等待回复的时间（毫秒），默认100ms
     */
    StepperMotor(uint8_t motorAddr, HardwareSerial* serial, ChecksumType checksumType = ChecksumType::FIXED, uint32_t timeout_ms = 100);

    /**
     * @brief 电机使能控制
     * @param enable true 表示使能电机，false 表示关闭电机
     * @param sync 多机同步标志，true 表示等待同步启动
     * @return 成功返回 true，失败返回 false
     */
    bool enableMotor(bool enable, bool sync = false);

    /**
     * @brief 速度模式控制
     * @param direction 旋转方向：0 表示顺时针 (CW)，1 表示逆时针 (CCW)
     * @param speedRpm 转速（RPM，每分钟转数）
     * @param accelerateLevel 加速度档位（0 表示不使用曲线加减速，其它值代表档位级别）
     * @param sync 多机同步标志
     * @return 成功返回 true，失败返回 false
     */
    bool setSpeedMode(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, bool sync = false);

    /**
     * @brief 位置模式控制
     * @param direction 旋转方向：0 表示顺时针 (CW)，1 表示逆时针 (CCW)
     * @param speedRpm 转速（RPM，每分钟转数）
     * @param accelerateLevel 加速度档位（0 表示不使用曲线加减速，其它值代表档位级别）
     * @param pulse 脉冲数，表示电机转动的脉冲数量
     * @param absolute 模式标志：true 表示绝对位置模式，false 表示相对位置模式
     * @param sync 多机同步标志
     * @return 成功返回 true，失败返回 false
     */
    bool setPositionMode(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, uint32_t pulse, bool absolute, bool sync = false);

    /**
     * @brief 立即停止命令
     * @param sync 多机同步标志
     * @return 成功返回 true，失败返回 false
     */
    bool stopMotor(bool sync = false);

    /**
     * @brief 多机同步运动命令
     * 该命令用于触发所有处于同步状态的电机同时运动。
     * 命令格式：地址 + 0xFF + 0x66 + 校验字节
     * @return 成功返回 true，失败返回 false
     */
    bool syncMove();

/*********************************************************读取电机参数*********************************************************/
    /**
     * @brief 读取固件版本和硬件版本
     * 命令格式：地址 + 0x1F + 校验字节
     * 返回格式：地址 + 0x1F + 固件版本号 + 硬件版本号 + 校验字节
     * @param firmware 输出固件版本号
     * @param hardware 输出硬件版本号
     * @return 成功返回 true，失败返回 false
     */
    bool readFirmwareVersion(uint8_t &firmware, uint8_t &hardware);

    /**
     * @brief 读取相电阻和相电感
     * 命令格式：地址 + 0x20 + 校验字节
     * 返回格式：地址 + 0x20 + 相电阻（2字节）+ 相电感（2字节）+ 校验字节
     * @param resistance 输出相电阻，单位 mΩ
     * @param inductance 输出相电感，单位 uH
     * @return 成功返回 true，失败返回 false
     */
    bool readPhaseResistanceInductance(uint16_t &resistance, uint16_t &inductance);

    /**
     * @brief 读取位置环 PID 参数
     * 命令格式：地址 + 0x21 + 校验字节
     * 返回格式：地址 + 0x21 + Kp（4字节）+ Ki（4字节）+ Kd（4字节）+ 校验字节
     * @param Kp 输出比例参数
     * @param Ki 输出积分参数
     * @param Kd 输出微分参数
     * @return 成功返回 true，失败返回 false
     */
    bool readPIDParameters(uint32_t &Kp, uint32_t &Ki, uint32_t &Kd);

    /**
     * @brief 读取总线电压
     * 命令格式：地址 + 0x24 + 校验字节
     * 返回格式：地址 + 0x24 + 总线电压（2字节）+ 校验字节
     * @param voltage 输出总线电压，单位 mV
     * @return 成功返回 true，失败返回 false
     */
    bool readBusVoltage(uint16_t &voltage);

    /**
     * @brief 读取相电流
     * 命令格式：地址 + 0x27 + 校验字节
     * 返回格式：地址 + 0x27 + 总线相电流（2字节）+ 校验字节
     * @param current 输出相电流，单位 mA
     * @return 成功返回 true，失败返回 false
     */
    bool readPhaseCurrent(uint16_t &current);

    /**
     * @brief 读取经过线性化校准后的编码器值
     * 命令格式：地址 + 0x31 + 校验字节
     * 返回格式：地址 + 0x31 + 编码器值（2字节）+ 校验字节
     * @param encoder 输出经校准后的编码器值
     * @return 成功返回 true，失败返回 false
     */
    bool readCalibratedEncoder(uint16_t &encoder);

    /**
     * @brief 读取输入脉冲数
     * 命令格式：地址 + 0x32 + 校验字节
     * 返回格式：地址 + 0x32 + 符号（1字节）+ 输入脉冲数（4字节）+ 校验字节
     * @param pulse 输出脉冲数（负数代表负值）
     * @return 成功返回 true，失败返回 false
     */
    bool readInputPulse(int32_t &pulse);

    /**
     * @brief 读取电机目标位置
     * 命令格式：地址 + 0x33 + 校验字节
     * 返回格式：地址 + 0x33 + 符号（1字节）+ 目标位置（4字节）+ 校验字节
     * @param position 输出电机目标位置（单位为内码值）
     * @return 成功返回 true，失败返回 false
     */
    bool readTargetPosition(int32_t &position);

    /**
     * @brief 读取实时设定目标位置（开环模式实时位置）
     * @param position 传出电机实时目标位置
     * @return 成功返回 true，失败返回 false
     */
    bool readRealTimeTargetPosition(int32_t &position);

    /**
     * @brief 读取电机实时转速
     * 命令格式：地址 + 0x35 + 校验字节
     * 返回格式：地址 + 0x35 + 符号（1字节）+ 转速（2字节）+ 校验字节
     * @param speed 输出实时转速，单位 RPM
     * @return 成功返回 true，失败返回 false
     */
    bool readRealTimeSpeed(int16_t &speed);

    /**
     * @brief 读取电机实时位置
     * 命令格式：地址 + 0x36 + 校验字节
     * 返回格式：地址 + 0x36 + 符号（1字节）+ 实时位置（4字节）+ 校验字节
     * @param position 输出实时位置（单位为内码值）
     * @return 成功返回 true，失败返回 false
     */
    bool readRealTimePosition(int32_t &position);

    /**
     * @brief 读取电机位置误差
     * 命令格式：地址 + 0x37 + 校验字节
     * 返回格式：地址 + 0x37 + 符号（1字节）+ 位置误差（4字节）+ 校验字节
     * @param error 输出位置误差值（单位为内码值）
     * @return 成功返回 true，失败返回 false
     */
    bool readPositionError(int32_t &error);

    /**
     * @brief 读取电机状态标志位
     * 命令格式：地址 + 0x3A + 校验字节
     * 返回格式：地址 + 0x3A + 状态字节 + 校验字节
     * @param status 输出状态标志字节
     * @return 成功返回 true，失败返回 false
     */
    bool readMotorStatus(uint8_t &status);

    /**
     * @brief 读取驱动配置参数
     * 命令格式：地址 + 0x42 + 0x6C + 校验字节
     * 返回格式：33字节数据，详细参数参见协议说明
     * @param config 输出驱动配置参数结构体
     * @return 成功返回 true，失败返回 false
     */
    bool readDriverConfig(DriverConfig &config);

    /**
     * @brief 读取系统状态参数
     * 命令格式：地址 + 0x43 + 0x7A + 校验字节
     * 返回格式：31字节数据，详细参数参见协议说明
     * @param status 输出系统状态参数结构体
     * @return 成功返回 true，失败返回 false
     */
    bool readSystemStatus(SystemStatus &status);
/***************************************************** 修改电机参数 *****************************************************/
    /**
     * @brief 修改任意细分设置
     * @param subdivision 细分值，0x00 表示 256 细分，其它值表示对应细分数
     * @param store true 表示将修改存储到芯片中，false 表示仅临时修改
     * @return 成功返回 true，失败返回 false
     */
    bool modifySubdivision(uint8_t subdivision, bool store);

    /**
     * @brief 修改电机 ID 地址
     * @param newID 新的电机地址，范围 1-255
     * @param store true 表示将修改存储到芯片中
     * @return 成功返回 true，失败返回 false
     */
    bool modifyMotorID(uint8_t newID, bool store);

    /**
     * @brief 切换开环/闭环模式
     * @param mode 0 表示开环模式，1 表示闭环模式
     * @param store true 表示存储修改
     * @return 成功返回 true，失败返回 false
     */
    bool switchControlMode(uint8_t mode, bool store);

    /**
     * @brief 修改开环模式工作电流
     * @param current 开环工作电流，单位 Ma
     * @param store true 表示存储修改，false 表示不保存
     * @return 成功返回 true，失败返回 false
     */
    bool modifyOpenLoopCurrent(uint16_t current, bool store);

    /**
     * @brief 修改驱动配置参数
     *
     * 传入的参数数据需按照协议格式排列，具体格式请参照通讯协议说明。
     * @param configData 驱动配置参数数据
     * @param store true 表示存储修改
     * @return 成功返回 true，失败返回 false
     */
    bool modifyDriverConfig(const std::vector<uint8_t>& configData, bool store);

    /**
     * @brief 修改位置环 PID 参数
     * @param Kp 新的比例系数
     * @param Ki 新的积分系数
     * @param Kd 新的微分系数
     * @param store true 表示存储配置，false 表示临时修改
     * @return 成功返回 true，失败返回 false
     */
    bool modifyPIDParameters(uint32_t Kp, uint32_t Ki, uint32_t Kd, bool store);

    /**
     * @brief 存储一组速度模式参数
     *
     * 用于设定上电后自动运行的速度模式参数。
     * @param direction 旋转方向：0 表示 CW，1 表示 CCW
     * @param speedRpm 转速，单位 RPM
     * @param accelerateLevel 加速度档位
     * @param enableEn 是否使能 En 引脚控制启停
     * @param store true 表示存储参数，false 表示不保存
     * @return 成功返回 true，失败返回 false
     */
    bool storeSpeedModeParameters(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, bool enableEn, bool store);

    /**
     * @brief 修改通讯控制的输入速度缩小倍数
     *
     * 修改后，发送的速度值将缩小10倍，便于精细控制。
     * @param enable true 表示使能缩小10倍，false 表示禁用
     * @param store true 表示存储配置
     * @return 成功返回 true，失败返回 false
     */
    bool modifyInputSpeedScaling(bool enable, bool store);

private:
    uint8_t motorAddr;          // 电机地址 ID
    HardwareSerial* port;       // ESP32 硬件串口对象
    uint32_t timeout_ms;        // 命令回复超时等待时间（毫秒）
    ChecksumType checksumType;  // 校验方式类型

    /**
     * @brief 内部函数：计算校验字节
     *
     * 根据传入的数据计算校验值，支持固定校验、XOR 校验或 CRC-8 校验。
     * @param data 待校验的数据缓冲区
     * @return 计算后的校验字节
     */
    uint8_t calculateChecksum(const std::vector<uint8_t>& data);

    /**
     * @brief 内部函数：发送命令并接收回复
     *
     * 将构造好的命令数据经串口发送到电机控制器，并等待接收返回的数据。
     * @param command 待发送的命令数据
     * @param response 接收的回复数据
     * @return 成功返回 true，失败返回 false
     */
    bool sendCommand(const std::vector<uint8_t>& command, std::vector<uint8_t>& response);

    /**
     * @brief 内部函数：将 16 位整数转换为字节序列并添加到缓冲区
     * @param buf 数据缓冲区
     * @param value 16 位无符号整数
     */
    void appendUint16(std::vector<uint8_t>& buf, uint16_t value);

    /**
     * @brief 内部函数：将 32 位整数转换为字节序列并添加到缓冲区
     * @param buf 数据缓冲区
     * @param value 32 位无符号整数
     */
    void appendUint32(std::vector<uint8_t>& buf, uint32_t value);

    /**
     * @brief 内部函数：构造命令帧
     *
     * 根据传入的功能码和指令数据构造完整的命令帧
     * 帧格式：地址 + 功能码 + 指令数据 + 校验字节
     * @param funcCode 功能码
     * @param payload 指令数据（可选，如果无则为空）
     * @return 构造好的命令帧
     */
    std::vector<uint8_t> buildFrame(uint8_t funcCode, const std::vector<uint8_t>& payload = {});
};
