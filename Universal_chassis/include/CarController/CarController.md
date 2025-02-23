# CarController 使用说明

`CarController` 类封装了对四个步进电机的协调控制，并基于运动学模型计算各电机的控制参数，提供两种主要运动模式：

## 1. 速度模式

使用接口：  
```cpp
bool setSpeed(float vx, float vy, float omega, uint32_t duration_ms, [float acceleration]);
```

- **参数说明：**  
  - `vx`：X 方向的线速度（单位：m/s），正值表示前进、负值表示后退  
  - `vy`：Y 方向的线速度（单位：m/s）  
  - `omega`：角速度（单位：rad/s），正值表示逆时针旋转  
  - `duration_ms`：运动持续时间（毫秒）；系统在运动结束后将自动停止所有电机  
  - `acceleration`（可选）：加速度档位（默认值由 `configure()` 接口设置）

调用该接口后，系统会根据运动学模型计算各轮所需转速，然后分解方向信息调用各个步进电机的速度模式接口，并通过多机同步控制确保运动一致。

## 2. 位置模式

使用接口：  
```cpp
bool moveDistance(float dx, float dy, float dtheta, [float acceleration, uint8_t subdivision]);
```

- **参数说明：**  
  - `dx`：X 方向位移（单位：m），正值表示前进  
  - `dy`：Y 方向位移（单位：m）  
  - `dtheta`：旋转角度（单位：rad），正值表示逆时针旋转  
  - `acceleration`（可选）：加速度档位（默认为 `configure()` 中设定的默认值）  
  - `subdivision`（可选）：细分数，用于判断步进电机每圈脉冲数（默认为 `configure()` 中设定的默认值）

调用该接口后，CarController 会根据运动学模型计算出各电机需要的脉冲数，然后分离出旋转（方向）与脉冲幅值，调用电机的绝对位置模式接口进行运动控制。

## 3. 其他接口

- `configure(const CarControllerConfig& config)` 可一次性设置默认的加速度与细分数  
- `getCarState()` 获取当前小车状态（包含各个轮电机的反馈转速）  
- `stop()` 紧急停止所有电机，内部使用同步控制，确保各电机同时执行快速停止命令

## 使用步骤示例

1. **创建步进电机对象**：  
   小车的4个轮分别对应 1234 号电机，例如：
   ```cpp
   StepperMotor motor1(1, &Serial00, ChecksumType::FIXED, 1000);
   StepperMotor motor2(2, &Serial00, ChecksumType::FIXED, 1000);
   StepperMotor motor3(3, &Serial00, ChecksumType::FIXED, 1000);
   StepperMotor motor4(4, &Serial00, ChecksumType::FIXED, 1000);
   ```
2. **创建运动学模型对象**：  
   例如使用普通轮（NormalWheelKinematics）模型，需提供轮子半径和左右轮距：
   ```cpp
   NormalWheelKinematics normalKinematics(0.1f, 0.3f); // 轮子半径 0.1 m, 轮距 0.3 m
   ```
3. **创建 CarController 对象**：  
   将上面4个电机对象和运动学模型传入 CarController 构造函数：
   ```cpp
   CarController carController(&motor1, &motor2, &motor3, &motor4, &normalKinematics);
   ```
4. **调用运动接口**：  
   使用 `setSpeed()` 实现速度控制、使用 `moveDistance()` 实现位置控制。

完整示例请参考 `src/main.cpp` 中的使用代码。
