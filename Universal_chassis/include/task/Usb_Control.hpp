#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "CarController/CarController.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 定义 USB 传输 JSON 缓冲区大小
#define USB_JSON_BUFFER_SIZE 256

/**
 * @brief USB 控制类
 *
 * 通过 USB 虚拟串口实现与主机之间的 JSON 数据交互，用于主动下发控制命令以及状态请求/自动发布状态信息。
 */
class UsbControl {
public:
    /**
     * @brief 构造函数，传入 CarController 对象指针
     */
    explicit UsbControl(CarController* carCtrl)
        : carController(carCtrl), autoSendInterval(0) {}

    /**
     * @brief 初始化 USB 控制
     *
     * 启动一个 FreeRTOS 任务持续监听 USB 虚拟串口数据。
     */
    void begin();

    /**
     * @brief 发布当前小车状态到 USB（Serial）
     *
     * 获取 CarController 的状态信息，并以 JSON 格式通过虚拟串口发送出去。
     */
    void publishStatus();

    /**
     * @brief 设置自动发送状态的时间间隔
     * @param interval_ms 间隔毫秒数（设置为0表示关闭自动发送）
     */
    void setAutoSendInterval(uint32_t interval_ms) {
        autoSendInterval = interval_ms;
    }

private:
    CarController* carController;
    // 自动发送状态的时间间隔（单位：毫秒），非0表示启用自动发布状态
    uint32_t autoSendInterval;
    // USB 控制任务句柄
    TaskHandle_t usbTaskHandle = nullptr;
};

////////////////////// 实现部分 //////////////////////

void UsbControl::begin() {
    // 创建 FreeRTOS 任务，用于一直监听 USB 虚拟串口数据
    xTaskCreate(
        [](void* param) {
            UsbControl* control = static_cast<UsbControl*>(param);
            uint32_t lastAutoSendTime = millis();
            for (;;) {
                // 检查是否有串口数据
                if (Serial.available()) {
                    // 读取一行（以换行符 '\n' 为终止标志）
                    String input = Serial.readStringUntil('\n');
                    input.trim(); // 去除前后空白
                    if (input.length() > 0) {
                        // 解析 JSON 数据
                        StaticJsonDocument<USB_JSON_BUFFER_SIZE> doc;
                        DeserializationError error = deserializeJson(doc, input);
                        if (error) {
                            Serial.print("USB JSON parse error: ");
                            Serial.println(error.c_str());
                        } else {
                            const char* command = doc["command"];
                            if (command != nullptr) {
                                if (strcmp(command, "speed") == 0) {
                                    float vx = doc["vx"] | 0.0;
                                    float vy = doc["vy"] | 0.0;
                                    float omega = doc["omega"] | 0.0;
                                    uint32_t duration = doc["duration"] | 1000;
                                    float acceleration = doc["acceleration"] | 10.0;
                                    Serial.println("USB: Executing speed command");
                                    control->carController->setSpeed(vx, vy, omega, duration, acceleration);
                                } else if (strcmp(command, "move") == 0) {
                                    float dx = doc["dx"] | 0.0;
                                    float dy = doc["dy"] | 0.0;
                                    float dtheta = doc["dtheta"] | 0.0;
                                    float acceleration = doc["acceleration"] | 10.0;
                                    uint16_t subdivision = doc["subdivision"] | 256;
                                    Serial.println("USB: Executing move command");
                                    control->carController->moveDistance(dx, dy, dtheta, acceleration, subdivision);
                                } else if (strcmp(command, "stop") == 0) {
                                    Serial.println("USB: Executing stop command");
                                    control->carController->stop();
                                } else if (strcmp(command, "status") == 0) {
                                    Serial.println("USB: Status request received");
                                    control->publishStatus();
                                } else if (strcmp(command, "auto") == 0) {
                                    // 设置自动发送状态的间隔，单位毫秒
                                    uint32_t interval = doc["interval"] | 0;
                                    if (interval > 0) {
                                        Serial.print("USB: Set auto status interval to ");
                                        Serial.print(interval);
                                        Serial.println(" ms");
                                    } else {
                                        Serial.println("USB: Auto status disabled");
                                    }
                                    control->setAutoSendInterval(interval);
                                } else {
                                    Serial.print("USB: Unknown command: ");
                                    Serial.println(command);
                                }
                            } else {
                                Serial.println("USB: No command field in JSON");
                            }
                        }
                    }
                }
                
                // 如果自动发送功能启用，则定期发布状态
                if (control->autoSendInterval > 0) {
                    uint32_t now = millis();
                    if (now - lastAutoSendTime >= control->autoSendInterval) {
                        control->publishStatus();
                        lastAutoSendTime = now;
                    }
                }
                
                // 小延时以释放任务
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        },
        "usbControlTask",
        4096,
        this,
        1,
        &usbTaskHandle
    );
}

void UsbControl::publishStatus() {
    // 获取当前小车状态
    CarState state = carController->getCarState();
    StaticJsonDocument<USB_JSON_BUFFER_SIZE> doc;
    doc["vx"] = state.vx;
    doc["vy"] = state.vy;
    doc["omega"] = state.omega;
    JsonArray speeds = doc.createNestedArray("wheelSpeeds");
    for (auto speed : state.wheelSpeeds) {
        speeds.add(speed);
    }
    char buffer[USB_JSON_BUFFER_SIZE];
    size_t n = serializeJson(doc, buffer);
    // 将 JSON 状态数据发送到 USB 虚拟串口
    Serial.println(buffer);
}
