<!--
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-23 16:55:18
 * @Description: 
 * Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
-->
# 通用底盘控制系统

## 项目简介
通用底盘控制系统是一个基于 Web 的远程控制平台，支持多设备管理，提供实时视频流和双摇杆控制界面，适用于各类移动机器人底盘的远程操控。

### 主要特性
- 🎮 双摇杆控制界面
  - 左摇杆：控制线速度（前进/后退）
  - 右摇杆：控制角速度（左转/右转）
- 📹 实时视频流显示
- 🔐 多设备支持（基于设备ID）
- 📱 移动端友好的响应式设计
- ⚡ 低延迟的控制响应
- 🔄 MQTT 通信保证可靠性

## 技术栈
- 前端：HTML5, CSS3, JavaScript
- 后端：Node.js, Express
- 通信：MQTT, WebSocket
- 控制：nipplejs (虚拟摇杆库)

## 快速开始

### 系统要求
- Node.js >= 14.0.0
- npm >= 6.0.0

### 安装步骤
1. 克隆仓库
```bash
git clone <repository-url>
cd universal-chassis-control
```

2. 安装依赖
```bash
npm install
```

3. 配置系统
编辑 `config.js` 文件，设置：
- MQTT 服务器信息
- 视频流 URL
- 设备认证信息
- 其他系统参数

4. 启动服务器
```bash
# 开发模式（支持热重载）
npm run dev

# 生产模式
npm start
```

### 配置说明

#### MQTT 主题格式
- 控制指令主题：`CarControl_<设备ID>`
- 状态信息主题：`CarStatus_<设备ID>`

示例：设备ID为 "001" 时
- 控制主题：`CarControl_001`
- 状态主题：`CarStatus_001`

#### 设备认证
- 设备ID：3位数字（例如：001, 002, ...）
- 默认密码：88888888

#### 控制参数
```javascript
defaults: {
    speed: 0.5,      // 默认速度 (m/s)
    omega: 0.1,      // 默认角速度 (rad/s)
    acceleration: 10.0, // 默认加速度
    commandDebounceTime: 20  // 命令防抖时间 (ms)
}
```

### 使用说明

1. 访问系统
- 打开浏览器，访问 `http://localhost:3000`
- 输入设备ID和密码登录

2. 控制界面
- 左侧摇杆：上下移动控制前进/后退
- 右侧摇杆：左右移动控制转向
- 滑块控制：
  - 最大速度设置
  - 最大角速度设置
  - 加速度设置

3. 视频显示
- 实时显示设备摄像头画面
- 支持自动重连

## API 说明

### 控制指令格式
```javascript
{
    command: "speed",
    vx: Number,     // 线速度 (-max_speed 到 +max_speed)
    vy: 0.0,        // 横向速度（当前版本未使用）
    omega: Number,  // 角速度 (-max_omega 到 +max_omega)
    acceleration: Number  // 加速度
}
```

### 状态信息格式
设备需要按照约定格式发送状态信息到对应的状态主题。

## 开发说明

### 目录结构
```
universal-chassis-control/
├── config.js          # 配置文件
├── server.js         # 服务器入口
├── control.html      # 控制页面
├── index.html        # 登录页面
├── package.json      # 项目配置
└── README.md         # 说明文档
```

### 开发模式
使用 nodemon 实现热重载：
```bash
npm run dev
```

## 注意事项
1. 生产环境部署时建议：
   - 启用 HTTPS
   - 修改默认密码
   - 配置反向代理
   - 设置环境变量

2. 性能优化：
   - 已实现命令节流，避免过多请求
   - WebSocket 保持长连接
   - 设备特定的主题订阅

## License
MIT

## 贡献指南
欢迎提交 Issue 和 Pull Request

## 更新日志
### v1.0.0
- 初始版本发布
- 支持多设备管理
- 实现双摇杆控制
- 添加视频流显示

## 项目概述

本项目实现了一个基于 Web 的通用底盘控制系统，使用 MQTT 协议与小车进行通信。系统包含登录验证、实时控制界面和状态反馈功能。

### 主要特点

- **直观的控制界面**：菱形排列的五个控制按钮（前进、后退、左转、右转、停止）
- **实时状态反馈**：通过 WebSocket 实时显示小车四个轮子的速度
- **参数调节**：可调节速度和加速度参数
- **响应式设计**：适配各种屏幕尺寸的移动设备
- **安全认证**：简单的密码验证机制
- **集中配置**：所有配置参数统一管理，便于修改

## 技术栈

- **前端**：HTML5, CSS3, JavaScript
- **后端**：Node.js, Express
- **通信**：MQTT, WebSocket
- **会话管理**：Express Session
- **进程管理**：PM2

## 系统架构

