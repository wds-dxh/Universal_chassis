#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import json
import time
import argparse
import threading
import re
from colorama import init, Fore, Style      # pip install colorama

# 初始化colorama
init()

class CarController:
    def __init__(self, port, baudrate=460800):
        """初始化小车控制器"""
        self.serial = serial.Serial(port, baudrate, timeout=0.1)
        self.running = True
        self.status_lock = threading.Lock()
        self.latest_status = None
        self.rx_buffer = ""  # 接收缓冲区
        
        # 启动状态接收线程
        self.rx_thread = threading.Thread(target=self._receive_status, daemon=True)
        self.rx_thread.start()
    
    def _receive_status(self):
        """接收并解析小车状态的线程函数"""
        while self.running:
            try:
                # 读取串口数据
                data = self.serial.read(1024).decode('utf-8', errors='ignore')
                if data:
                    # 添加到接收缓冲区
                    self.rx_buffer += data
                    
                    # 尝试从缓冲区中提取完整的JSON对象
                    self._process_buffer()
                    
                time.sleep(0.01)  # 短暂休眠，减少CPU占用
            except Exception as e:
                print(f"{Fore.RED}接收状态时出错: {e}{Style.RESET_ALL}")
    
    def _process_buffer(self):
        """处理接收缓冲区，提取完整的JSON对象"""
        # 查找可能的JSON对象（以{开始，以}结束）
        while True:
            # 查找第一个{和最后一个}
            start = self.rx_buffer.find('{')
            if start == -1:
                # 没有找到{，清空缓冲区
                self.rx_buffer = ""
                return
            
            end = self.rx_buffer.find('}', start)
            if end == -1:
                # 没有找到}，保留缓冲区等待更多数据
                return
            
            # 提取可能的JSON字符串
            json_str = self.rx_buffer[start:end+1]
            
            # 更新缓冲区，移除已处理的部分
            self.rx_buffer = self.rx_buffer[end+1:]
            
            # 尝试解析JSON
            try:
                status = json.loads(json_str)
                # 解析成功，更新状态
                with self.status_lock:
                    self.latest_status = status
                print(f"接收: {json_str}")
                self._print_status(status)
            except json.JSONDecodeError as e:
                print(f"{Fore.YELLOW}JSON解析错误: {json_str}{Style.RESET_ALL}")
    
    def _print_status(self, status):
        """打印小车状态信息"""
        print(f"\n{Fore.CYAN}--- 小车状态 ---{Style.RESET_ALL}")
        print(f"线速度X: {status.get('vx', 0):.2f} m/s")
        print(f"线速度Y: {status.get('vy', 0):.2f} m/s")
        print(f"角速度: {status.get('omega', 0):.2f} rad/s")
        print(f"轮子速度: {status.get('wheelSpeeds', [0, 0, 0, 0])}")
        print(f"{Fore.CYAN}----------------{Style.RESET_ALL}\n")
    
    def send_command(self, command):
        """发送命令到小车"""
        try:
            # 确保命令是JSON格式
            if not command.startswith('{'):
                # 尝试解析简单命令
                parts = command.strip().split()
                cmd_type = parts[0].lower()
                
                if cmd_type == "speed" and len(parts) >= 2:
                    # speed vx [vy] [omega] [acceleration]
                    vx = float(parts[1])
                    vy = float(parts[2]) if len(parts) > 2 else 0.0
                    omega = float(parts[3]) if len(parts) > 3 else 0.0
                    accel = float(parts[4]) if len(parts) > 4 else 10.0
                    command = json.dumps({
                        "command": "speed",
                        "vx": vx,
                        "vy": vy,
                        "omega": omega,
                        "acceleration": accel
                    })
                
                elif cmd_type == "move" and len(parts) >= 2:
                    # move dx [dy] [dtheta] [speed] [acceleration] [subdivision]
                    dx = float(parts[1])
                    dy = float(parts[2]) if len(parts) > 2 else 0.0
                    dtheta = float(parts[3]) if len(parts) > 3 else 0.0
                    speed = float(parts[4]) if len(parts) > 4 else 1.0
                    accel = float(parts[5]) if len(parts) > 5 else 10.0
                    subdiv = int(parts[6]) if len(parts) > 6 else 256
                    command = json.dumps({
                        "command": "move",
                        "dx": dx,
                        "dy": dy,
                        "dtheta": dtheta,
                        "speed": speed,
                        "acceleration": accel,
                        "subdivision": subdiv
                    })
                
                elif cmd_type == "stop":
                    command = json.dumps({"command": "stop"})
                
                elif cmd_type == "status" or cmd_type == "get_status":
                    command = json.dumps({"command": "get_status"})
                
                elif cmd_type == "interval" and len(parts) >= 2:
                    interval = int(parts[1])
                    command = json.dumps({
                        "command": "set_interval",
                        "interval": interval
                    })
                
                elif cmd_type == "wifi" and len(parts) >= 3:
                    ssid = parts[1]
                    password = parts[2]
                    command = json.dumps({
                        "command": "set_wifi",
                        "ssid": ssid,
                        "password": password
                    })
                
                else:
                    print(f"{Fore.RED}未知命令: {command}{Style.RESET_ALL}")
                    return
            
            # 发送命令
            print(f"{Fore.GREEN}发送: {command}{Style.RESET_ALL}")
            self.serial.write((command + '\n').encode('utf-8'))
            self.serial.flush()
        
        except Exception as e:
            print(f"{Fore.RED}发送命令时出错: {e}{Style.RESET_ALL}")
    
    def get_latest_status(self):
        """获取最新的小车状态"""
        with self.status_lock:
            return self.latest_status
    
    def close(self):
        """关闭控制器，释放资源"""
        self.running = False
        if self.rx_thread.is_alive():
            self.rx_thread.join(1.0)  # 等待接收线程结束
        if self.serial.is_open:
            self.serial.close()

