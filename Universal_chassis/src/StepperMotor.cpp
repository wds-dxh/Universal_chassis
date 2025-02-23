#include "StepperMotor/StepperMotor.h"
#include <Arduino.h>  // 提供 millis() 和 delay() 等函数

// 构造函数实现
StepperMotor::StepperMotor(uint8_t motorAddr, HardwareSerial* serial, ChecksumType checksumType, uint32_t timeout_ms)
    : motorAddr(motorAddr), port(serial), timeout_ms(timeout_ms), checksumType(checksumType)
{
    // 如果需要，可在此进行串口初始化配置（例如设置波特率）
    // 本示例不做额外配置，按上层传入的 HardwareSerial 对象进行操作
}

// 私有方法：计算校验字节
uint8_t StepperMotor::calculateChecksum(const std::vector<uint8_t>& data) {
    switch (checksumType) {
        case ChecksumType::FIXED:
            return 0x6B;
        case ChecksumType::XOR: {
            uint8_t checksum = 0;
            for (auto byte : data) {
                checksum ^= byte;
            }
            return checksum;
        }
        case ChecksumType::CRC8: {
            // 使用 CRC-8 算法，采用多项式 0x07，初始值为 0
            uint8_t crc = 0;
            for (auto byte : data) {
                crc ^= byte;
                for (int i = 0; i < 8; ++i) {
                    if (crc & 0x80) {
                        crc = (crc << 1) ^ 0x07;
                    } else {
                        crc <<= 1;
                    }
                }
            }
            return crc;
        }
        default:
            return 0x6B;
    }
}

// 私有方法：发送命令并接收回复
bool StepperMotor::sendCommand(const std::vector<uint8_t>& command, std::vector<uint8_t>& response) {
    if (!port) {
        return false;
    }
    
    response.clear();
    
    // 清空接收缓冲区中的数据
    while (port->available()) {
        port->read();
    }
    
    // 发送命令数据
    size_t bytesSent = port->write(command.data(), command.size());
    port->flush(); // 确保数据发送完成
    if (bytesSent != command.size()) {
        return false;
    }
    
    // 等待回复数据，使用超时机制
    unsigned long startTime = millis();
    while (millis() - startTime < timeout_ms) {
        if (port->available() > 0) {
            // 稍作延时，确保数据稳定
            vTaskDelay(10);    //无阻塞延时
            // 读取所有可用的数据
            while (port->available()) {
                int byteRead = port->read();
                if (byteRead >= 0) {
                    response.push_back(static_cast<uint8_t>(byteRead));
                }
            }
            break; // 数据接收完成，跳出循环
        }
        vTaskDelay(1);
    }
    
    if (response.empty()) {
        // 超时未收到数据
        return false;
    }
    
    // 对于回复数据，假定最后一个字节为校验字节，验证其正确性
    if (!response.empty()) {
        std::vector<uint8_t> dataWithoutChecksum(response.begin(), response.end() - 1);
        uint8_t expectedChecksum = calculateChecksum(dataWithoutChecksum);
        uint8_t receivedChecksum = response.back();
        if (expectedChecksum != receivedChecksum) {
            // 校验失败
            return false;
        }
    }
    
    return true;
}

// 私有方法：将 16 位无符号整数转换为字节序列并添加到缓冲区（大端顺序）
void StepperMotor::appendUint16(std::vector<uint8_t>& buf, uint16_t value) {
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

// 私有方法：将 32 位无符号整数转换为字节序列并添加到缓冲区（大端顺序）
void StepperMotor::appendUint32(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

// 新增函数实现：构造命令帧
// 内部函数：构造命令帧
std::vector<uint8_t> StepperMotor::buildFrame(uint8_t funcCode, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> frame;
    // 添加地址
    frame.push_back(motorAddr);
    // 添加功能码
    frame.push_back(funcCode);
    // 添加指令数据（如果有）
    if (!payload.empty()) {
        frame.insert(frame.end(), payload.begin(), payload.end());
    }
    // 计算校验字节并追加
    uint8_t checksum = calculateChecksum(frame);
    frame.push_back(checksum);
    return frame;
}
/*************************************************** 写入命令 *************************************/
// 实现电机使能控制命令
bool StepperMotor::enableMotor(bool enable, bool sync) {
    std::vector<uint8_t> payload;
    payload.push_back(0xAB);                     // 固定命令数据标识
    payload.push_back(enable ? 0x01 : 0x00);       // 使能状态：1-使能，0-失能
    payload.push_back(sync ? 0x01 : 0x00);           // 多机同步标志：1-同步，0-立即执行
    
    auto frame = buildFrame(0xF3, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) {
        return false;
    }
    // 期望回复：地址 + 0xF3 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xF3 || response[2] != 0x02) {
        return false;
    }
    return true;
}

// 实现速度模式控制命令
bool StepperMotor::setSpeedMode(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, bool sync) {
    std::vector<uint8_t> payload;
    payload.push_back(direction);          // 旋转方向：0-CW，1-CCW
    appendUint16(payload, speedRpm);         // 速度（2字节大端序）
    payload.push_back(accelerateLevel);      // 加速度档位
    payload.push_back(sync ? 0x01 : 0x00);     // 多机同步标志
    
    auto frame = buildFrame(0xF6, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) {
        return false;
    }
    // 期望回复：地址 + 0xF6 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xF6 || response[2] != 0x02) {
        return false;
    }
    return true;
}

