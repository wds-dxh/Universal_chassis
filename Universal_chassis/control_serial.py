#!/usr/bin/env python3
"""
ESP32小车USB串口控制测试程序

该程序用于测试通过USB串口与ESP32小车进行通信，
实现了控制协议中定义的所有命令，并提供简单的命令行界面。
"""

import serial   # pip install pyserial
import json
import time
import argparse
import threading
from typing import Dict, Any, Optional

class CarController:
    """ESP32小车USB串口控制类"""
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 1.0):
        """
        初始化串口连接
        
        Args:
            port: 串口设备名，如 '/dev/ttyUSB0' 或 'COM3'
            baudrate: 波特率，默认115200
            timeout: 超时时间（秒）
        """
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        self.status_callback = None
        self.running = False
        self.status_thread = None
        
        # 清空缓冲区
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        
        print(f"已连接到 {port}，波特率 {baudrate}")
    
    def close(self):
        """关闭串口连接"""
        self.running = False
        if self.status_thread:
            self.status_thread.join(timeout=1.0)
        self.ser.close()
        print("串口连接已关闭")
    
    def _send_command(self, command: Dict[str, Any]):
        """
        发送JSON格式命令
        
        Args:
            command: 包含命令的字典
        """
        cmd_str = json.dumps(command)
        self.ser.write((cmd_str + '\n').encode())
        self.ser.flush()
        print(f"发送: {cmd_str}")
    
    def _read_response(self) -> Optional[Dict[str, Any]]:
        """
        读取JSON格式响应
        
        Returns:
            解析后的JSON响应，如果没有响应或解析失败则返回None
        """
        try:
            line = self.ser.readline().decode().strip()
            if line:
                print(f"接收: {line}")
                return json.loads(line)
        except json.JSONDecodeError:
            print(f"JSON解析错误: {line}")
        except Exception as e:
            print(f"读取响应错误: {e}")
        return None
    
    def start_status_listener(self, callback=None):
        """
        启动状态监听线程
        
        Args:
            callback: 状态更新回调函数，接收状态字典作为参数
        """
        self.status_callback = callback
        self.running = True
        
        def listener():
            while self.running:
                response = self._read_response()
                if response and self.status_callback:
                    self.status_callback(response)
        
        self.status_thread = threading.Thread(target=listener, daemon=True)
        self.status_thread.start()
    
    def set_speed(self, vx: float, vy: float = 0.0, omega: float = 0.0, acceleration: float = 10.0):
        """
        速度模式控制
        
        Args:
            vx: X方向线速度 (m/s)
            vy: Y方向线速度 (m/s)
            omega: 旋转角速度 (rad/s)
            acceleration: 加速度
        """
        command = {
            "command": "speed",
            "vx": vx,
            "vy": vy,
            "omega": omega,
            "acceleration": acceleration
        }
        self._send_command(command)
    
    def move_distance(self, dx: float, dy: float = 0.0, dtheta: float = 0.0, 
                     speed: float = 1.0, acceleration: float = 10.0, subdivision: int = 256):
        """
        位置模式控制
        
        Args:
            dx: X方向位移 (m)
            dy: Y方向位移 (m)
            dtheta: 旋转角度 (rad)
            speed: 运动速度 (m/s)
            acceleration: 加速度
            subdivision: 细分数
        """
        command = {
            "command": "move",
            "dx": dx,
            "dy": dy,
            "dtheta": dtheta,
            "speed": speed,
            "acceleration": acceleration,
            "subdivision": subdivision
        }
        self._send_command(command)
    
    def stop(self):
        """紧急停止"""
        command = {"command": "stop"}
        self._send_command(command)
    
    def get_status(self):
        """请求获取状态"""
        command = {"command": "get_status"}
        self._send_command(command)
    
    def set_status_interval(self, interval_ms: int):
        """
        设置状态发布间隔
        
        Args:
            interval_ms: 状态发布间隔（毫秒），0表示禁用自动发布
        """
        command = {
            "command": "set_interval",
            "interval": interval_ms
        }
        self._send_command(command)
    
    def set_wifi(self, ssid: str, password: str):
        """
        设置WiFi连接
        
        Args:
            ssid: WiFi网络名称
            password: WiFi密码
        """
        command = {
            "command": "set_wifi",
            "ssid": ssid,
            "password": password
        }
        self._send_command(command)


def print_status(status):
    """打印小车状态信息"""
    print("\n--- 小车状态 ---")
    print(f"线速度X: {status.get('vx', 0):.2f} m/s")
    print(f"线速度Y: {status.get('vy', 0):.2f} m/s")
    print(f"角速度: {status.get('omega', 0):.2f} rad/s")
    
    wheel_speeds = status.get('wheelSpeeds', [0, 0, 0, 0])
    print(f"轮子速度: {wheel_speeds}")
    print("----------------\n")


