<!--
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-23 16:55:18
 * @Description: 
 * Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
-->
# 群体具身智能--通用底盘控制系统

## 项目概述
本项目旨在实现基于MQTT协议控制小车的前端控制界面，采用前后端混合开发方式（目前先实现前端部分）。

界面主要特点：
- 整体采用蓝色调风格，界面美观、现代，适用于手机浏览器。
- 页面顶部显示小车当前状态（如速度、是否运动等信息）。
- 页面底部有五个按钮，以菱形排列，其中中央按钮为"停止"。
  - 上方按钮：前进
  - 左侧按钮：左转
  - 中央按钮：停止
  - 右侧按钮：右转
  - 下方按钮：后退
- 按钮支持长按交互，且长按时不会触发复制、剪切等默认行为。
- 代码结构规范清晰，注释详细，便于后续维护和扩展。

## 技术栈
- 前端：HTML5, CSS3, JavaScript
- 后端：Node.js/Python
- 通信：MQTT, WebSocket
- 会话管理：Express Session
- 进程管理：PM2

## 系统要求
- Node.js 14.0+
- npm 6.0+
- Python 3.8+ (可选，如果使用Python后端)
- Linux/Windows 服务器

## 文件说明
- index.html：登录页面，用于验证用户身份
- control.html：主控制界面，包含所有控制功能
- server.js：Node.js 后端服务文件
- server.py：Python 后端服务文件（可选）
- readme.md：项目说明文档

## 使用方法

### 安装依赖
```bash
# 安装 Node.js 依赖
npm install express body-parser mqtt cors ws path

# 安装 pm2 用于后台运行
npm install -g pm2
```

### 启动服务
```bash
# 使用 pm2 后台运行服务
pm2 start server.js --name "car-control"

# 查看运行状态
pm2 status

# 查看日志
pm2 logs car-control

# 停止服务
pm2 stop car-control

# 删除服务
pm2 delete car-control
```

### 访问控制界面
1. 打开浏览器访问 `http://localhost:3000`
2. 输入学号 `8888888` 进入控制界面
3. 使用控制界面操作小车

### 配置 pm2 开机自启
```bash
# 保存当前进程列表
pm2 save

# 生成开机自启脚本
pm2 startup

# 按提示执行相应命令
```

## 部署步骤

### 1. 环境准备
```bash
# 安装 Node.js 和 npm (Ubuntu/Debian)
curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
sudo apt-get install -y nodejs

# 安装项目依赖
npm install express body-parser mqtt cors ws path express-session
npm install -g pm2

# 如果使用Python后端，安装Python依赖
pip install fastapi pydantic paho-mqtt uvicorn
```

### 2. 配置文件检查
1. 确保 MQTT 配置正确：
   ```javascript
   const mqttOptions = {
     username: 'emqx_u',
     password: 'public'
   };
   const mqttBrokerUrl = 'mqtt://ctl_car.dxh-wds.top:1883';
   ```

2. 检查服务器端口配置：
   ```javascript
   server.listen(3000, ...);
   ```

### 3. 生产环境部署

#### 使用 PM2 部署
```bash
# 安装 PM2
npm install -g pm2

# 启动服务
pm2 start server.js --name "car-control"

# 配置开机自启
pm2 startup
pm2 save

# 查看日志
pm2 logs car-control

# 监控应用状态
pm2 monit
```

#### 配置 Nginx 反向代理（推荐）
```nginx
# /etc/nginx/sites-available/car-control
server {
    listen 80;
    server_name your_domain.com;

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

### 4. 安全配置

#### 会话配置
```javascript
app.use(session({
    secret: 'your-secret-key',
    resave: false,
    saveUninitialized: true,
    cookie: { 
        secure: true,  // 生产环境启用 HTTPS
        maxAge: 1800000 // 30分钟过期
    }
}));
```

#### 防火墙配置
```bash
# Ubuntu/Debian
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw allow 3000/tcp
sudo ufw allow 1883/tcp  # MQTT
```

### 5. 维护指南

#### 日常维护命令
```bash
# 查看应用状态
pm2 status

# 重启应用
pm2 restart car-control

# 更新代码后重新加载
pm2 reload car-control

# 查看详细日志
pm2 logs car-control --lines 100
```

#### 备份策略
```bash
# 备份配置文件
cp server.js server.js.backup
cp package.json package.json.backup

# 如果使用 Git
git add .
git commit -m "Backup before deployment"
git push
```

### 6. 故障排查

#### 常见问题
1. 无法连接 MQTT：
   - 检查 MQTT 服务器状态
   - 验证用户名密码
   - 确认防火墙配置

2. WebSocket 连接失败：
   - 检查 Nginx 配置
   - 确认端口开放状态
   - 查看服务器日志

3. 会话失效：
   - 检查 session 配置
   - 验证 cookie 设置
   - 确认超时时间

#### 日志查看
```bash
# PM2 日志
pm2 logs

# Nginx 日志
sudo tail -f /var/log/nginx/error.log
sudo tail -f /var/log/nginx/access.log

# 系统日志
sudo journalctl -u nginx
sudo journalctl -u pm2-root
```

## 安全说明
- 当前使用简单的密码验证机制
- 生产环境建议添加更完善的身份验证和会话管理
- 建议定期更改密码并限制访问IP

## MQTT 控制协议说明

该协议用于通过 MQTT 与 ESP32 小车控制系统进行交互。系统采用 JSON 格式的数据传输，支持下发小车运动控制命令与发布小车状态信息。

---

### 1. MQTT 连接参数

- **Broker 地址**: `47.108.223.146`
- **端口**: `1883`
- **用户名**: `emqx_u`
- **密码**: `public`

---

### 2. MQTT 主题

- **控制主题**: `CarControl`  
  外部系统下发控制命令到此主题，ESP32 订阅此主题。

- **状态主题**: `CarStatus`  
  ESP32 定期发布小车当前状态到此主题，外部系统可订阅监控状态。

---

### 3. 控制命令格式

所有控制命令均使用 JSON 格式传输，数据格式及各参数说明如下：

#### 3.1 速度模式控制指令

用于设置小车以指定速度运动，运动结束后系统会自动停止电机。

**JSON 示例**:
````

## 开发指南

### 本地开发环境设置
```bash
# 克隆项目
git clone [repository_url]
cd WebControl

# 安装依赖
npm install

# 启动开发服务器
node server.js
```

### 代码提交规范
```bash
git add .
git commit -m "type: description"
git push
```

提交类型:
- feat: 新功能
- fix: 修复问题
- docs: 文档更新
- style: 代码格式
- refactor: 重构
- test: 测试相关
- chore: 构建过程或辅助工具的变动

## 许可证
MIT License

## 联系方式
作者：wds2dxh
邮箱：wdsnpshy@163.com