// 实现位置模式控制命令
bool StepperMotor::setPositionMode(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, uint32_t pulse, bool absolute, bool sync) {
    std::vector<uint8_t> payload;
    payload.push_back(direction);          // 旋转方向：0-CW，1-CCW
    appendUint16(payload, speedRpm);         // 速度（2字节大端序）
    payload.push_back(accelerateLevel);      // 加速度档位
    appendUint32(payload, pulse);            // 脉冲数（4字节大端序）
    payload.push_back(absolute ? 0x01 : 0x00); // 模式标志：1-绝对，0-相对
    payload.push_back(sync ? 0x01 : 0x00);     // 多机同步标志
    
    auto frame = buildFrame(0xFD, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) {
        return false;
    }
    // 期望回复：地址 + 0xFD + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xFD || response[2] != 0x02) {
        return false;
    }
    return true;
}

// 实现立即停止命令
bool StepperMotor::stopMotor(bool sync) {
    std::vector<uint8_t> payload;
    payload.push_back(0x98);                     // 固定停止命令数据
    payload.push_back(sync ? 0x01 : 0x00);         // 多机同步标志
    
    auto frame = buildFrame(0xFE, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) {
        return false;
    }
    // 期望回复：地址 + 0xFE + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xFE || response[2] != 0x02) {
        return false;
    }
    return true;
}

// 实现多机同步运动命令
bool StepperMotor::syncMove() {
    // 构造多机同步运动命令帧：地址 + 0xFF + 0x66 + 校验字节
    std::vector<uint8_t> payload;
    payload.push_back(0x66);  // 固定命令数据
    auto frame = buildFrame(0xFF, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) {
        return false;
    }
    // 期望回复：地址 + 0xFF + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xFF || response[2] != 0x02) {
        return false;
    }
    return true;
}

/***************************************************读取命令 *************************************/
// 读取固件版本和硬件版本
bool StepperMotor::readFirmwareVersion(uint8_t &firmware, uint8_t &hardware) {
    auto frame = buildFrame(0x1F);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x1F + 固件版本 + 硬件版本 + 校验字节，共 5 字节
    if (response.size() != 5 || response[0] != motorAddr || response[1] != 0x1F)
        return false;
    firmware = response[2];
    hardware = response[3];
    return true;
}

// 读取相电阻和相电感
bool StepperMotor::readPhaseResistanceInductance(uint16_t &resistance, uint16_t &inductance) {
    auto frame = buildFrame(0x20);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x20 + R_H + R_L + L_H + L_L + 校验字节，共 7 字节
    if (response.size() != 7 || response[0] != motorAddr || response[1] != 0x20)
        return false;
    resistance = (static_cast<uint16_t>(response[2]) << 8) | response[3];
    inductance = (static_cast<uint16_t>(response[4]) << 8) | response[5];
    return true;
}

// 读取位置环 PID 参数
bool StepperMotor::readPIDParameters(uint32_t &Kp, uint32_t &Ki, uint32_t &Kd) {
    auto frame = buildFrame(0x21);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x21 + Kp(4) + Ki(4) + Kd(4) + 校验字节，共 15 字节
    if (response.size() != 15 || response[0] != motorAddr || response[1] != 0x21)
        return false;
    Kp = (static_cast<uint32_t>(response[2]) << 24) |
         (static_cast<uint32_t>(response[3]) << 16) |
         (static_cast<uint32_t>(response[4]) << 8)  | response[5];
    Ki = (static_cast<uint32_t>(response[6]) << 24) |
         (static_cast<uint32_t>(response[7]) << 16) |
         (static_cast<uint32_t>(response[8]) << 8)  | response[9];
    Kd = (static_cast<uint32_t>(response[10]) << 24) |
         (static_cast<uint32_t>(response[11]) << 16) |
         (static_cast<uint32_t>(response[12]) << 8)  | response[13];
    return true;
}

