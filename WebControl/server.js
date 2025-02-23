/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-23 23:45:03
 * @Description: 
 * Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
 */

const express = require('express');
const bodyParser = require('body-parser');
const mqtt = require('mqtt');
const cors = require('cors');  // 引入 cors
const WebSocket = require('ws');
const http = require('http');
const path = require('path');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// 修改路由配置
app.use(express.static(__dirname));

// 登录页面路由
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// 控制页面路由
app.get('/control', (req, res) => {
    res.sendFile(path.join(__dirname, 'control.html'));
});

// 添加通配符路由，处理其他所有请求
app.get('*', (req, res) => {
    res.redirect('/');
});

// MQTT 连接配置
const mqttOptions = {
  username: 'emqx_u',
  password: 'public'
};
const mqttBrokerUrl = 'mqtt://47.108.223.146:1883';
const client = mqtt.connect(mqttBrokerUrl, mqttOptions);

// 存储所有活动的 WebSocket 连接
const clients = new Set();

// 订阅状态主题
client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe('CarStatus', (err) => {
    if (!err) {
      console.log('Subscribed to CarStatus topic');
    }
  });
});

// 处理 MQTT 状态消息
client.on('message', (topic, message) => {
  if (topic === 'CarStatus') {
    try {
      const status = JSON.parse(message.toString());
      // 广播状态给所有连接的 WebSocket 客户端
      clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(JSON.stringify(status));
        }
      });
    } catch (error) {
      console.error('Error processing MQTT message:', error);
    }
  }
});

// WebSocket 连接处理
wss.on('connection', (ws) => {
  clients.add(ws);
  ws.on('close', () => clients.delete(ws));
});

app.use(cors()); // 使用 cors 中间件
app.use(bodyParser.json());

// 接口：接收动作请求，并发布 MQTT 控制命令
app.post('/control/:action', (req, res) => {
  const action = req.params.action;  // forward, backward, left, right, stop
  const { speed, omega, acceleration } = req.body;  // 前端进度条设置的参数
  let payload;

  switch(action) {
    case 'forward':
      payload = {
        command: "speed",
        vx: speed ? Number(speed) * 0.27778 : 0.5,  // km/h 到 m/s 转换（约1 km/h = 0.27778 m/s）
        vy: 0.0,
        omega: 0.0,
        duration: 0,
        acceleration: acceleration ? Number(acceleration) : 10.0
      };
      break;
    case 'backward':
      payload = {
        command: "speed",
        vx: speed ? -Number(speed) * 0.27778 : -0.5,
        vy: 0.0,
        omega: 0.0,
        duration: 0,
        acceleration: acceleration ? Number(acceleration) : 10.0
      };
      break;
    case 'left':
      payload = {
        command: "speed",
        vx: 0.0,
        vy: 0.0,
        omega: omega ? Number(omega) : 0.1,  // 左转（逆时针）
        duration: 0,
        acceleration: acceleration ? Number(acceleration) : 10.0
      };
      break;
    case 'right':
      payload = {
        command: "speed",
        vx: 0.0,
        vy: 0.0,
        omega: omega ? -Number(omega) : -0.1,  // 右转（顺时针）
        duration: 0,
        acceleration: acceleration ? Number(acceleration) : 10.0
      };
      break;
    case 'stop':
      payload = { command: "stop" };
      break;
    default:
      return res.status(400).send("Invalid action");
  }

  // 发布命令至 MQTT 主题 CarControl
  client.publish('CarControl', JSON.stringify(payload), (err) => {
    if (err) {
      return res.status(500).send("MQTT publish error");
    }
    res.send("Command sent");
  });
});

server.listen(3000, () => {
  console.log('Server running on port 3000');
  console.log('Visit http://localhost:3000 to access the control panel');
});
