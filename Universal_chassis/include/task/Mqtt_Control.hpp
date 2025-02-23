#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "CarController/CarController.h"

// WiFi 配置
#define WIFI_SSID "wds"
#define WIFI_PASSWORD "wds666666"

// MQTT 配置
#define MQTT_BROKER_IP "47.108.223.146"
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME "emqx_u"
#define MQTT_PASSWORD "public"

// MQTT 主题
#define MQTT_TOPIC_CONTROL "CarControl"
#define MQTT_TOPIC_STATUS  "CarStatus"

// JSON 缓冲区大小
#define JSON_BUFFER_SIZE 256

class MqttControl {
public:
    explicit MqttControl(CarController* carCtrl) : carController(carCtrl) {}

    // 初始化 WiFi、MQTT 并启动 FreeRTOS 任务
    void begin(int count = 10); // 状态发布频率

    // MQTT client 循环函数（如果需要，可在 loop 中调用）
    void loop();

    // 发布当前小车状态到 MQTT
    void publishStatus();

private:
    // MQTT 接收到消息的回调函数（static 供 PubSubClient 使用）
    static void mqttCallback(char* topic, byte* payload, unsigned int length);

    // 尝试连接到 MQTT Broker
    void connectMQTT();

    // 静态实例指针，供回调函数使用
    static MqttControl* instance;

    CarController* carController;
    WiFiClient       wifiClient;
    PubSubClient     mqttClient { wifiClient };

    int count = 0;      // 计数器，控制状态发布频率
    int publishCount = 0; // 状态发布计数器
};

// ---------------- 实现部分 ----------------

MqttControl* MqttControl::instance = nullptr;

void MqttControl::begin(int count) {
    this->count = count;
    instance = this;

    // 连接到 WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("Connected to WiFi. IP: ");
    Serial.println(WiFi.localIP());

    // 设置 MQTT Broker 参数及回调函数
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttCallback);

    connectMQTT();

    // 启动一个 FreeRTOS 任务来运行 MQTT 循环
    xTaskCreate(
        [](void* param) {
            MqttControl* control = reinterpret_cast<MqttControl*>(param);
            for (;;) {
                control->loop();
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "mqttLoopTask",
        4096,
        this,
        1,
        NULL
    );
}

void MqttControl::connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT Broker...");
        if (mqttClient.connect("ESP32Client", MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("connected");
            // 订阅控制主题
            mqttClient.subscribe(MQTT_TOPIC_CONTROL);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void MqttControl::loop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
    // 可根据需要定期发布状态
    publishCount++;
    if (publishCount % count == 0) {
        publishStatus();
        publishCount = 0;
    }
}

void MqttControl::publishStatus() {
    // 获取当前小车状态
    CarState state = carController->getCarState();
    
    JsonDocument doc;
    doc["vx"] = state.vx;
    doc["vy"] = state.vy;
    doc["omega"] = state.omega;
    JsonArray speeds = doc["wheelSpeeds"].to<JsonArray>();
    for (auto speed : state.wheelSpeeds) {
        speeds.add(speed);
    }
    
    char buffer[JSON_BUFFER_SIZE];
    size_t n = serializeJson(doc, buffer);
    mqttClient.publish(MQTT_TOPIC_STATUS, buffer, n);
}

void MqttControl::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("JSON Parse failed: ");
        Serial.println(error.c_str());
        return;
    }
    
    const char* command = doc["command"];
    if (!command) {
        Serial.println("No command found in JSON");
        return;
    }
    
    // 根据命令调用 CarController 接口
    if (strcmp(command, "speed") == 0) {
        float vx = doc["vx"] | 0.0;
        float vy = doc["vy"] | 0.0;
        float omega = doc["omega"] | 0.0;
        uint32_t duration = doc["duration"] | 1000;
        float acceleration = doc["acceleration"] | 10.0;
        Serial.println("Executing speed command");
        if (instance && instance->carController) {
            // 调用带自定义加速度的速度模式接口
            instance->carController->setSpeed(vx, vy, omega, duration, acceleration);
        }
    } else if (strcmp(command, "move") == 0) {
        float dx = doc["dx"] | 0.0;
        float dy = doc["dy"] | 0.0;
        float dtheta = doc["dtheta"] | 0.0;
        float acceleration = doc["acceleration"] | 10.0;
        uint16_t subdivision = doc["subdivision"] | 256;
        Serial.println("Executing move command");
        if (instance && instance->carController) {
            instance->carController->moveDistance(dx, dy, dtheta, acceleration, subdivision);
        }
    } else if (strcmp(command, "stop") == 0) {
        Serial.println("Executing stop command");
        if (instance && instance->carController) {
            instance->carController->stop();
        }
    } else {
        Serial.println("Unknown command");
    }
}