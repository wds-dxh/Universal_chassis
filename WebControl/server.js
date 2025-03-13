/**
 * @file 通用底盘控制系统服务器
 * @description 处理HTTP请求、WebSocket连接和MQTT通信
 */

const express = require('express');
const bodyParser = require('body-parser');
const mqtt = require('mqtt');
const cors = require('cors');
const WebSocket = require('ws');
const http = require('http');
const path = require('path');
const session = require('express-session');

// 导入配置文件
const config = require('./config');

// 创建Express应用和HTTP服务器
const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// 中间件配置
app.use(cors());
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// 会话配置
app.use(session({
    secret: config.session.secret,
    resave: false,
    saveUninitialized: true,
    cookie: config.session.cookie
}));

// 验证中间件
function requireAuth(req, res, next) {
    if (req.session.authenticated) {
        next();
    } else {
        res.redirect('/');
    }
}

// 静态文件服务
app.use(express.static(__dirname));

// 路由配置
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// 登录验证接口
app.post('/login', (req, res) => {
    console.log('Login attempt:', req.body);
    const { deviceId, password } = req.body;
    
    // 验证设备ID格式
    if (!config.auth.deviceIdPattern.test(deviceId)) {
        return res.status(401).json({
            success: false,
            message: '设备编号格式不正确'
        });
    }
    
    // 验证密码
    if (password === config.auth.password) {
        req.session.authenticated = true;
        req.session.deviceId = deviceId;  // 保存设备ID到会话
        req.session.loginTime = new Date();
        res.json({ success: true });
    } else {
        res.status(401).json({ 
            success: false, 
            message: '密码错误'
        });
    }
});

// 控制页面路由
app.get('/control', requireAuth, (req, res) => {
    res.sendFile(path.join(__dirname, 'control.html'));
});

// 通配符路由
app.get('*', (req, res) => {
    res.redirect('/');
});

// MQTT客户端
const mqttClient = mqtt.connect(config.mqtt.brokerUrl, config.mqtt.options);

// 存储WebSocket连接
const wsClients = new Set();

// MQTT连接和订阅
mqttClient.on('connect', () => {
    console.log('Connected to MQTT broker:', config.mqtt.brokerUrl);
    // 不需要在这里订阅任何主题，因为我们会在WebSocket连接时
    // 根据设备ID动态订阅对应的主题
});

// WebSocket连接处理
wss.on('connection', (ws, req) => {
    // 从会话中获取设备ID
    const deviceId = req.session?.deviceId;
    if (!deviceId) {
        ws.close();
        return;
    }
    
    wsClients.add(ws);
    console.log(`New WebSocket connection for device ${deviceId}, total clients:`, wsClients.size);
    
    // 订阅特定设备的状态主题
    const statusTopic = config.mqtt.topicPrefix.status + deviceId;
    mqttClient.subscribe(statusTopic, (err) => {
        if (!err) {
            console.log('Subscribed to topic:', statusTopic);
        } else {
            console.error('Failed to subscribe:', err);
        }
    });
    
    ws.deviceId = deviceId; // 保存设备ID到WebSocket连接
    
    ws.on('close', () => {
        // 取消订阅该设备的主题
        const statusTopic = config.mqtt.topicPrefix.status + deviceId;
        mqttClient.unsubscribe(statusTopic, (err) => {
            if (err) {
                console.error('Failed to unsubscribe:', err);
            }
        });
        
        wsClients.delete(ws);
        console.log('WebSocket disconnected, remaining clients:', wsClients.size);
    });
});

// 处理MQTT消息
mqttClient.on('message', (topic, message) => {
    const deviceId = topic.split('_')[1]; // 从主题中提取设备ID
    if (!deviceId) return;
    
    try {
        const status = JSON.parse(message.toString());
        // 只向对应设备的客户端发送消息
        wsClients.forEach(client => {
            if (client.deviceId === deviceId && client.readyState === WebSocket.OPEN) {
                client.send(JSON.stringify(status));
            }
        });
    } catch (error) {
        console.error('Error processing MQTT message:', error);
    }
});

// 控制接口 - 修复路由冲突问题
app.post('/control/:action', requireAuth, (req, res) => {
    const action = req.params.action;
    const deviceId = req.session.deviceId;
    const { speed, omega, acceleration } = req.body;
    let payload;

    // 根据控制协议构建命令
    switch(action) {
        case 'forward':
            payload = {
                command: "speed",
                vx: speed ? Number(speed) : config.defaults.speed,
                vy: 0.0,
                omega: 0.0,
                acceleration: acceleration ? Number(acceleration) : config.defaults.acceleration
            };
            break;
        case 'backward':
            payload = {
                command: "speed",
                vx: speed ? -Number(speed) : -config.defaults.speed,
                vy: 0.0,
                omega: 0.0,
                acceleration: acceleration ? Number(acceleration) : config.defaults.acceleration
            };
            break;
        case 'left':
            payload = {
                command: "speed",
                vx: 0.0,
                vy: 0.0,
                omega: omega ? Number(omega) : config.defaults.omega,
                acceleration: acceleration ? Number(acceleration) : config.defaults.acceleration
            };
            break;
        case 'right':
            payload = {
                command: "speed",
                vx: 0.0,
                vy: 0.0,
                omega: omega ? -Number(omega) : -config.defaults.omega,
                acceleration: acceleration ? Number(acceleration) : config.defaults.acceleration
            };
            break;
        case 'stop':
            payload = { command: "stop" };
            break;
        case 'move':
            // 处理摇杆控制命令
            payload = {
                command: "speed",
                vx: speed !== undefined ? Number(speed) : 0,
                vy: 0.0,
                omega: omega !== undefined ? Number(omega) : 0,
                acceleration: acceleration ? Number(acceleration) : config.defaults.acceleration
            };
            break;
        default:
            return res.status(400).json({ error: "Invalid action" });
    }

    // 发布到特定设备的控制主题
    const controlTopic = config.mqtt.topicPrefix.control + deviceId;
    mqttClient.publish(controlTopic, JSON.stringify(payload), (err) => {
        if (err) {
            console.error('MQTT publish error:', err);
            return res.status(500).json({ error: "Failed to send command" });
        }
        console.log('Command sent:', action, payload);
        res.json({ success: true, message: "Command sent" });
    });
});

// 配置接口 - 提供前端所需的配置信息
app.get('/config', requireAuth, (req, res) => {
    // 只返回前端需要的配置信息
    const clientConfig = {
        video: config.video
    };
    
    res.json(clientConfig);
});

// 启动服务器
server.listen(config.server.port, () => {
    console.log(`Server running on http://${config.server.host}:${config.server.port}`);
    console.log('Visit http://localhost:' + config.server.port + ' to access the control panel');
});

// 错误处理
mqttClient.on('error', (err) => {
    console.error('MQTT client error:', err);
});

process.on('uncaughtException', (err) => {
    console.error('Uncaught exception:', err);
});
