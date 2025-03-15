#pragma once

#include "CarController/CarController.h"
#include "utils/Logger.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <cmath>

// 定义命令类型
enum class CommandType {
    SPEED,        // 设置速度
    MOVE,         // 移动距离
    STOP,         // 停止
    GET_STATUS,   // 获取状态   
    RESET_ODOMETER // 重置里程计
};

// 定义里程计
typedef struct {
    float x;     // 里程计x坐标
    float y;     // 里程计y坐标
    float theta; // 里程计角度 (-π到π)
    float vx;    // 里程计x方向速度，因为底盘无y方向速度，vx就是线速度
    float vy;    // 里程计y方向速度,普通底盘无y方向速度，所以vy=0，为了结构体统一所以补全
    float omega; // 里程计角速度
} Odometer;

// 命令结构体
struct ControlCommand {
    CommandType type;
    float param1;  // vx 或 dx
    float param2;  // vy 或 dy
    float param3;  // omega 或 dtheta
    float param4;  // acceleration
    float param5;  // speed (仅用于MOVE命令)
    uint16_t param6; // subdivision (仅用于MOVE命令)
    uint32_t timestamp; // 命令时间戳，用于判断新旧
};

// 控制管理器类 - 单例模式
class ControlManager {
public:
    // 获取单例实例
    static ControlManager& getInstance();

    // 初始化控制管理器
    void init(CarController* controller);

    // 设置速度命令
    void setSpeed(float vx, float vy, float omega, float acceleration = 10.0f, uint16_t subdivision = 256);

    // 移动距离命令
    void moveDistance(float dx, float dy, float dtheta, float acceleration = 10.0f, 
                     float speed = 1.0f, uint16_t subdivision = 256);

    // 停止命令
    void stop();
    
    // 重置里程计命令
    void resetOdometer();

    // 获取当前小车状态
    CarState getCarState();
    
    // 获取当前里程计数据
    Odometer getOdometer();
    
    // 设置状态更新间隔
    void setStateUpdateInterval(uint32_t interval_ms);

private:
    // 私有构造函数，防止外部创建实例
    ControlManager() = default;
    
    // 禁止拷贝和赋值
    ControlManager(const ControlManager&) = delete;
    ControlManager& operator=(const ControlManager&) = delete;
    
    // 静态任务包装函数
    static void controlTaskWrapper(void* param);    // 统一控制任务
    
    // 替换命令队列中的同类型命令
    void replaceCommand(const ControlCommand& newCmd);
    
    // 统一控制任务 - 处理命令、更新状态和里程计
    void controlTask();
    
    // 执行控制命令
    void executeCommand(const ControlCommand& cmd);
    
    // 更新状态
    void updateState();
    
    // 更新里程计
    void updateOdometer();

    CarController* carController = nullptr;
    TaskHandle_t controlTaskHandle = nullptr;
    
    // FreeRTOS 队列和互斥锁
    QueueHandle_t commandQueue = nullptr;   // 命令队列
    SemaphoreHandle_t stateMutex = nullptr; // 状态互斥锁
    SemaphoreHandle_t odometerMutex = nullptr; // 里程计互斥锁
    
    // 状态缓存
    CarState cachedState;
    uint32_t lastStateUpdateTime;
    uint32_t stateUpdateInterval; // 状态更新间隔（毫秒）
    
    // 里程计数据
    Odometer odometer;
    uint32_t lastOdometerUpdateTime;
};

//==============================================================================
// 实现部分
//==============================================================================

// 获取单例实例
inline ControlManager& ControlManager::getInstance() {
    static ControlManager instance;
    return instance;
}

// 初始化控制管理器
inline void ControlManager::init(CarController* controller) {
    carController = controller;
    
    // 创建命令队列
    commandQueue = xQueueCreate(10, sizeof(ControlCommand));
    
    // 创建互斥锁
    stateMutex = xSemaphoreCreateMutex();
    odometerMutex = xSemaphoreCreateMutex();
    
    // 设置默认状态更新间隔
    stateUpdateInterval = 50;
    lastStateUpdateTime = 0;
    
    // 初始化里程计
    resetOdometer();
    lastOdometerUpdateTime = millis();
    
    // 启动统一控制任务
    xTaskCreate(
        controlTaskWrapper,
        "controlTask",
        4096,
        this,
        5, // 高优先级
        &controlTaskHandle
    );
}

// 静态任务包装函数
inline void ControlManager::controlTaskWrapper(void* param) {
    ControlManager* manager = static_cast<ControlManager*>(param);
    manager->controlTask();
}