def interactive_mode(controller):
    """交互式命令行模式"""
    print("\n=== ESP32小车控制测试程序 ===")
    print("可用命令:")
    print("  speed <vx> [vy] [omega] [acceleration] - 速度模式控制")
    print("  move <dx> [dy] [dtheta] [speed] [acceleration] [subdivision] - 位置模式控制")
    print("  stop - 紧急停止")
    print("  status - 获取当前状态")
    print("  interval <ms> - 设置状态发布间隔")
    print("  wifi <ssid> <password> - 设置WiFi连接")
    print("  exit/quit - 退出程序")
    print("==============================\n")
    
    controller.start_status_listener(print_status)
    
    try:
        while True:
            cmd = input("> ").strip()
            if not cmd:
                continue
            
            parts = cmd.split()
            command = parts[0].lower()
            
            if command in ["exit", "quit"]:
                break
            
            elif command == "speed":
                try:
                    vx = float(parts[1]) if len(parts) > 1 else 0.0
                    vy = float(parts[2]) if len(parts) > 2 else 0.0
                    omega = float(parts[3]) if len(parts) > 3 else 0.0
                    accel = float(parts[4]) if len(parts) > 4 else 10.0
                    controller.set_speed(vx, vy, omega, accel)
                except (IndexError, ValueError) as e:
                    print(f"参数错误: {e}")
            
            elif command == "move":
                try:
                    dx = float(parts[1]) if len(parts) > 1 else 0.0
                    dy = float(parts[2]) if len(parts) > 2 else 0.0
                    dtheta = float(parts[3]) if len(parts) > 3 else 0.0
                    speed = float(parts[4]) if len(parts) > 4 else 1.0
                    accel = float(parts[5]) if len(parts) > 5 else 10.0
                    subdiv = int(parts[6]) if len(parts) > 6 else 256
                    controller.move_distance(dx, dy, dtheta, speed, accel, subdiv)
                except (IndexError, ValueError) as e:
                    print(f"参数错误: {e}")
            
            elif command == "stop":
                controller.stop()
            
            elif command == "status":
                controller.get_status()
            
            elif command == "interval":
                try:
                    interval = int(parts[1]) if len(parts) > 1 else 1000
                    controller.set_status_interval(interval)
                except (IndexError, ValueError) as e:
                    print(f"参数错误: {e}")
            
            elif command == "wifi":
                try:
                    ssid = parts[1]
                    password = parts[2]
                    controller.set_wifi(ssid, password)
                except IndexError:
                    print("用法: wifi <ssid> <password>")
            
            else:
                print(f"未知命令: {command}")
    
    except KeyboardInterrupt:
        print("\n程序已中断")
    finally:
        controller.close()


def demo_mode(controller, demo_type="basic"):
    """
    演示模式
    
    Args:
        controller: 控制器对象
        demo_type: 演示类型，可选 "basic"、"square"、"circle"
    """
    controller.start_status_listener(print_status)
    
    try:
        print(f"\n开始 {demo_type} 演示...")
        
        if demo_type == "basic":
            # 基础演示：前进、后退、旋转
            print("前进 0.5 m/s，持续 2 秒")
            controller.set_speed(0.5)
            time.sleep(2)
            
            print("停止")
            controller.stop()
            time.sleep(1)
            
            print("后退 0.5 m/s，持续 2 秒")
            controller.set_speed(-0.5)
            time.sleep(2)
            
            print("停止")
            controller.stop()
            time.sleep(1)
            
            print("原地旋转，持续 2 秒")
            controller.set_speed(0, 0, 0.5)
            time.sleep(2)
            
            print("停止")
            controller.stop()
        
        elif demo_type == "square":
            # 方形路径演示
            side_length = 0.5  # 边长 0.5 米
            
            for i in range(4):
                print(f"移动第 {i+1} 条边")
                controller.move_distance(side_length, 0, 0)
                time.sleep(3)  # 等待移动完成
                
                print("旋转 90 度")
                controller.move_distance(0, 0, 1.57)  # 约 90 度
                time.sleep(2)  # 等待旋转完成
            
            print("方形路径完成")
        
        elif demo_type == "circle":
            # 圆形路径演示（通过速度模式实现）
            print("开始圆形运动，持续 10 秒")
            controller.set_speed(0.3, 0, 0.5)  # 前进速度和旋转速度组合形成圆形路径
            time.sleep(10)
            
            print("停止")
            controller.stop()
        
        else:
            print(f"未知演示类型: {demo_type}")
    
    except KeyboardInterrupt:
        print("\n演示已中断")
    finally:
        controller.stop()
        time.sleep(0.5)
        controller.close()


def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="ESP32小车USB串口控制测试程序")
    parser.add_argument("--port", "-p", required=False, default="/dev/ttyACM0", help="串口设备，如 /dev/ttyUSB0 或 COM3")
    parser.add_argument("--baudrate", "-b", type=int, default=115200, help="波特率，默认115200")
    parser.add_argument("--demo", "-d", choices=["basic", "square", "circle"], help="运行演示模式")
    
    args = parser.parse_args()
    
    try:
        controller = CarController(args.port, args.baudrate)
        
        if args.demo:
            demo_mode(controller, args.demo)
        else:
            interactive_mode(controller)
    
    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except Exception as e:
        print(f"程序错误: {e}")


if __name__ == "__main__":
    main()
