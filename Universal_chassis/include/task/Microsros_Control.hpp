#pragma once

#include <Arduino.h>
#include "control/ControlManager.hpp"
#include "utils/Logger.hpp"
#include "config.h"

// MicroROS相关头文件
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>

class MicrorosControl {
public:
    // 构造函数
    MicrorosControl();
    
    // 初始化MicroROS
    void begin();
    
    // 循环处理MicroROS事件
    void spin();
    
    // 检查MicroROS连接状态
    bool isConnected() const;
    
private:
    // 初始化MicroROS通信
    bool initMicroROS();
    
    // 创建订阅者
    bool createSubscription();
    
    // 处理cmd_vel消息的回调函数
    static void cmdVelCallback(const void* msgin);
    
    // 控制管理器引用
    ControlManager& controlManager;
    
    // MicroROS相关变量
    rcl_node_t node;
    rcl_subscription_t cmdVelSub;
    geometry_msgs__msg__Twist cmdVelMsg;
    rclc_executor_t executor;
    rclc_support_t support;
    rcl_allocator_t allocator;
    
    // 连接状态
    bool connected;
    
    // 任务句柄
    TaskHandle_t spinTaskHandle;
    
    // 静态任务包装函数
    static void spinTaskWrapper(void* param);
};

//==============================================================================
// 实现部分
//==============================================================================

// 构造函数
inline MicrorosControl::MicrorosControl() 
    : controlManager(ControlManager::getInstance()),
      connected(false),
      spinTaskHandle(nullptr) {
}

// 初始化MicroROS
inline void MicrorosControl::begin() {
    Logger::info(MICROROS_TAG, "Initializing MicroROS control interface");
    
    // 初始化MicroROS
    if (initMicroROS()) {
        // 创建订阅者
        if (createSubscription()) {
            // 创建任务处理MicroROS事件
            xTaskCreate(
                spinTaskWrapper,
                "microROSTask",
                4096,
                this,
                4, // 中高优先级
                &spinTaskHandle
            );
            
            Logger::info(MICROROS_TAG, "MicroROS initialized successfully");
        } else {
            Logger::error(MICROROS_TAG, "Failed to create subscription");
        }
    } else {
        Logger::error(MICROROS_TAG, "Failed to initialize MicroROS");
    }
}

// 初始化MicroROS通信
inline bool MicrorosControl::initMicroROS() {
    // 设置MicroROS传输层 - 使用UDP
    IPAddress agent_ip;
    agent_ip.fromString(MICROROS_AGENT_IP);
    
    Logger::info(MICROROS_TAG, "Connecting to MicroROS agent at %s:%d", 
                MICROROS_AGENT_IP, MICROROS_AGENT_PORT);
    
    // 初始化MicroROS传输层
    set_microros_wifi_transports(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD, agent_ip, MICROROS_AGENT_PORT);
    
    // 初始化分配器
    allocator = rcl_get_default_allocator();
    
    // 初始化支持结构
    rclc_support_init(&support, 0, NULL, &allocator);
    
    // 创建节点
    rclc_node_init_default(&node, MICROROS_NODE_NAME, "", &support);
    
    // 创建执行器
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    
    connected = true;
    return true;
}

// 创建订阅者
inline bool MicrorosControl::createSubscription() {
    // 创建cmd_vel订阅者
    rcl_subscription_options_t subscription_options = rcl_subscription_get_default_options();
    
    // 初始化消息
    geometry_msgs__msg__Twist__init(&cmdVelMsg);
    
    // 创建订阅
    rcl_ret_t ret = rcl_subscription_init(
        &cmdVelSub,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        MICROROS_TOPIC_CMD_VEL,
        &subscription_options
    );
    
    if (ret != RCL_RET_OK) {
        Logger::error(MICROROS_TAG, "Failed to create cmd_vel subscription: %d", ret);
        return false;
    }
    
    // 添加订阅到执行器
    ret = rclc_executor_add_subscription(
        &executor,
        &cmdVelSub,
        &cmdVelMsg,
        &MicrorosControl::cmdVelCallback,
        ON_NEW_DATA
    );
    
    if (ret != RCL_RET_OK) {
        Logger::error(MICROROS_TAG, "Failed to add subscription to executor: %d", ret);
        return false;
    }
    
    Logger::info(MICROROS_TAG, "Subscribed to %s topic", MICROROS_TOPIC_CMD_VEL);
    return true;
}

// 处理cmd_vel消息的回调函数
inline void MicrorosControl::cmdVelCallback(const void* msgin) {
    const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msgin;
    
    // 获取控制管理器实例
    ControlManager& manager = ControlManager::getInstance();
    
    // 提取线速度和角速度
    float vx = (float)msg->linear.x;
    float vy = 0.0f; // 我们只使用x方向的线速度
    float omega = (float)msg->angular.z; // 只使用z轴的角速度
    
    // 设置小车速度
    manager.setSpeed(vx, vy, omega);
    
    Logger::debug(MICROROS_TAG, "Received cmd_vel: vx=%.2f, omega=%.2f", vx, omega);
}

// 循环处理MicroROS事件
inline void MicrorosControl::spin() {
    if (connected) {
        // 处理MicroROS事件
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    } else {
        // 尝试重新连接
        if (initMicroROS()) {
            createSubscription();
        }
    }
}

// 静态任务包装函数
inline void MicrorosControl::spinTaskWrapper(void* param) {
    MicrorosControl* control = static_cast<MicrorosControl*>(param);
    
    // 循环处理MicroROS事件
    for (;;) {
        control->spin();
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz
    }
}

// 检查MicroROS连接状态
inline bool MicrorosControl::isConnected() const {
    return connected;
}
