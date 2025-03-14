#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "control/ControlManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "utils/Logger.hpp"
#include "config.h"

// 定义 USB 传输 JSON 缓冲区大小
#define USB_JSON_BUFFER_SIZE 256

// 日志标签
#define USB_TAG "USB"

// 串口接收缓冲区大小
#define SERIAL_RX_BUFFER_SIZE 512

/**
 * @brief USB 控制类
 *
 * 通过 USB 虚拟串口实现与主机之间的 JSON 数据交互，用于主动下发控制命令以及状态请求/自动发布状态信息。
 * 使用事件驱动方式处理串口数据，提高性能和响应速度。
 */
class UsbControl {
public:
    /**
     * @brief 构造函数
     */
    explicit UsbControl(uint32_t statusInterval = 100)
        : statusInterval(statusInterval) {
        // 获取控制管理器实例
        controlManager = &ControlManager::getInstance();
    }

    /**
     * @brief 初始化 USB 控制
     *
     * 启动一个 FreeRTOS 任务处理 USB 虚拟串口数据。
     */
    void begin();

    /**
     * @brief 发布当前小车状态到 USB（Serial）
     *
     * 获取控制管理器的状态信息，并以 JSON 格式通过虚拟串口发送出去。
     */
    void publishStatus();

    /**
     * @brief 设置自动发送状态的时间间隔
     * @param interval_ms 间隔毫秒数（设置为0表示关闭自动发送）
     */
    void setStatusInterval(uint32_t interval_ms) {
        statusInterval = interval_ms;
    }
    
    /**
     * @brief 连接到WiFi网络
     * @param ssid WiFi名称
     * @param password WiFi密码
     * @param timeout_ms 连接超时时间（毫秒）
     * @return 连接是否成功
     */
    bool connectToWiFi(const char* ssid, const char* password, int timeout_ms = 10000);

private:
    // 处理接收到的命令
    void processCommand(const String& command);
    
    ControlManager* controlManager;
    // 自动发送状态的时间间隔（单位：毫秒），非0表示启用自动发布状态
    uint32_t statusInterval;
    // USB 控制任务句柄
    TaskHandle_t usbTaskHandle = nullptr;
    // 串口接收缓冲区
    char rxBuffer[SERIAL_RX_BUFFER_SIZE];
    // 串口接收位置
    size_t rxPos = 0;
};

////////////////////// 实现部分 //////////////////////

void UsbControl::begin() {
    Logger::info(USB_TAG, "Initializing USB control interface");
    
    // 创建 FreeRTOS 任务处理 USB 数据
    xTaskCreate(
        [](void* param) {
            UsbControl* control = static_cast<UsbControl*>(param);
            uint32_t lastStatusTime = millis();
            String commandLine;
            
            for (;;) {
                // 处理串口数据 - 更高效的方式
                while (Serial.available()) {
                    char c = Serial.read();
                    
                    // 检测行结束
                    if (c == '\n' || c == '\r') {
                        if (commandLine.length() > 0) {
                            // 处理完整的命令行
                            control->processCommand(commandLine);
                            commandLine = "";
                        }
                    } else {
                        // 添加到当前命令行
                        commandLine += c;
                    }
                }
                
                // 处理自动状态发送
                if (control->statusInterval > 0) {
                    uint32_t now = millis();
                    if (now - lastStatusTime >= control->statusInterval) {
                        control->publishStatus();
                        lastStatusTime = now;
                    }
                }
                
                // 短暂延时，避免占用过多CPU
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "usbControlTask",
        4096,
        this,
        1,
        &usbTaskHandle
    );
}

bool UsbControl::connectToWiFi(const char* ssid, const char* password, int timeout_ms) {
    Logger::info(WIFI_TAG, "Connecting to WiFi: %s", ssid);
    WiFi.begin(ssid, password);
    
    // 等待连接，带超时
    int elapsed = 0;
    while (WiFi.status() != WL_CONNECTED && elapsed < timeout_ms) {
        Logger::debug(WIFI_TAG, ".");
        delay(500);
        elapsed += 500;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Logger::info(WIFI_TAG, "Connected to WiFi. IP: %s", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Logger::error(WIFI_TAG, "Failed to connect to WiFi");
        return false;
    }
}

void UsbControl::processCommand(const String& commandStr) {
    if (commandStr.length() == 0) return;
    
    Logger::debug(USB_TAG, "Processing command: %s", commandStr.c_str());
    
    // 解析 JSON 数据
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, commandStr);
    if (error) {
        // 只在调试模式下输出错误
        Logger::debug(USB_TAG, "JSON parse error: %s", error.c_str());
        return;
    }
    
    const char* command = doc["command"];
    if (!command) {
        Logger::debug(USB_TAG, "No command field in JSON");
        return;
    }
    
    // 处理各种命令
    if (strcmp(command, "speed") == 0) {
        float vx = doc["vx"] | 0.0;
        float vy = doc["vy"] | 0.0;
        float omega = doc["omega"] | 0.0;
        float acceleration = doc["acceleration"] | 10.0;
        Logger::debug(USB_TAG, "Speed command: vx=%.2f, vy=%.2f, omega=%.2f", vx, vy, omega);
        controlManager->setSpeed(vx, vy, omega, acceleration);
    } 
    else if (strcmp(command, "move") == 0) {
        float dx = doc["dx"] | 0.0;
        float dy = doc["dy"] | 0.0;
        float dtheta = doc["dtheta"] | 0.0;
        float speed = doc["speed"] | 1.0;
        float acceleration = doc["acceleration"] | 10.0;
        uint16_t subdivision = doc["subdivision"] | 256;
        Logger::debug(USB_TAG, "Move command: dx=%.2f, dy=%.2f, dtheta=%.2f, speed=%.2f", dx, dy, dtheta, speed);
        controlManager->moveDistance(dx, dy, dtheta, acceleration, speed, subdivision);
    } 
    else if (strcmp(command, "stop") == 0) {
        Logger::debug(USB_TAG, "Stop command");
        controlManager->stop();
    } 
    else if (strcmp(command, "get_status") == 0) {
        Logger::debug(USB_TAG, "Status request");
        publishStatus();
    } 
    else if (strcmp(command, "set_interval") == 0) {
        // 设置自动发送状态的间隔
        uint32_t interval = doc["interval"] | 0;
        setStatusInterval(interval);
        Logger::debug(USB_TAG, "Set status interval: %d ms", interval);
    }
    else if (strcmp(command, "set_wifi") == 0) {
        // 处理WiFi设置命令
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];
        
        if (ssid && password) {
            Logger::info(USB_TAG, "Setting WiFi: SSID=%s", ssid);
            // 尝试连接到新的WiFi
            connectToWiFi(ssid, password);
        } else {
            Logger::warn(USB_TAG, "Invalid WiFi settings");
        }
    }
}

void UsbControl::publishStatus() {
    // 获取当前小车状态 - 从控制管理器获取
    CarState state = controlManager->getCarState();
    JsonDocument doc;
    doc["vx"] = state.vx;
    doc["vy"] = state.vy;
    doc["omega"] = state.omega;
    JsonArray speeds = doc["wheelSpeeds"].to<JsonArray>();
    for (auto speed : state.wheelSpeeds) {
        speeds.add(speed);
    }
    char buffer[USB_JSON_BUFFER_SIZE];
    size_t n = serializeJson(doc, buffer);
    
    // 将 JSON 状态数据发送到 USB 虚拟串口
    Serial.println(buffer);
}
