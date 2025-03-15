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
    uint32_t timestamp; // 命令时间戳，用于判断新旧
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
            3, // 提高优先级，确保命令能及时处理
            &commandTaskHandle
        );
        
        // 启动状态更新任务 - 独立于命令处理
        xTaskCreate(
            [](void* param) {
                ControlManager* manager = static_cast<ControlManager*>(param);
                manager->updateStateTask();
            },
            "stateUpdateTask",
            4096,
            this,
            1, // 较低优先级
            &stateTaskHandle
        );
    }

    // 设置速度命令
    void setSpeed(float vx, float vy, float omega, float acceleration = 10.0f, uint16_t subdivision = 256) {
        ControlCommand cmd;
        cmd.type = CommandType::SPEED;
        cmd.param1 = vx;
        cmd.param2 = vy;
        cmd.param3 = omega;
        cmd.param4 = acceleration;
        cmd.param6 = subdivision;
        cmd.timestamp = millis();
        
        // 添加到命令队列，替换同类型的旧命令
        std::lock_guard<std::mutex> lock(commandMutex);
        replaceCommand(cmd);
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
        cmd.timestamp = millis();
        
        // 添加到命令队列，替换同类型的旧命令
        std::lock_guard<std::mutex> lock(commandMutex);
        replaceCommand(cmd);
    }

    // 停止命令 - 停止命令始终优先处理
    void stop() {
        ControlCommand cmd;
        cmd.type = CommandType::STOP;
        cmd.timestamp = millis();
        
        // 停止命令优先处理，清空队列
        std::lock_guard<std::mutex> lock(commandMutex);
        while (!commandQueue.empty()) {
            commandQueue.pop();
        }
        commandQueue.push(cmd);
        
        // 直接调用停止，不等待任务处理
        if (carController) {
            carController->stop();
        }
    }

    // 获取当前小车状态 - 使用缓存
    CarState getCarState() {
        std::lock_guard<std::mutex> lock(stateMutex);
        return cachedState;
    }

    // 强制更新状态
    void forceUpdateState() {
        ControlCommand cmd;
        cmd.type = CommandType::GET_STATUS;
        cmd.timestamp = millis();
        
        std::lock_guard<std::mutex> lock(commandMutex);
        // 不替换，直接添加到队列
        commandQueue.push(cmd);
    }

    // 设置状态更新间隔
    void setStateUpdateInterval(uint32_t interval_ms) {
        stateUpdateInterval = interval_ms;
    }

private:
    // 私有构造函数 - 单例模式
    ControlManager() : lastStateUpdateTime(0), stateUpdateInterval(50) {}
    
    // 禁止拷贝和赋值
    ControlManager(const ControlManager&) = delete;
    ControlManager& operator=(const ControlManager&) = delete;
    
    // 替换队列中同类型的命令
    void replaceCommand(const ControlCommand& newCmd) {
        // 创建临时队列
        std::queue<ControlCommand> tempQueue;
        
        // 遍历原队列，保留不同类型的命令
        while (!commandQueue.empty()) {
            ControlCommand cmd = commandQueue.front();
            commandQueue.pop();
            
            // 如果不是同类型命令，保留
            if (cmd.type != newCmd.type) {
                tempQueue.push(cmd);
            }
        }
        
        // 添加新命令
        tempQueue.push(newCmd);
        
        // 恢复队列
        commandQueue = std::move(tempQueue);
    }

    // 命令处理任务
    void processCommandsTask() {
        for (;;) {
            bool hasCommand = false;
            ControlCommand cmd;
            
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
                        Logger::debug("ControlManager", "Executing speed command: vx=%.2f, vy=%.2f, omega=%.2f, subdivision=%d", 
                                     cmd.param1, cmd.param2, cmd.param3, cmd.param6);
                        carController->setSpeed(cmd.param1, cmd.param2, cmd.param3, cmd.param4,cmd.param6);
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
                        updateState(true);
                        break;
                }
            }
            
            // 更短的延时，提高响应速度
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    
    // 状态更新任务 - 独立于命令处理
    void updateStateTask() {
        for (;;) {
            updateState(false);
            vTaskDelay(pdMS_TO_TICKS(stateUpdateInterval));
        }
    }
    
    // 更新状态
    void updateState(bool force) {
        uint32_t now = millis();
        
        // 如果强制更新或者状态过期
        if (force || (now - lastStateUpdateTime > stateUpdateInterval)) {
            if (carController) {
                CarState newState = carController->getCarState();
                
                // 更新缓存
                {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    cachedState = newState;
                    lastStateUpdateTime = now;
                }
            }
        }
    }

    CarController* carController = nullptr;
    TaskHandle_t commandTaskHandle = nullptr;
    TaskHandle_t stateTaskHandle = nullptr;
    
    // 命令队列和互斥锁
    std::queue<ControlCommand> commandQueue;
    std::mutex commandMutex;
    
    // 状态缓存和互斥锁
    CarState cachedState;
    std::mutex stateMutex;
    uint32_t lastStateUpdateTime;
    uint32_t stateUpdateInterval; // 状态更新间隔（毫秒）
}; 