```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│   浏览器    │◄────►│  Node.js    │◄────►│  MQTT Broker │
│  (控制界面)  │      │  (服务器)   │      │  (消息中转)  │
└─────────────┘      └─────────────┘      └──────┬──────┘
                                                 │
                                          ┌──────▼──────┐
                                          │   ESP32     │
                                          │  (小车控制) │
                                          └─────────────┘
```

## 快速开始

### 前提条件

- Node.js 14.0+ 和 npm 6.0+
- MQTT Broker (如果需要连接实际小车)

### 安装步骤

1. **克隆项目**

```bash
git clone https://github.com/yourusername/universal-chassis.git
cd universal-chassis
```

2. **安装依赖**

```bash
npm install
```

3. **配置系统**

编辑 `config.js` 文件，根据您的环境修改配置：

```javascript
// 修改 MQTT 连接信息
mqtt: {
    brokerUrl: 'mqtt://your-mqtt-broker:1883',
    options: {
        username: 'your-username',
        password: 'your-password'
    }
}
```

4. **启动开发服务器**

```bash
npm run dev
```

5. **访问控制界面**

打开浏览器访问 `http://localhost:3000`，使用配置文件中设置的密码登录。

## 开发指南

### 项目结构

```
universal-chassis/
├── config.js         # 配置文件
├── server.js         # 服务器入口
├── index.html        # 登录页面
├── control.html      # 控制界面
├── package.json      # 项目依赖
└── README.md         # 项目文档
```

### 开发模式

开发过程中，使用 nodemon 实时监控文件变化并自动重启服务器：

```bash
npm run dev
```

### 修改配置

所有系统配置都集中在 `config.js` 文件中，包括：

- 服务器配置（端口、主机）
- MQTT 连接参数（代理地址、用户名、密码）
- 会话配置（密钥、Cookie 设置）
- 认证配置（登录密码）
- 控制参数默认值（速度、角速度、加速度）

## 生产环境部署

### 使用 PM2 部署

1. **安装 PM2**

```bash
npm install -g pm2
```

2. **启动服务**

```bash
npm run pm2:start
```

3. **查看日志**

```bash
npm run pm2:logs
```

4. **重启服务**

```bash
npm run pm2:restart
```

5. **停止服务**

```bash
npm run pm2:stop
```

### 配置开机自启

```bash
# 保存当前进程列表
pm2 save

# 生成开机自启脚本
pm2 startup

# 按提示执行相应命令
```

### 使用 Nginx 反向代理（推荐）

1. **安装 Nginx**

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install nginx
```

2. **配置 Nginx**

创建配置文件 `/etc/nginx/sites-available/universal-chassis`：

```nginx
server {
    listen 80;
    server_name your-domain.com;

    location / {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
}
```

3. **启用站点**

```bash
sudo ln -s /etc/nginx/sites-available/universal-chassis /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx
```

4. **配置 HTTPS（推荐）**

使用 Let's Encrypt 获取免费 SSL 证书：

```bash
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d your-domain.com
```

### 安全配置

1. **更新配置文件**

```javascript
// config.js
session: {
    secret: 'your-strong-secret-key',
    cookie: {
        maxAge: 3600000, // 1小时
        secure: true     // 启用 HTTPS
    }
},
auth: {
    password: 'your-strong-password'
}
```

2. **配置防火墙**

```bash
# Ubuntu/Debian
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw deny 3000/tcp  # 阻止直接访问 Node.js 服务
```

## MQTT 控制协议

系统使用标准 JSON 格式通过 MQTT 与小车通信：

### 控制命令格式

```json
{
  "command": "speed",
  "vx": 0.5,              // X 方向线速度（m/s）
  "vy": 0.0,              // Y 方向线速度（m/s）
  "omega": 0.1,           // 旋转角速度（rad/s）
  "acceleration": 10.0    // 加速度
}
```

### 停止命令

```json
{
  "command": "stop"
}
```

### 状态信息格式

```json
{
  "vx": 0.0,              // 当前 X 方向线速度
  "vy": 0.0,              // 当前 Y 方向线速度
  "omega": 0.0,           // 当前旋转角速度
  "wheelSpeeds": [0, 0, 0, 0]  // 四个轮子的速度
}
```

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

### 日志查看

```bash
# 查看 PM2 日志
npm run pm2:logs

# 查看 Nginx 日志
sudo tail -f /var/log/nginx/error.log
sudo tail -f /var/log/nginx/access.log
```

## 维护与更新

### 更新系统

```bash
# 拉取最新代码
git pull

# 安装新依赖
npm install

# 重启服务
npm run pm2:restart
```

### 备份配置

```bash
# 备份配置文件
cp config.js config.js.backup
```

## 许可证

MIT License

## 联系方式

作者：wds2dxh  
邮箱：wdsnpshy@163.com