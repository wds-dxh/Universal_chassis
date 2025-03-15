#pragma once

// WiFi 默认配置
#define DEFAULT_WIFI_SSID "ivis"
#define DEFAULT_WIFI_PASSWORD "ivis666666"

// MQTT 配置
#define MQTT_BROKER_IP "47.108.223.146"
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME "emqx_u"
#define MQTT_PASSWORD "public"

//设备编号
#define DEVICE_ID "001"

// MQTT 主题
#define MQTT_TOPIC_CONTROL "CarControl_001"
#define MQTT_TOPIC_STATUS "CarStatus_001"

// JSON 缓冲区大小
#define JSON_BUFFER_SIZE 256

// 串口接收缓冲区大小
#define SERIAL_RX_BUFFER_SIZE 512

// USB 传输 JSON 缓冲区大小
#define USB_JSON_BUFFER_SIZE 1024


// 日志标签
#define MQTT_TAG "MQTT"
#define WIFI_TAG "WIFI"
#define USB_TAG "USB"