// 设置速度命令
inline void ControlManager::setSpeed(float vx, float vy, float omega, float acceleration, uint16_t subdivision) {
    ControlCommand cmd;
    cmd.type = CommandType::SPEED;
    cmd.param1 = vx;
    cmd.param2 = vy;
    cmd.param3 = omega;
    cmd.param4 = acceleration;
    cmd.param5 = 0.0f;
    cmd.param6 = subdivision;
    cmd.timestamp = millis();
    
    // 替换队列中的同类型命令
    replaceCommand(cmd);
    
    // 如果是停止命令，立即执行
    if (vx == 0.0f && vy == 0.0f && omega == 0.0f) {
        stop();
    }
}

// 移动距离命令
inline void ControlManager::moveDistance(float dx, float dy, float dtheta, float acceleration, float speed, uint16_t subdivision) {
    ControlCommand cmd;
    cmd.type = CommandType::MOVE;
    cmd.param1 = dx;
    cmd.param2 = dy;
    cmd.param3 = dtheta;
    cmd.param4 = acceleration;
    cmd.param5 = speed;
    cmd.param6 = subdivision;
    cmd.timestamp = millis();
    
    // 替换队列中的同类型命令
    replaceCommand(cmd);
}

// 停止命令
inline void ControlManager::stop() {
    ControlCommand cmd;
    cmd.type = CommandType::STOP;
    cmd.timestamp = millis();
    
    // 添加到队列，高优先级
    if (commandQueue) {
        // 清空队列，确保停止命令立即执行
        xQueueReset(commandQueue);
        xQueueSendToFront(commandQueue, &cmd, 0);
    }
    
    // 如果控制器可用，直接调用停止
    if (carController) {
        carController->stop();
    }
}

// 重置里程计命令
inline void ControlManager::resetOdometer() {
    ControlCommand cmd;
    cmd.type = CommandType::RESET_ODOMETER;
    cmd.timestamp = millis();
    
    // 加入队列
    replaceCommand(cmd);
    
    // 也可以直接重置
    if (xSemaphoreTake(odometerMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        odometer.x = 0.0f;
        odometer.y = 0.0f;
        odometer.theta = 0.0f;
        odometer.vx = 0.0f;
        odometer.vy = 0.0f;
        odometer.omega = 0.0f;
        xSemaphoreGive(odometerMutex);
        Logger::debug("ControlManager", "Odometer reset");
    }
}

// 获取当前小车状态
inline CarState ControlManager::getCarState() {
    CarState state;
    
    // 从缓存获取状态
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = cachedState;
        xSemaphoreGive(stateMutex);
    }
    
    return state;
}

