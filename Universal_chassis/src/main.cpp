/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-17 16:34:08
 * @Description: 示例测试程序，用于测试 StepperMotor 驱动各接口
 * 本示例通过串口打印调用情况，可对照串口发送的数据进行调试
 */

#include <Arduino.h>
#include "StepperMotor/StepperMotor.h"
#include <vector>

// 创建一个 StepperMotor 实例，电机地址默认 1，使用 Serial 端口，校验方式为 FIXED，超时 1000 毫秒
// StepperMotor motor(1, &Serial, ChecksumType::FIXED, 1000);  
StepperMotor motor(1, &Serial1, ChecksumType::FIXED, 1000);

void setup() {
    //初始化usb引脚为串口
    Serial.begin(115200);
    // 等待串口准备完成
    while (!Serial) { vTaskDelay(10); }
    delay(1000);
}

void loop() {
    bool result;

    Serial.println("Calling enableMotor(true, false)");
    result = motor.enableMotor(true, false);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling setSpeedMode(CCW, 1500, 8, false)");
    result = motor.setSpeedMode(1, 1500, 8, false); // 1 表示 CCW
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling setPositionMode(CW, 1200, 5, 32000, true, false)");
    result = motor.setPositionMode(0, 1200, 5, 32000, true, false); // 0 表示 CW，绝对模式 true
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling stopMotor(false)");
    result = motor.stopMotor(false);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling syncMove()");
    result = motor.syncMove();
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling modifySubdivision(7, true)");
    result = motor.modifySubdivision(7, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling modifyMotorID(16, true)");
    result = motor.modifyMotorID(16, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling switchControlMode(1, true) // 0x01=open loop");
    result = motor.switchControlMode(0x01, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling modifyOpenLoopCurrent(1000, false)");
    result = motor.modifyOpenLoopCurrent(1000, false);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    // 示例：构造一组虚拟的驱动配置参数数据
    std::vector<uint8_t> dummyConfig = {0x10, 0x00, 0x0B, 0xB8, 0x0F, 0xA0, 0x01, 0x00, 0x01, 0x01, 0x00, 0x28, 0x0F, 0xA0};
    Serial.println("Calling modifyDriverConfig(dummyConfig, true)");
    result = motor.modifyDriverConfig(dummyConfig, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling modifyPIDParameters(62000, 100, 62000, false)");
    result = motor.modifyPIDParameters(62000, 100, 62000, false);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling storeSpeedModeParameters(CCW, 1500, 10, true, true)");
    result = motor.storeSpeedModeParameters(1, 1500, 10, true, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling modifyInputSpeedScaling(true, true)");
    result = motor.modifyInputSpeedScaling(true, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    Serial.println("Calling readFirmwareVersion()");
    uint8_t firmware, hardware;
    result = motor.readFirmwareVersion(firmware, hardware);
    Serial.print("Firmware: ");
    Serial.print(firmware, HEX);
    Serial.print(" Hardware: ");
    Serial.println(hardware, HEX);
    delay(3000);
}

