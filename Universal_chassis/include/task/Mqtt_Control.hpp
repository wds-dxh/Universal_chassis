#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "CarController/CarController.h"
#include "utils/Logger.hpp"
#include "config.h"

class MqttControl
{
public:
    explicit MqttControl(CarController *carCtrl, uint32_t statusInterval = 1000) 
        : carController(carCtrl), statusInterval(statusInterval) {}

    // 初始化 MQTT 并启动 FreeRTOS 任务
    void begin();

    // 连接到WiFi网络
    bool connectToWiFi(const char* ssid, const char* password, int timeout_ms = 10000);

    // MQTT client 循环函数（如果需要，可在 loop 中调用）
    void loop();

    // 发布当前小车状态到 MQTT
    void publishStatus();

    // 设置状态发布间隔
    void setStatusInterval(uint32_t interval_ms) {
        statusInterval = interval_ms;
    }

    // 获取当前状态发布间隔
    uint32_t getStatusInterval() const {
        return statusInterval;
    }

private:
    // MQTT 接收到消息的回调函数（static 供 PubSubClient 使用）
    static void mqttCallback(char *topic, byte *payload, unsigned int length);

    // 尝试连接到 MQTT Broker
    void connectMQTT();

    // 静态实例指针，供回调函数使用
    static MqttControl *instance;

    CarController *carController;
    WiFiClient wifiClient;
    PubSubClient mqttClient{wifiClient};

    uint32_t statusInterval; // 状态发布间隔（毫秒）
    uint32_t lastStatusTime = 0; // 上次发布状态的时间
    
    // WiFi配置
    String wifiSSID = DEFAULT_WIFI_SSID;
    String wifiPassword = DEFAULT_WIFI_PASSWORD;
};

// ---------------- 实现部分 ----------------

MqttControl *MqttControl::instance = nullptr;

void MqttControl::begin()
{
    instance = this;
    lastStatusTime = millis();

    // 设置 MQTT Broker 参数及回调函数
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttCallback);

    // 启动一个 FreeRTOS 任务来运行 MQTT 循环
    xTaskCreate(
        [](void *param)
        {
            MqttControl *control = reinterpret_cast<MqttControl *>(param);
            for (;;)
            {
                control->loop();
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "mqttLoopTask",
        4096,
        this,
        1,
        NULL);
}

bool MqttControl::connectToWiFi(const char* ssid, const char* password, int timeout_ms) {
    // 保存WiFi配置
    wifiSSID = ssid;
    wifiPassword = password;
    
    // 连接到 WiFi
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
        // 连接成功后尝试连接MQTT
        connectMQTT();
        return true;
    } else {
        Logger::error(WIFI_TAG, "Failed to connect to WiFi");
        return false;
    }
}

void MqttControl::connectMQTT()
{
    if (WiFi.status() != WL_CONNECTED) {
        Logger::error(MQTT_TAG, "WiFi not connected, cannot connect to MQTT");
        return;
    }
    
    int retries = 0;
    while (!mqttClient.connected() && retries < 3)
    {
        Logger::info(MQTT_TAG, "Connecting to MQTT Broker...");
        if (mqttClient.connect("ESP32Client", MQTT_USERNAME, MQTT_PASSWORD))
        {
            Logger::info(MQTT_TAG, "Connected to MQTT broker");
            // 订阅控制主题
            mqttClient.subscribe(MQTT_TOPIC_CONTROL);
        }
        else
        {
            Logger::error(MQTT_TAG, "Failed to connect to MQTT, rc=%d, retry %d/3", mqttClient.state(), retries+1);
            vTaskDelay(pdMS_TO_TICKS(2000));
            retries++;
        }
    }
}

void MqttControl::loop()
{
    // 检查WiFi连接
    if (WiFi.status() != WL_CONNECTED) {
        static uint32_t lastReconnectAttempt = 0;
        uint32_t now = millis();
        // 每30秒尝试重新连接一次WiFi
        if (now - lastReconnectAttempt > 30000) {
            lastReconnectAttempt = now;
            Logger::info(WIFI_TAG, "WiFi disconnected, attempting to reconnect...");
            connectToWiFi(wifiSSID.c_str(), wifiPassword.c_str());
        }
        return;
    }
    
    // 检查MQTT连接
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    
    // 处理MQTT消息
    mqttClient.loop();
    
    // 定期发布状态 - 使用时间间隔而非计数
    if (statusInterval > 0) {
        uint32_t now = millis();
        if (now - lastStatusTime >= statusInterval) {
            publishStatus();
            lastStatusTime = now;
        }
    }
}

