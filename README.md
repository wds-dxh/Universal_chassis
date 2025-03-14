# ESP32 通用底盘控制系统

这是一个完整的基于ESP32的小车控制系统，包含硬件控制和Web前端两个主要部分。系统支持通过MQTT无线控制和USB串口两种方式进行远程操作，实现了完整的运动控制功能和实时状态反馈。

## 系统架构图

```
+----------------+     +-------------------+     +----------------+
|                |     |                   |     |                |
|  Web控制界面   +---->+  MQTT Broker      +---->+                |
|  (浏览器)      |     |  (消息中转服务器) |     |                |
|                |     |                   |     |                |
+----------------+     +-------------------+     |                |
                                                 |   ESP32控制器  |
+----------------+     +-------------------+     |   (小车主控)   |
|                |     |                   |     |                |
|  控制终端      +---->+  USB串口          +---->+                |
|  (PC/树莓派)   |     |  (直连通信)       |     |                |
|                |     |                   |     |                |
+----------------+     +-------------------+     +----------------+
                                                        |
                                                        v
                                               +------------------+
                                               |                  |
                                               |  步进电机驱动    |
                                               |  (运动执行)      |
                                               |                  |
                                               +------------------+
```

## 系统架构

本项目由两个主要模块组成：

1. **硬件控制模块** (`Universal_chassis/`): 
   - 负责ESP32与步进电机的通信和控制
   - 实现运动学模型和控制算法
   - 提供MQTT和USB串口控制接口
   - [详细说明](Universal_chassis/README.md)

2. **Web控制界面** (`WebControl/`):
   - 提供基于Web的用户界面
   - 支持双摇杆控制和实时视频流显示
   - 多设备管理和响应式设计
   - [详细说明](WebControl/readme.md)

## 功能特点

### 硬件控制部分
- **双通道控制**：支持MQTT无线控制和USB串口控制
- **多种运动模式**：速度模式和位置模式
- **实时状态反馈**：提供小车速度和电机状态的实时反馈
- **动态配置**：支持运行时修改WiFi连接和状态发布间隔

### Web控制界面部分
- **双摇杆控制界面**：左摇杆控制线速度，右摇杆控制角速度
- **实时视频流显示**和**多设备支持**
- **移动端友好**的响应式设计
- **低延迟**的控制响应

## 硬件要求

- ESP32开发板
- 四轮小车底盘
- 四个步进电机及驱动器
- USB连接线（用于串口控制和调试）
- 电源系统
- 可选：摄像头（用于视频流）

## 软件依赖

- PlatformIO开发环境
- Arduino框架
- PubSubClient库（MQTT客户端）
- ArduinoJson库
- Node.js (Web界面部分)
- Express (Web服务器)

## 快速开始

### 硬件控制部分

请参考 [硬件控制模块说明](Universal_chassis/README.md) 获取详细的编译、上传和使用说明。

### Web控制界面部分

请参考 [Web控制界面说明](WebControl/readme.md) 获取详细的安装、配置和使用说明。

## 项目结构

```
Universal_chassis/
├── Universal_chassis/        # 硬件控制部分
│   ├── include/              # 头文件
│   │   ├── CarController/    # 小车控制核心类
│   │   ├── KinematicsModel/  # 运动学模型
│   │   ├── StepperMotor/     # 步进电机控制
│   │   ├── protocol/         # 控制协议定义
│   │   ├── task/             # 任务模块（MQTT和USB控制）
│   │   └── utils/            # 工具类（日志等）
│   ├── src/                  # 源文件
│   ├── example/              # 示例代码
│   ├── control_serial.py     # Python测试脚本
│   └── platformio.ini        # PlatformIO配置
└── WebControl/               # Web控制界面部分
    ├── public/               # 静态资源
    ├── src/                  # 前端源代码
    ├── server.js             # 服务器脚本
    └── config.js             # 配置文件
```

## 控制协议

系统使用JSON格式的控制协议，支持多种命令类型。详细协议说明请参考：
- [通用控制协议](Universal_chassis/include/protocol/ControlProtocol.md)
- [MQTT控制协议](Universal_chassis/include/task/Mqtt_Control.md)
- [USB控制协议](Universal_chassis/include/task/Usb_Control.md)

### 速度控制示例

```json
{
  "command": "speed",
  "vx": 0.5,
  "vy": 0.0,
  "omega": 0.1,
  "acceleration": 10.0
}
```

### 位置控制示例

```json
{
  "command": "move",
  "dx": 1.0,
  "dy": 0.0,
  "dtheta": 0.0,
  "speed": 0.5,
  "acceleration": 10.0,
  "subdivision": 256
}
```

## 高级配置

### 修改WiFi设置

可以通过控制命令动态修改WiFi连接：

```json
{
  "command": "set_wifi",
  "ssid": "MyWiFi",
  "password": "MyPassword"
}
```

### 调整状态发布频率

```json
{
  "command": "set_interval",
  "interval": 500
}
```

### 日志级别控制

在`main.cpp`中修改日志级别：

```cpp
#ifdef DEBUG_MODE
    Logger::init(LOG_LEVEL_DEBUG);
#else
    Logger::init(LOG_LEVEL_NONE);  // 生产环境禁用日志
#endif
```

## Web界面使用说明

### 控制界面

- **左侧摇杆**：上下移动控制前进/后退
- **右侧摇杆**：左右移动控制转向
- **滑块控制**：
  - 最大速度设置
  - 最大角速度设置
  - 加速度设置

### 视频显示

- 实时显示设备摄像头画面
- 支持自动重连

### 设备认证

- 设备ID：3位数字（例如：001, 002, ...）
- 默认密码：88888888

## 故障排除

### 常见问题

1. **无法连接 MQTT**
   - 检查 MQTT Broker 地址和端口
   - 验证用户名和密码
   - 确认网络连接和防火墙设置

2. **WebSocket 连接失败**
   - 检查服务器是否正常运行
   - 确认浏览器支持 WebSocket
   - 检查 Nginx 配置是否正确转发 WebSocket 连接

3. **控制命令无响应**
   - 检查 MQTT 主题是否正确
   - 验证小车是否在线并订阅了控制主题
   - 查看服务器日志是否有错误信息

## 贡献

欢迎提交问题报告和改进建议！请遵循以下步骤：

1. Fork本仓库
2. 创建您的特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交您的更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 打开Pull Request

## 许可证

本项目采用Apache License 2.0许可证 - 详情请参见[LICENSE](LICENSE)文件

## 联系方式

项目维护者 - [wds-dxh](https://github.com/wds-dxh)

项目链接: [https://github.com/wds-dxh/Universal_chassis](https://github.com/wds-dxh/Universal_chassis)
