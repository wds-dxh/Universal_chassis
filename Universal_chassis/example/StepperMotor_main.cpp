/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-23 20:09:28
 * @Description: 
 * Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
 */
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
HardwareSerial Serial00(0);
StepperMotor motor0(0, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor1(1, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor2(2, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor3(3, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor4(4, &Serial00, ChecksumType::FIXED, 1000);

void setup() {
    Serial00.begin(115200, SERIAL_8N1, RX, TX);


    //初始化usb引脚为串口
    Serial.begin(115200);
    // 等待串口准备完成
    // while (!Serial) { vTaskDelay(10); }
    delay(1000);


    bool result;

    //使能所有电机
    Serial.println("Calling enableMotor(true, false)");
    result = motor0.enableMotor(true, false);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);

    //电机1,2顺时针，电机3,4逆时针。
    Serial.println("Calling setSpeedMode(0, 1000, 100, true)");
    result = motor1.setSpeedMode(0, 2000, 100, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");

    Serial.println("Calling setSpeedMode(1, 1000, 100, true)");
    result = motor2.setSpeedMode(0, 2000, 100, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");

    Serial.println("Calling setSpeedMode(2, 1000, 100, true)");
    result = motor3.setSpeedMode(1, 2000, 100, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");

    Serial.println("Calling setSpeedMode(3, 1000, 100, true)");
    result = motor4.setSpeedMode(1, 2000, 100, true);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    
    //同步运动
    Serial.println("Calling syncMove()");
    result = motor0.syncMove();
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");

    //持续运动12s
    delay(3000);
    delay(3000);
    delay(3000);
    delay(3000);
    delay(3000);
    delay(3000);
    delay(3000);
    delay(3000);


    // //立即停止
    result = motor0.stopMotor(false);
    Serial.print("Result: ");
    Serial.println(result ? "Success" : "Failure");
    delay(3000);


    // //设置细分为256
    // Serial.println("Calling modifySubdivision(256, true)");
    // result = motor0.modifySubdivision(256, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);
}

void loop() {
    // bool result;

    // Serial.println("Calling enableMotor(true, false)");
    // result = motor.enableMotor(true, false);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling setSpeedMode(CCW, 1500, 8, false)");
    // result = motor.setSpeedMode(1, 1500, 8, false); // 1 表示 CCW
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling setPositionMode(CW, 1200, 5, 32000, true, false)");
    // result = motor.setPositionMode(0, 1200, 5, 32000, true, false); // 0 表示 CW，绝对模式 true
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling stopMotor(false)");
    // result = motor.stopMotor(false);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling syncMove()");
    // result = motor.syncMove();
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling modifySubdivision(7, true)");
    // result = motor.modifySubdivision(7, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling modifyMotorID(16, true)");
    // result = motor.modifyMotorID(16, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling switchControlMode(1, true) // 0x01=open loop");
    // result = motor.switchControlMode(0x01, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling modifyOpenLoopCurrent(1000, false)");
    // result = motor.modifyOpenLoopCurrent(1000, false);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // // 示例：构造一组虚拟的驱动配置参数数据
    // std::vector<uint8_t> dummyConfig = {0x10, 0x00, 0x0B, 0xB8, 0x0F, 0xA0, 0x01, 0x00, 0x01, 0x01, 0x00, 0x28, 0x0F, 0xA0};
    // Serial.println("Calling modifyDriverConfig(dummyConfig, true)");
    // result = motor.modifyDriverConfig(dummyConfig, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling modifyPIDParameters(62000, 100, 62000, false)");
    // result = motor.modifyPIDParameters(62000, 100, 62000, false);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling storeSpeedModeParameters(CCW, 1500, 10, true, true)");
    // result = motor.storeSpeedModeParameters(1, 1500, 10, true, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling modifyInputSpeedScaling(true, true)");
    // result = motor.modifyInputSpeedScaling(true, true);
    // Serial.print("Result: ");
    // Serial.println(result ? "Success" : "Failure");
    // delay(3000);

    // Serial.println("Calling readFirmwareVersion()");
    // uint8_t firmware, hardware;
    // result = motor.readFirmwareVersion(firmware, hardware);
    // Serial.print("Firmware: ");
    // Serial.print(firmware, HEX);
    // Serial.print(" Hardware: ");
    // Serial.println(hardware, HEX);
    // delay(3000);
}