void MqttControl::publishStatus()
{
    // 如果未连接到MQTT，则不发布状态
    if (!mqttClient.connected()) {
        return;
    }
    
    // 获取当前小车状态
    CarState state = carController->getCarState();

    JsonDocument doc;
    doc["vx"] = state.vx;
    doc["vy"] = state.vy;
    doc["omega"] = state.omega;
    JsonArray speeds = doc["wheelSpeeds"].to<JsonArray>();
    for (auto speed : state.wheelSpeeds)
    {
        speeds.add(speed);
    }

    char buffer[JSON_BUFFER_SIZE];
    size_t n = serializeJson(doc, buffer);
    mqttClient.publish(MQTT_TOPIC_STATUS, buffer, n);
    Logger::debug(MQTT_TAG, "Published status: %s", buffer);
}

void MqttControl::mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Logger::info(MQTT_TAG, "Message arrived [%s]", topic);

    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Logger::debug(MQTT_TAG, "Payload: %s", message.c_str());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Logger::error(MQTT_TAG, "JSON Parse failed: %s", error.c_str());
        return;
    }

    const char *command = doc["command"];
    if (!command)
    {
        Logger::warn(MQTT_TAG, "No command found in JSON");
        return;
    }

    // 根据命令调用 CarController 接口
    if (strcmp(command, "speed") == 0)
    {
        float vx = doc["vx"] | 0.0;
        float vy = doc["vy"] | 0.0;
        float omega = doc["omega"] | 0.0;
        float acceleration = doc["acceleration"] | 10.0;
        Logger::info(MQTT_TAG, "Executing speed command: vx=%.2f, vy=%.2f, omega=%.2f", vx, vy, omega);
        if (instance && instance->carController)
        {
            // 调用带自定义加速度的速度模式接口
            instance->carController->setSpeed(vx, vy, omega, acceleration);
        }
    }
    else if (strcmp(command, "move") == 0)
    {
        float dx = doc["dx"] | 0.0;
        float dy = doc["dy"] | 0.0;
        float dtheta = doc["dtheta"] | 0.0;
        float speed = doc["speed"] | 1.0;
        float acceleration = doc["acceleration"] | 10.0;
        uint16_t subdivision = doc["subdivision"] | 256;
        Logger::info(MQTT_TAG, "Executing move command: dx=%.2f, dy=%.2f, dtheta=%.2f, speed=%.2f", dx, dy, dtheta, speed);
        if (instance && instance->carController)
        {
            instance->carController->moveDistance(dx, dy, dtheta, acceleration, speed, subdivision);
        }
    }
    else if (strcmp(command, "stop") == 0)
    {
        Logger::info(MQTT_TAG, "Executing stop command");
        if (instance && instance->carController)
        {
            instance->carController->stop();
        }
    }
    else if (strcmp(command, "get_status") == 0)
    {
        Logger::info(MQTT_TAG, "Status request received");
        if (instance) {
            instance->publishStatus();
        }
    }
    else if (strcmp(command, "set_interval") == 0)
    {
        // 处理设置状态发布间隔命令
        uint32_t interval = doc["interval"] | 1000;
        Logger::info(MQTT_TAG, "Setting status interval to %d ms", interval);
        if (instance) {
            instance->setStatusInterval(interval);
        }
    }
    else if (strcmp(command, "set_wifi") == 0)
    {
        // 处理WiFi设置命令
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];
        
        if (ssid && password) {
            Logger::info(MQTT_TAG, "Setting WiFi: SSID=%s", ssid);
            if (instance) {
                // 断开当前MQTT连接
                instance->mqttClient.disconnect();
                // 尝试连接到新的WiFi
                instance->connectToWiFi(ssid, password);
            }
        } else {
            Logger::warn(MQTT_TAG, "Invalid WiFi settings");
        }
    }
    else
    {
        Logger::warn(MQTT_TAG, "Unknown command: %s", command);
    }
}