def run_demo(controller, demo_type):
    """运行演示模式"""
    print(f"{Fore.MAGENTA}开始 {demo_type} 演示...{Style.RESET_ALL}")
    
    if demo_type == "basic":
        # 基础演示：前进、停止、后退、停止
        controller.send_command('{"command":"speed","vx":0.5,"vy":0,"omega":0}')
        time.sleep(2)
        controller.send_command('{"command":"stop"}')
        time.sleep(1)
        controller.send_command('{"command":"speed","vx":-0.5,"vy":0,"omega":0}')
        time.sleep(2)
        controller.send_command('{"command":"stop"}')
    
    elif demo_type == "square":
        # 方形路径演示
        # 前进1米
        controller.send_command('{"command":"move","dx":1.0,"dy":0,"dtheta":0,"speed":0.5}')
        time.sleep(3)
        # 左转90度
        controller.send_command('{"command":"move","dx":0,"dy":0,"dtheta":1.57,"speed":0.5}')
        time.sleep(2)
        # 前进1米
        controller.send_command('{"command":"move","dx":1.0,"dy":0,"dtheta":0,"speed":0.5}')
        time.sleep(3)
        # 左转90度
        controller.send_command('{"command":"move","dx":0,"dy":0,"dtheta":1.57,"speed":0.5}')
        time.sleep(2)
        # 前进1米
        controller.send_command('{"command":"move","dx":1.0,"dy":0,"dtheta":0,"speed":0.5}')
        time.sleep(3)
        # 左转90度
        controller.send_command('{"command":"move","dx":0,"dy":0,"dtheta":1.57,"speed":0.5}')
        time.sleep(2)
        # 前进1米
        controller.send_command('{"command":"move","dx":1.0,"dy":0,"dtheta":0,"speed":0.5}')
        time.sleep(3)
        # 左转90度（回到原点和方向）
        controller.send_command('{"command":"move","dx":0,"dy":0,"dtheta":1.57,"speed":0.5}')
        time.sleep(2)
    
    elif demo_type == "circle":
        # 圆形路径演示 - 使用速度模式
        print("开始圆形运动...")
        # 设置前进速度和角速度，形成圆周运动
        controller.send_command('{"command":"speed","vx":0.5,"vy":0,"omega":0.5}')
        time.sleep(12.56)  # 大约走完一个圆 (2*pi/0.5 = 12.56秒)
        controller.send_command('{"command":"stop"}')
    
    print(f"{Fore.MAGENTA}演示结束{Style.RESET_ALL}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="ESP32小车USB串口控制测试程序")
    parser.add_argument("--port", "-p", required=False, default="/dev/ttyACM0", help="串口设备，如 /dev/ttyUSB0 或 COM3")
    parser.add_argument("--baudrate", "-b", type=int, default=460800, help="波特率，默认460800")
    parser.add_argument("--demo", "-d", choices=["basic", "square", "circle"], help="运行演示模式")
    
    args = parser.parse_args()
    
    try:
        print(f"{Fore.GREEN}连接到小车控制器 ({args.port}, {args.baudrate})...{Style.RESET_ALL}")
        controller = CarController(args.port, args.baudrate)
        
        # 如果指定了演示模式，运行演示后退出
        if args.demo:
            run_demo(controller, args.demo)
            controller.close()
            return
        
        print(f"{Fore.GREEN}连接成功! 输入命令控制小车，输入 'exit' 或 'quit' 退出{Style.RESET_ALL}")
        print(f"{Fore.YELLOW}支持的简单命令格式:{Style.RESET_ALL}")
        print("  speed <vx> [vy] [omega] [acceleration]")
        print("  move <dx> [dy] [dtheta] [speed] [acceleration] [subdivision]")
        print("  stop")
        print("  status 或 get_status")
        print("  interval <ms>")
        print("  wifi <ssid> <password>")
        print(f"{Fore.YELLOW}也可以直接输入JSON格式命令{Style.RESET_ALL}")
        
        # 主循环 - 读取用户输入并发送命令
        while True:
            try:
                command = input(f"{Fore.GREEN}> {Style.RESET_ALL}")
                if command.lower() in ['exit', 'quit']:
                    break
                
                if command.strip():
                    controller.send_command(command)
            
            except KeyboardInterrupt:
                print("\n退出...")
                break
            
            except Exception as e:
                print(f"{Fore.RED}错误: {e}{Style.RESET_ALL}")
        
        controller.close()
        print(f"{Fore.GREEN}已断开连接{Style.RESET_ALL}")
    
    except serial.SerialException as e:
        print(f"{Fore.RED}无法打开串口 {args.port}: {e}{Style.RESET_ALL}")
    
    except Exception as e:
        print(f"{Fore.RED}发生错误: {e}{Style.RESET_ALL}")

if __name__ == "__main__":
    main()