// 获取当前里程计数据
inline Odometer ControlManager::getOdometer() {
    Odometer odom;
    
    // 获取互斥锁
    if (xSemaphoreTake(odometerMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        odom = odometer;    //直接从缓存获取。
        xSemaphoreGive(odometerMutex);
    }
    
    return odom;
}

// 设置状态更新间隔
inline void ControlManager::setStateUpdateInterval(uint32_t interval_ms) {
    stateUpdateInterval = interval_ms;
}

// 替换命令队列中的同类型命令
inline void ControlManager::replaceCommand(const ControlCommand& newCmd) {
    if (!commandQueue) return;
    
    // 创建临时队列
    QueueHandle_t tempQueue = xQueueCreate(10, sizeof(ControlCommand));
    if (!tempQueue) {
        // 队列创建失败，直接添加新命令
        xQueueSend(commandQueue, &newCmd, 0);
        return;
    }
    
    bool replaced = false;
    ControlCommand cmd;
    
    // 遍历原队列
    while (xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
        // 如果找到同类型的命令，替换它
        if (cmd.type == newCmd.type && !replaced) {
            xQueueSend(tempQueue, &newCmd, 0);
            replaced = true;
        } else {
            // 否则保留原命令
            xQueueSend(tempQueue, &cmd, 0);
        }
    }
    
    // 如果没有替换任何命令，添加新命令
    if (!replaced) {
        xQueueSend(tempQueue, &newCmd, 0);
    }
    
    // 将临时队列中的命令复制回原队列
    while (xQueueReceive(tempQueue, &cmd, 0) == pdTRUE) {
        xQueueSend(commandQueue, &cmd, 0);
    }
    
    // 删除临时队列
    vQueueDelete(tempQueue);
}

// 统一控制任务 - 处理命令、更新状态和里程计
inline void ControlManager::controlTask() {
    TickType_t lastOdometerTime = xTaskGetTickCount();
    TickType_t lastStateTime = xTaskGetTickCount();
    TickType_t currentTime;
    
    for (;;) {
        currentTime = xTaskGetTickCount();
        
        // 1. 优先处理命令队列中的命令
        ControlCommand cmd;
        if (xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
            // 有命令，执行命令
            executeCommand(cmd);
        } else {
            // 2. 没有命令，检查是否需要更新状态
            if ((currentTime - lastStateTime) >= pdMS_TO_TICKS(stateUpdateInterval)) {
                updateState();
                lastStateTime = currentTime;
            }
            
            // 3. 检查是否需要更新里程计 (100Hz)
            if ((currentTime - lastOdometerTime) >= pdMS_TO_TICKS(10)) {
                updateOdometer();
                lastOdometerTime = currentTime;
            }
            
            // 短暂休眠，避免占用过多CPU
            vTaskDelay(1);
        }
    }
}

// 执行控制命令
inline void ControlManager::executeCommand(const ControlCommand& cmd) {
    if (!carController) return;
    
    switch (cmd.type) {
        case CommandType::SPEED:
            Logger::debug("ControlManager", "Executing speed command: vx=%.2f, vy=%.2f, omega=%.2f", 
                         cmd.param1, cmd.param2, cmd.param3);
            carController->setSpeed(cmd.param1, cmd.param2, cmd.param3, cmd.param4, cmd.param6);
            break;
        
        case CommandType::MOVE:
            Logger::debug("ControlManager", "Executing move command: dx=%.2f, dy=%.2f, dtheta=%.2f", 
                         cmd.param1, cmd.param2, cmd.param3);
            carController->moveDistance(cmd.param1, cmd.param2, cmd.param3, 
                                      cmd.param4, cmd.param5, cmd.param6);
            break;
        
        case CommandType::STOP:
            Logger::debug("ControlManager", "Executing stop command");
            carController->stop();
            break;
        
        case CommandType::GET_STATUS:
            // 强制更新状态
            updateState();
            break;
            
        case CommandType::RESET_ODOMETER:
            // 重置里程计
            if (xSemaphoreTake(odometerMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                odometer.x = 0.0f;
                odometer.y = 0.0f;
                odometer.theta = 0.0f;
                odometer.vx = 0.0f;
                odometer.vy = 0.0f;
                odometer.omega = 0.0f;
                xSemaphoreGive(odometerMutex);
                Logger::debug("ControlManager", "Odometer reset");
            }
            break;
    }
}

// 更新状态
inline void ControlManager::updateState() {
    if (!carController) return;
    
    CarState newState = carController->getCarState();
    
    // 更新缓存
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        cachedState = newState;
        lastStateUpdateTime = millis();
        xSemaphoreGive(stateMutex);
    }
}

// 更新里程计
inline void ControlManager::updateOdometer() {
    // 获取当前状态
    CarState state = getCarState();
    
    // 获取当前时间
    uint32_t currentTime = millis();
    float dt = (currentTime - lastOdometerUpdateTime) / 1000.0f; // 转换为秒
    
    // 防止时间差过大或为负（可能由于溢出）
    if (dt > 0.5f || dt <= 0.0f) {
        dt = 0.01f; // 默认10ms
    }
    
    // 获取互斥锁
    if (xSemaphoreTake(odometerMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // 更新速度
        odometer.vx = state.vx;
        odometer.vy = state.vy;
        odometer.omega = state.omega;
        
        // 更新位置和角度（积分）
        float dtheta = odometer.omega * dt;
        odometer.theta += dtheta;
        
        // 保持角度在-π到π范围内
        while (odometer.theta > M_PI) odometer.theta -= 2.0f * M_PI;
        while (odometer.theta < -M_PI) odometer.theta += 2.0f * M_PI;
        
        // 计算位移增量（考虑当前方向）
        float cos_theta = cosf(odometer.theta - dtheta/2.0f); // 使用中点角度计算
        float sin_theta = sinf(odometer.theta - dtheta/2.0f);
        
        // 计算全局坐标系中的位移
        float dx = odometer.vx * cos_theta * dt;
        float dy = odometer.vx * sin_theta * dt;
        
        // 更新位置
        odometer.x += dx;
        odometer.y += dy;
        
        xSemaphoreGive(odometerMutex);  // 释放互斥锁
    }
    
    // 更新时间戳
    lastOdometerUpdateTime = currentTime;
} 

