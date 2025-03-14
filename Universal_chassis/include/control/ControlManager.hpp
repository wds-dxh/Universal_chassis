#pragma once

#include "CarController/CarController.h"
#include "utils/Logger.hpp"
#include <mutex>
#include <queue>
#include <functional>

// 定义命令类型
enum class CommandType {
    SPEED,
    MOVE,
    STOP,
    GET_STATUS
};

// 命令结构体
struct ControlCommand {
    CommandType type;
    float param1;  // vx 或 dx
    float param2;  // vy 或 dy
    float param3;  // omega 或 dtheta
    float param4;  // acceleration
    float param5;  // speed (仅用于MOVE命令)
    uint16_t param6; // subdivision (仅用于MOVE命令)
};

// 控制管理器类 - 单例模式
class ControlManager {
public:
    // 获取单例实例
    static ControlManager& getInstance() {
        static ControlManager instance;
        return instance;
    }

    // 初始化控制管理器
    void init(CarController* controller) {
        carController = controller;
        // 启动命令处理任务
        xTaskCreate(
            [](void* param) {
                ControlManager* manager = static_cast<ControlManager*>(param);
                manager->processCommandsTask();
            },
            "cmdProcessTask",
            4096,
            this,
            2, // 优先级高于MQTT和USB任务
            &commandTaskHandle
        );
    }

    // 设置速度命令
    void setSpeed(float vx, float vy, float omega, float acceleration = 10.0f) {
        ControlCommand cmd;
        cmd.type = CommandType::SPEED;
        cmd.param1 = vx;
        cmd.param2 = vy;
        cmd.param3 = omega;
        cmd.param4 = acceleration;
        
        // 添加到命令队列
        std::lock_guard<std::mutex> lock(commandMutex);
        commandQueue.push(cmd);
    }

    // 移动距离命令
    void moveDistance(float dx, float dy, float dtheta, float acceleration = 10.0f, 
                     float speed = 1.0f, uint16_t subdivision = 256) {
        ControlCommand cmd;
        cmd.type = CommandType::MOVE;
        cmd.param1 = dx;
        cmd.param2 = dy;
        cmd.param3 = dtheta;
        cmd.param4 = acceleration;
        cmd.param5 = speed;
        cmd.param6 = subdivision;
        
        // 添加到命令队列
        std::lock_guard<std::mutex> lock(commandMutex);
        commandQueue.push(cmd);
    }

    // 停止命令
    void stop() {
        ControlCommand cmd;
        cmd.type = CommandType::STOP;
        
        // 添加到命令队列 - 停止命令优先处理
        std::lock_guard<std::mutex> lock(commandMutex);
        // 清空队列中的其他命令
        while (!commandQueue.empty()) {
            commandQueue.pop();
        }
        commandQueue.push(cmd);
    }

    // 获取当前状态 - 直接返回缓存的状态
    CarState getCarState() {
        std::lock_guard<std::mutex> lock(stateMutex);
        // 如果状态过期，则更新状态
        uint32_t now = millis();
        if (now - lastStateUpdateTime > stateUpdateInterval) {
            if (carController) {
                cachedState = carController->getCarState();
                lastStateUpdateTime = now;
            }
        }
        return cachedState;
    }

    // 设置状态更新间隔
    void setStateUpdateInterval(uint32_t interval_ms) {
        stateUpdateInterval = interval_ms;
    }

private:
    // 私有构造函数 - 单例模式
    ControlManager() : lastStateUpdateTime(0), stateUpdateInterval(100) {}
    
    // 禁止拷贝和赋值
    ControlManager(const ControlManager&) = delete;
    ControlManager& operator=(const ControlManager&) = delete;

    // 命令处理任务
    void processCommandsTask() {
        for (;;) {
            ControlCommand cmd;
            bool hasCommand = false;
            
            // 获取命令
            {
                std::lock_guard<std::mutex> lock(commandMutex);
                if (!commandQueue.empty()) {
                    cmd = commandQueue.front();
                    commandQueue.pop();
                    hasCommand = true;
                }
            }
            
            // 处理命令
            if (hasCommand && carController) {
                switch (cmd.type) {
                    case CommandType::SPEED:
                        Logger::debug("ControlManager", "Executing speed command: vx=%.2f, vy=%.2f, omega=%.2f", 
                                     cmd.param1, cmd.param2, cmd.param3);
                        carController->setSpeed(cmd.param1, cmd.param2, cmd.param3, cmd.param4);
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
                        {
                            std::lock_guard<std::mutex> lock(stateMutex);
                            cachedState = carController->getCarState();
                            lastStateUpdateTime = millis();
                        }
                        break;
                }
                
                // 更新状态缓存
                {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    cachedState = carController->getCarState();
                    lastStateUpdateTime = millis();
                }
            }
            
            // 短暂延时
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    CarController* carController = nullptr;
    TaskHandle_t commandTaskHandle = nullptr;
    
    // 命令队列和互斥锁
    std::queue<ControlCommand> commandQueue;
    std::mutex commandMutex;
    
    // 状态缓存和互斥锁
    CarState cachedState;
    std::mutex stateMutex;
    uint32_t lastStateUpdateTime;
    uint32_t stateUpdateInterval; // 状态更新间隔（毫秒）
}; 