// 读取总线电压
bool StepperMotor::readBusVoltage(uint16_t &voltage) {
    auto frame = buildFrame(0x24);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x24 + V_H + V_L + 校验字节，共 5 字节
    if (response.size() != 5 || response[0] != motorAddr || response[1] != 0x24)
        return false;
    voltage = (static_cast<uint16_t>(response[2]) << 8) | response[3];
    return true;
}

// 读取相电流
bool StepperMotor::readPhaseCurrent(uint16_t &current) {
    auto frame = buildFrame(0x27);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x27 + current_H + current_L + 校验字节，共 5 字节
    if (response.size() != 5 || response[0] != motorAddr || response[1] != 0x27)
        return false;
    current = (static_cast<uint16_t>(response[2]) << 8) | response[3];
    return true;
}

// 读取经过线性校准后的编码器值
bool StepperMotor::readCalibratedEncoder(uint16_t &encoder) {
    auto frame = buildFrame(0x31);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x31 + encoder_H + encoder_L + 校验字节，共 5 字节
    if (response.size() != 5 || response[0] != motorAddr || response[1] != 0x31)
        return false;
    encoder = (static_cast<uint16_t>(response[2]) << 8) | response[3];
    return true;
}

// 读取输入脉冲数
bool StepperMotor::readInputPulse(int32_t &pulse) {
    auto frame = buildFrame(0x32);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x32 + 符号（1字节）+ 4字节脉冲数 + 校验字节，共 8 字节
    if (response.size() != 8 || response[0] != motorAddr || response[1] != 0x32)
        return false;
    uint32_t raw = (static_cast<uint32_t>(response[3]) << 24) |
                   (static_cast<uint32_t>(response[4]) << 16) |
                   (static_cast<uint32_t>(response[5]) << 8)  | response[6];
    pulse = (response[2] == 0x01) ? -static_cast<int32_t>(raw) : static_cast<int32_t>(raw);
    return true;
}

// 读取电机目标位置
bool StepperMotor::readTargetPosition(int32_t &position) {
    auto frame = buildFrame(0x33);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x33 + 符号（1字节）+ 4字节位置值 + 校验字节，共 8 字节
    if (response.size() != 8 || response[0] != motorAddr || response[1] != 0x33)
        return false;
    uint32_t raw = (static_cast<uint32_t>(response[3]) << 24) |
                   (static_cast<uint32_t>(response[4]) << 16) |
                   (static_cast<uint32_t>(response[5]) << 8)  | response[6];
    position = (response[2] == 0x01) ? -static_cast<int32_t>(raw) : static_cast<int32_t>(raw);
    return true;
}

// 读取电机实时转速
bool StepperMotor::readRealTimeSpeed(int16_t &speed) {
    auto frame = buildFrame(0x35);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x35 + 符号（1字节）+ 2字节转速 + 校验字节，共 6 字节
    if (response.size() != 6 || response[0] != motorAddr || response[1] != 0x35)
        return false;
    uint16_t raw = (static_cast<uint16_t>(response[3]) << 8) | response[4];
    speed = (response[2] == 0x01) ? -static_cast<int16_t>(raw) : static_cast<int16_t>(raw);
    return true;
}

// 读取电机实时位置
bool StepperMotor::readRealTimePosition(int32_t &position) {
    auto frame = buildFrame(0x36);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x36 + 符号（1字节）+ 4字节位置值 + 校验字节，共 8 字节
    if (response.size() != 8 || response[0] != motorAddr || response[1] != 0x36)
        return false;
    uint32_t raw = (static_cast<uint32_t>(response[3]) << 24) |
                   (static_cast<uint32_t>(response[4]) << 16) |
                   (static_cast<uint32_t>(response[5]) << 8)  | response[6];
    position = (response[2] == 0x01) ? -static_cast<int32_t>(raw) : static_cast<int32_t>(raw);
    return true;
}

// 读取电机位置误差
bool StepperMotor::readPositionError(int32_t &error) {
    auto frame = buildFrame(0x37);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x37 + 符号（1字节）+ 4字节误差值 + 校验字节，共 8 字节
    if (response.size() != 8 || response[0] != motorAddr || response[1] != 0x37)
        return false;
    uint32_t raw = (static_cast<uint32_t>(response[3]) << 24) |
                   (static_cast<uint32_t>(response[4]) << 16) |
                   (static_cast<uint32_t>(response[5]) << 8)  | response[6];
    error = (response[2] == 0x01) ? -static_cast<int32_t>(raw) : static_cast<int32_t>(raw);
    return true;
}

