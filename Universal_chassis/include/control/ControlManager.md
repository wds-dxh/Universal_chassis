# 控制管理器 (ControlManager)

控制管理器是ESP32小车控制系统的核心组件，负责协调不同控制接口（MQTT、USB等）对小车的控制请求，并提供状态缓存和里程计功能。它采用单例模式设计，确保系统中只有一个控制管理器实例。

## 功能特点

- **命令队列管理**：使用FreeRTOS队列处理控制命令，避免并发冲突
- **状态缓存**：缓存小车状态，减少对底层硬件的频繁访问
- **里程计**：通过速度积分计算小车位置和方向
- **并发控制**：使用FreeRTOS互斥锁保护共享资源
- **实时处理**：独立任务处理命令和更新状态，确保实时响应

## 接口说明

### 初始化

```cpp
void init(CarController* controller);
```

初始化控制管理器，设置底层控制器，创建任务和队列。

### 控制命令

```cpp
// 设置速度命令
void setSpeed(float vx, float vy, float omega, float acceleration = 10.0f, uint16_t subdivision = 256);

// 移动距离命令
void moveDistance(float dx, float dy, float dtheta, float acceleration = 10.0f, 
                 float speed = 1.0f, uint16_t subdivision = 256);

// 停止命令
void stop();
```

这些方法将控制命令添加到命令队列，由命令处理任务执行。

### 状态查询

```cpp
// 获取当前小车状态
CarState getCarState();

// 获取当前里程计数据
Odometer getOdometer();
```

这些方法返回缓存的状态信息，无需直接访问底层硬件。

### 里程计控制

```cpp
// 重置里程计命令
void resetOdometer();
```

将里程计数据重置为零。

### 配置

```cpp
// 设置状态更新间隔
void setStateUpdateInterval(uint32_t interval_ms);
```

设置状态缓存的更新频率。

## 数据结构

### 命令类型 (CommandType)

```cpp
enum class CommandType {
    SPEED,        // 设置速度
    MOVE,         // 移动距离
    STOP,         // 停止
    GET_STATUS,   // 获取状态   
    RESET_ODOMETER // 重置里程计
};
```

### 里程计 (Odometer)

```cpp
typedef struct {
    float x;     // 里程计x坐标
    float y;     // 里程计y坐标
    float theta; // 里程计角度 (-π到π)
    float vx;    // 里程计x方向速度
    float vy;    // 里程计y方向速度
    float omega; // 里程计角速度
} Odometer;
```

### 控制命令 (ControlCommand)

```cpp
struct ControlCommand {
    CommandType type;
    float param1;  // vx 或 dx
    float param2;  // vy 或 dy
    float param3;  // omega 或 dtheta
    float param4;  // acceleration
    float param5;  // speed (仅用于MOVE命令)
    uint16_t param6; // subdivision (仅用于MOVE命令)
    uint32_t timestamp; // 命令时间戳
};
```

## 实现细节

### 命令处理

控制管理器使用命令队列和命令处理任务来处理控制命令。当收到新命令时，会先检查队列中是否有同类型的旧命令，如果有则替换，确保只执行最新的命令。这种设计避免了命令积压和执行延迟。

### 状态缓存

状态缓存机制减少了对底层硬件的频繁访问，提高了系统性能。状态更新任务定期从底层控制器获取最新状态并更新缓存。

### 里程计实现

里程计通过速度积分计算小车的位置和方向：

1. 角速度积分得到角度变化：`dtheta = omega * dt`
2. 线速度在全局坐标系下分解：
   - `dx = vx * cos(theta) * dt`
   - `dy = vx * sin(theta) * dt`
3. 角度保持在-π到π范围内

里程计更新任务以100Hz的频率运行，确保积分精度。

### 并发控制

控制管理器使用FreeRTOS互斥锁保护共享资源：
- `stateMutex`保护状态缓存
- `odometerMutex`保护里程计数据

这确保了在多任务环境下数据的一致性和安全性。

## 使用示例

```cpp
// 获取控制管理器实例
ControlManager& manager = ControlManager::getInstance();

// 初始化
manager.init(&carController);

// 设置速度
manager.setSpeed(0.5, 0.0, 0.1);

// 获取状态
CarState state = manager.getCarState();

// 获取里程计数据
Odometer odom = manager.getOdometer();

// 重置里程计
manager.resetOdometer();
```

## 注意事项

- 控制管理器是单例模式，不能创建多个实例
- 必须先调用`init`方法初始化，再使用其他功能
- 里程计数据是通过速度积分计算的，长时间运行可能会有累积误差
- 状态缓存的更新频率影响状态数据的实时性，可根据需要调整 