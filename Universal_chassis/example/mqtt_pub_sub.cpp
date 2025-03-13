/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-17 16:34:08
 * @Description: 示例测试程序，展示如何使用 CarController 以及 MQTT 控制小车运动
 */

#include <Arduino.h>
#include "StepperMotor/StepperMotor.h"
#include "CarController/CarController.h"
#include "KinematicsModel/KinematicsModel.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task/Mqtt_Control.hpp"

// 初始化 ESP32 硬件串口（示例使用 Serial00）
HardwareSerial Serial00(0);

// 创建主控板及四个轮的步进电机实例
StepperMotor motor0(0, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor1(1, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor2(2, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor3(3, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor4(4, &Serial00, ChecksumType::FIXED, 1000);

// 创建普通轮运动学模型实例：例子中轮子半径为 0.08m, 轮距 0.6m
NormalWheelKinematics normalKinematics(0.08f, 0.6f);

// 创建 CarController 对象（传入四个轮及主控板电机和运动学模型）
CarController carController(&motor1, &motor2, &motor3, &motor4, &motor0, &normalKinematics);

// 在全局声明 MQTT 控制对象
MqttControl mqttControl(&carController);

void setup() {
    Serial00.begin(115200, SERIAL_8N1, RX, TX);
    Serial.begin(115200);

    // 启动 MQTT 控制，内部会连接 WiFi、MQTT 并启动 FreeRTOS 任务
    mqttControl.begin(100);
}

void loop() {
    // 主 loop 中无需过多处理，MQTT 任务已在后台运行
    vTaskDelay(pdMS_TO_TICKS(1000));
}