// 读取电机状态标志位
bool StepperMotor::readMotorStatus(uint8_t &status) {
    auto frame = buildFrame(0x3A);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回：地址 + 0x3A + 状态字节 + 校验字节，共 4 字节
    if (response.size() != 4 || response[0] != motorAddr || response[1] != 0x3A)
        return false;
    status = response[2];
    return true;
}

// 读取驱动配置参数
bool StepperMotor::readDriverConfig(DriverConfig &config) {
    auto frame = buildFrame(0x42, {0x6C});
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回总字节数约33字节（协议要求返回21个配置参数）
    if (response.size() < 33 || response[0] != motorAddr || response[1] != 0x42)
        return false;
    // 以下映射基于协议描述（各字段可能根据实际硬件文档调整）
    config.motorType = response[2];
    config.pulseControlMode = response[3];
    config.commPortMode = response[4];
    config.enPinEffectiveLevel = response[5];
    config.dirPinEffectiveDirection = response[6];
    config.subdivision = (static_cast<uint16_t>(response[7]) << 8) | response[8];
    config.subdivisionInterpolation = (response[9] != 0);
    config.autoSleep = (response[10] != 0);
    config.openLoopCurrent = (static_cast<uint16_t>(response[11]) << 8) | response[12];
    config.closedLoopMaxCurrent = (static_cast<uint16_t>(response[13]) << 8) | response[14];
    config.maxOutputVoltage = (static_cast<uint16_t>(response[15]) << 8) | response[16];
    config.serialBaudRate = (static_cast<uint32_t>(response[17]) << 24) | (static_cast<uint32_t>(response[18]) << 16)
                            | (static_cast<uint32_t>(response[19]) << 8) | response[20];
    config.canCommRate = (static_cast<uint32_t>(response[21]) << 24) | (static_cast<uint32_t>(response[22]) << 16)
                         | (static_cast<uint32_t>(response[23]) << 8) | response[24];
    config.id = response[25];
    config.commChecksum = response[26];
    config.cmdResponse = response[27];
    config.stallProtectionEnabled = (response[28] != 0);
    config.stallThresholdSpeed = (static_cast<uint16_t>(response[29]) << 8) | response[30];
    config.stallThresholdCurrent = (static_cast<uint16_t>(response[31]) << 8) | response[32];
    return true;
}

// 读取系统状态参数
bool StepperMotor::readSystemStatus(SystemStatus &status) {
    auto frame = buildFrame(0x43, {0x7A});
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望返回约31字节数据（协议中说明返回9个参数）
    if (response.size() < 31 || response[0] != motorAddr || response[1] != 0x43)
        return false;
    // 以下字段按照协议描述依次解析（具体字节偏移根据实际返回数据可能需要调整）
    status.busVoltage = (static_cast<uint16_t>(response[2]) << 8) | response[3];
    status.phaseCurrent = (static_cast<uint16_t>(response[4]) << 8) | response[5];
    status.calibratedEncoderValue = (static_cast<uint16_t>(response[6]) << 8) | response[7];
    status.targetPosition = (static_cast<int32_t>(response[8]) << 24) | (static_cast<int32_t>(response[9]) << 16)
                            | (static_cast<int32_t>(response[10]) << 8) | response[11];
    status.realTimeSpeed = (static_cast<uint16_t>(response[12]) << 8) | response[13];
    status.realTimePosition = (static_cast<int32_t>(response[14]) << 24) | (static_cast<int32_t>(response[15]) << 16)
                             | (static_cast<int32_t>(response[16]) << 8) | response[17];
    status.positionError = (static_cast<int32_t>(response[18]) << 24) | (static_cast<int32_t>(response[19]) << 16)
                           | (static_cast<int32_t>(response[20]) << 8) | response[21];
    status.readyStatus = response[22];
    status.motorStatus = response[23];
    return true;
}

