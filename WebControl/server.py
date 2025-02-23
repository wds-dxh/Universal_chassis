'''
Author: wds2dxh wdsnpshy@163.com
Date: 2025-02-23 23:55:13
Description: 
Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
'''
from fastapi import FastAPI, HTTPException, WebSocket
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import paho.mqtt.client as mqtt
import json

app = FastAPI()

# 添加 CORS 中间件
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 允许所有来源
    allow_credentials=True,
    allow_methods=["*"],  # 允许所有方法
    allow_headers=["*"],  # 允许所有请求头
)

# MQTT 连接配置
mqtt_broker_url = '47.108.223.146'
mqtt_broker_port = 1883
mqtt_username = 'emqx_u'
mqtt_password = 'public'
mqtt_topic_control = 'CarControl'
mqtt_topic_status = 'CarStatus'

# 存储所有活动的 WebSocket 连接
active_connections = []

# MQTT 状态回调
def on_message(client, userdata, message):
    try:
        status = json.loads(message.payload.decode())
        # 将状态广播给所有连接的 WebSocket 客户端
        for connection in active_connections:
            await connection.send_json(status)
    except Exception as e:
        print(f"Error processing MQTT message: {e}")

# 设置 MQTT 客户端
client = mqtt.Client()
client.username_pw_set(mqtt_username, mqtt_password)
client.on_message = on_message
client.connect(mqtt_broker_url, mqtt_broker_port, 60)
client.subscribe(mqtt_topic_status)
client.loop_start()  # 在后台线程中启动 MQTT 循环

# WebSocket 连接处理
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    active_connections.append(websocket)
    try:
        while True:
            await websocket.receive_text()  # 保持连接活跃
    except:
        active_connections.remove(websocket)

class ControlRequest(BaseModel):
    speed: float
    omega: float
    acceleration: float

@app.post("/control/{action}")
async def control_car(action: str, request: ControlRequest):
    if action not in ["forward", "backward", "left", "right", "stop"]:
        raise HTTPException(status_code=400, detail="Invalid action")

    payload = {
        "command": "speed",
        "vx": 0.0,
        "vy": 0.0,
        "omega": 0.0,
        "duration": 0,
        "acceleration": request.acceleration
    }

    if action == "forward":
        payload["vx"] = request.speed * 0.27778  # km/h 到 m/s 转换
    elif action == "backward":
        payload["vx"] = -request.speed * 0.27778
    elif action == "left":
        payload["omega"] = request.omega
    elif action == "right":
        payload["omega"] = -request.omega
    elif action == "stop":
        payload["command"] = "stop"

    client.publish(mqtt_topic_control, payload=str(payload))
    return {"message": "Command sent"}
