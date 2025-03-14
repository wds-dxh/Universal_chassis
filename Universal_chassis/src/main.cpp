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
#include "task/Usb_Control.hpp"
#include "utils/Logger.hpp"
#include "config.h"
#include "control/ControlManager.hpp"

// 初始化 ESP32 硬件串口（示例使用 Serial00）
HardwareSerial Serial00(0);

// 创建主控板及四个轮的步进电机实例
StepperMotor motor0(0, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor1(1, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor2(2, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor3(3, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor4(4, &Serial00, ChecksumType::FIXED, 1000);

// 创建普通轮运动学模型实例：例子中轮子半径为 0.09m, 轮距 0.45m(v1.1)
NormalWheelKinematics normalKinematics(0.09f, 0.45f, 6);

// 创建 CarController 对象（传入四个轮及主控板电机和运动学模型）
CarController carController(&motor1, &motor2, &motor3, &motor4, &motor0, &normalKinematics);

// 在全局声明 MQTT 控制对象
MqttControl mqttControl(10); // 默认1000ms发布一次状态

// 在全局声明 USB 控制对象
UsbControl usbControl(0);

void setup() {
    Serial00.begin(115200, SERIAL_8N1, RX, TX);
    Serial.begin(460800); 
    
    // 初始化日志系统，默认为NONE级别（不输出任何日志）
    // 可以通过编译时定义DEBUG_MODE来启用调试日志
    // #define DEBUG_MODE
    #ifdef DEBUG_MODE
        Logger::init(LOG_LEVEL_DEBUG);
    #else
        Logger::init(LOG_LEVEL_NONE);  // 默认不输出任何日志
    #endif
    
    Logger::info("MAIN", "System initializing...");

    
    // 初始化ControlManager
    ControlManager::getInstance().init(&carController);
    
    mqttControl.begin();
    
    usbControl.begin();
    
    // 连接到默认WiFi网络
    usbControl.connectToWiFi(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
    
    Logger::info("MAIN", "System initialized successfully");
}

void loop() {
    // 主 loop 中无需过多处理，MQTT和USB任务已在后台运行
    vTaskDelay(pdMS_TO_TICKS(1000));
}