// 读取电机实时目标位置
bool StepperMotor::readRealTimeTargetPosition(int32_t &targetPosition) {
    // 构造命令帧，功能码为 0x33，无额外 payload 数据
    auto frame = buildFrame(0x33);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response))
        return false;
    
    // 检查返回数据长度：应为 8 字节（地址 + 0x33 + 符号 + 4字节目标位置 + 校验字节）
    if (response.size() != 8 || response[0] != motorAddr || response[1] != 0x33)
        return false;
    
    uint8_t sign = response[2];  // 第3个字节为符号
    // 提取后面4字节为目标位置数据（大端顺序）
    uint32_t rawValue = (static_cast<uint32_t>(response[3]) << 24) |
                        (static_cast<uint32_t>(response[4]) << 16) |
                        (static_cast<uint32_t>(response[5]) << 8)  |
                        (static_cast<uint32_t>(response[6]));
    
    // 根据符号进行转换：0x01 表示负数，0x00 表示正数
    if (sign == 0x01)
        targetPosition = -static_cast<int32_t>(rawValue);
    else if (sign == 0x00)
        targetPosition = static_cast<int32_t>(rawValue);
    else
        return false;  // 非法符号位
    
    return true;
}

/************************************* 修改命令 *************************************/
// 修改任意细分命令
bool StepperMotor::modifySubdivision(uint8_t subdivision, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0x8A); // 子命令：修改细分
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    payload.push_back(subdivision); // 细分值（00表示256细分，其它值为对应细分数）
    auto frame = buildFrame(0x84, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0x84 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0x84 || response[2] != 0x02)
        return false;
    return true;
}

// 修改任意 ID 地址命令
bool StepperMotor::modifyMotorID(uint8_t newID, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0x4B); // 子命令：修改ID地址
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    payload.push_back(newID); // 新的ID地址
    auto frame = buildFrame(0xAE, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0xAE + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xAE || response[2] != 0x02)
        return false;
    return true;
}

// 切换开环/闭环模式命令
bool StepperMotor::switchControlMode(uint8_t mode, bool store) {
    // mode: 0x01 表示开环模式，0x02 表示闭环模式
    std::vector<uint8_t> payload;
    payload.push_back(0x69); // 子命令：切换模式
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    payload.push_back(mode); // 模式标志
    auto frame = buildFrame(0x46, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0x46 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0x46 || response[2] != 0x02)
        return false;
    return true;
}

// 修改开环模式工作电流命令
bool StepperMotor::modifyOpenLoopCurrent(uint16_t current, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0x33); // 子命令：修改开环模式电流
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    payload.push_back(static_cast<uint8_t>((current >> 8) & 0xFF));  // 高字节
    payload.push_back(static_cast<uint8_t>(current & 0xFF));         // 低字节
    auto frame = buildFrame(0x44, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0x44 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0x44 || response[2] != 0x02)
        return false;
    return true;
}

// 修改驱动配置参数命令
bool StepperMotor::modifyDriverConfig(const std::vector<uint8_t>& configData, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0xD1); // 子命令：修改驱动配置参数
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    payload.insert(payload.end(), configData.begin(), configData.end());
    auto frame = buildFrame(0x48, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0x48 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0x48 || response[2] != 0x02)
        return false;
    return true;
}

// 修改位置环 PID 参数命令
bool StepperMotor::modifyPIDParameters(uint32_t Kp, uint32_t Ki, uint32_t Kd, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0xC3); // 子命令：修改位置环 PID 参数
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    appendUint32(payload, Kp);
    appendUint32(payload, Ki);
    appendUint32(payload, Kd);
    auto frame = buildFrame(0x4A, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0x4A + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0x4A || response[2] != 0x02)
        return false;
    return true;
}

// 存储一组速度模式参数命令
bool StepperMotor::storeSpeedModeParameters(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, bool enableEn, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0x1C); // 子命令：存储速度模式参数
    payload.push_back(store ? 0x01 : 0x00); // 存储/清除标志
    payload.push_back(direction);
    appendUint16(payload, speedRpm);
    payload.push_back(accelerateLevel);
    payload.push_back(enableEn ? 0x01 : 0x00);
    auto frame = buildFrame(0xF7, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0xF7 + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0xF7 || response[2] != 0x02)
        return false;
    return true;
}

// 修改通讯控制的输入速度是否缩小10倍输入命令
bool StepperMotor::modifyInputSpeedScaling(bool enable, bool store) {
    std::vector<uint8_t> payload;
    payload.push_back(0x71); // 子命令：修改输入速度缩放
    payload.push_back(store ? 0x01 : 0x00); // 存储标志
    payload.push_back(enable ? 0x01 : 0x00);
    auto frame = buildFrame(0x4F, payload);
    std::vector<uint8_t> response;
    if (!sendCommand(frame, response)) return false;
    // 期望回复：地址 + 0x4F + 0x02 + 校验字节
    if (response.size() < 3 || response[0] != motorAddr || response[1] != 0x4F || response[2] != 0x02)
        return false;
    return true;
}
