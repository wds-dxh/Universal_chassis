/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-17 16:34:08
 * @Description: 示例测试程序，展示如何使用 CarController 控制小车进行速度模式和位置模式运动
 */

#include <Arduino.h>
#include "StepperMotor/StepperMotor.h"
#include "CarController/CarController.h"
#include "KinematicsModel/KinematicsModel.h"
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 初始化 ESP32 硬件串口（示例使用 Serial00）
HardwareSerial Serial00(0);

// 创建步进电机实例，分别对应小车的四个轮（编号1~4）
StepperMotor motor0(0, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor1(1, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor2(2, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor3(3, &Serial00, ChecksumType::FIXED, 1000);
StepperMotor motor4(4, &Serial00, ChecksumType::FIXED, 1000);

// 创建普通轮运动学模型实例：参数为轮子半径 0.1m 和左右轮距 0.3m
NormalWheelKinematics normalKinematics(0.08f, 0.6f);

// 创建 CarController 实例，传入四个步进电机及运动学模型
CarController carController(&motor1, &motor2, &motor3, &motor4, &motor0, &normalKinematics);

void setup() {
    
    Serial00.begin(115200, SERIAL_8N1, RX, TX);
    
    Serial.begin(115200);
    //一直循环直到Serial 初始化成功
    // while (!Serial.available()) { vTaskDelay(10); }

    // 设置初始细分和加速度// 
    carController.configure(CarControllerConfig{10.0f, 256*6, 1.0f});   //0表示256细分,6是减速比


}

void loop() {

    while (!Serial.available()) { vTaskDelay(10); }
    String input = Serial.readStringUntil('\n');



    bool output = motor1.modifySubdivision(0, true);
    Serial.print("motor1 subdivision: ");
    Serial.println(output);

    output = motor2.modifySubdivision(0, true);
    Serial.print("motor2 subdivision: ");
    Serial.println(output);

    output = motor3.modifySubdivision(0, true);
    Serial.print("motor3 subdivision: ");
    Serial.println(output);
     
    output = motor4.modifySubdivision(0, true);
    Serial.print("motor4 subdivision: ");
    Serial.println(output);

   

    bool result = false;

    // --------------------------
    // CarController 速度模式示例
    // Serial.println("set speed");

    // result = carController.setSpeed(1, 0, 0, 5000);
    // Serial.print("speed mode result: ");
    // Serial.println(result ? "success" : "failure");

    // --------------------------
    // CarController 位置模式示例
    Serial.println("set position");
    // 控制小车前进 1.0 m（位置模式下，旋转角度为 0）
    result = carController.moveDistance(0.56, 0, 0);
    Serial.print("position mode result: ");
    Serial.println(result ? "success" : "failure");

    // // 获取当前小车状态
    // CarState state = carController.getCarState();
    // Serial.print("当前各轮转速反馈: ");
    // Serial.print(state.wheelSpeeds[0]); Serial.print("  ");
    // Serial.print(state.wheelSpeeds[1]); Serial.print("  ");
    // Serial.print(state.wheelSpeeds[2]); Serial.print("  ");
    // Serial.println(state.wheelSpeeds[3]);

}

