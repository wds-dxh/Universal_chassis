# StepperMotor 驱动使用文档

## 1. 概述

本驱动用于控制采用 Emm42_V5.0 闭环驱动的步进电机，支持以下功能：
- **控制命令**  
  - 电机使能（开启/关闭）
  - 速度模式运动控制
  - 位置模式运动控制
  - 立即停止
  - 多机同步运动

- **参数读取**  
  - 读取固件版本和硬件版本
  - 读取相电阻与相电感
  - 读取位置环 PID 参数
  - 读取总线电压、相电流
  - 读取经过校准的编码器值
  - 读取输入脉冲数、目标/实时位置、位置误差
  - 读取电机状态、驱动配置参数及系统状态

- **参数修改**  
  - 修改细分（步进细分设置）
  - 修改电机 ID 地址
  - 切换开环/闭环模式
  - 修改开环模式工作电流
  - 修改驱动配置参数
  - 修改位置环 PID 参数
  - 存储速度模式参数
  - 修改通讯控制输入速度缩放（缩小10倍）

## 2. 通讯协议简介

### 2.1 命令帧格式

所有命令帧格式统一为：  
&nbsp;&nbsp;&nbsp;&nbsp;**地址 + 功能码 + 指令数据 + 校验字节**

- **地址（ID_Addr）**  
  1 字节，范围 0（广播）～255（推荐使用 1～10），0 为广播，仅地址为 1 的电机会回复广播命令。

- **功能码**  
  1 字节，不同命令对应不同的功能码（例如：0xF3 表示电机使能控制，0xF6 表示速度模式控制等）。

- **指令数据**  
  根据具体命令的规定，可包含子命令、参数、是否存储标志等。

- **校验字节**  
  支持固定校验（默认 0x6B）、XOR 校验或 CRC-8 校验。

### 2.2 主要命令说明

#### 2.2.1 控制动作命令
- **电机使能**  
  - 格式：`地址 + 0xF3 + 0xAB + 使能状态 + 多机同步标志 + 校验字节`  
  - 示例：发送 `01 F3 AB 01 00 6B`（使能）；回复 `01 F3 02 6B` 表示成功。

- **速度模式控制**  
  - 格式：`地址 + 0xF6 + 方向 + 速度（2字节）+ 加速度 + 多机同步标志 + 校验字节`

- **位置模式控制**  
  - 格式：`地址 + 0xFD + 方向 + 速度（2字节） + 加速度 + 脉冲数（4字节）+ 相对/绝对标志 + 多机同步标志 + 校验字节`

- **立即停止**  
  - 格式：`地址 + 0xFE + 0x98 + 多机同步标志 + 校验字节`

- **多机同步运动**  
  - 格式：`地址 + 0xFF + 0x66 + 校验字节`

#### 2.2.2 参数读取命令
- **固件和硬件版本读取**  
  - 命令：`地址 + 0x1F + 校验字节`  
  - 返回：`地址 + 0x1F + 固件版本号 + 硬件版本号 + 校验字节`

- **相电阻/相电感**  
  - 命令：`地址 + 0x20 + 校验字节`  
  - 返回：`地址 + 0x20 + 相电阻（2字节）+ 相电感（2字节）+ 校验字节`

- **其他读取命令**  
  读取位置环 PID、总线电压、相电流、编码器值、输入脉冲、目标/实时位置、位置误差、电机状态、驱动配置参数、系统状态等，具体格式请参照协议说明和代码注释。

#### 2.2.3 参数修改命令
- **修改细分**  
  - 命令：`地址 + 0x84 + 0x8A + 存储标志 + 细分值 + 校验字节`  
    （注：细分值 00 表示 256 细分，其它值为对应细分数）

- **修改 ID 地址**  
  - 命令：`地址 + 0xAE + 0x4B + 存储标志 + 新 ID 地址 + 校验字节`

- **切换开环／闭环模式**  
  - 命令：`地址 + 0x46 + 0x69 + 存储标志 + 模式值（0x01=开环，0x02=闭环）+ 校验字节`

- **修改开环工作电流**  
  - 命令：`地址 + 0x44 + 0x33 + 存储标志 + 电流值（2字节）+ 校验字节`

- **修改驱动配置参数**、**位置环 PID 参数**、**存储速度模式参数**、**修改输入速度缩放**  
  具体格式请参照相关协议说明和代码实现。

## 3. API 参考

所有 API 均定义在 `StepperMotor` 类中，常用接口如下：

### 3.1 控制命令接口
- `bool enableMotor(bool enable, bool sync = false);`  
  使能或失能电机，`sync` 用于是否采用多机同步模式。

- `bool setSpeedMode(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, bool sync = false);`  
  设置速度模式运动。（direction：0 为 CW，1 为 CCW）

- `bool setPositionMode(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, uint32_t pulse, bool absolute, bool sync = false);`  
  设置位置模式运动指令。

- `bool stopMotor(bool sync = false);`  
  立即停止电机运动。

- `bool syncMove();`  
  触发多机同步运动（此前下发同步指令后，由本命令使所有电机同时动作）。

### 3.2 参数读取接口
- `bool readFirmwareVersion(uint8_t &firmware, uint8_t &hardware);`
- `bool readPhaseResistanceInductance(uint16_t &resistance, uint16_t &inductance);`
- `bool readPIDParameters(uint32_t &Kp, uint32_t &Ki, uint32_t &Kd);`
- *…… 其他读取接口请参阅头文件说明。*

### 3.3 参数修改接口
- `bool modifySubdivision(uint8_t subdivision, bool store);`
- `bool modifyMotorID(uint8_t newID, bool store);`
- `bool switchControlMode(uint8_t mode, bool store);`
- `bool modifyOpenLoopCurrent(uint16_t current, bool store);`
- `bool modifyDriverConfig(const std::vector<uint8_t>& configData, bool store);`
- `bool modifyPIDParameters(uint32_t Kp, uint32_t Ki, uint32_t Kd, bool store);`
- `bool storeSpeedModeParameters(uint8_t direction, uint16_t speedRpm, uint8_t accelerateLevel, bool enableEn, bool store);`
- `bool modifyInputSpeedScaling(bool enable, bool store);`

## 4. 数据结构

### 4.1 DriverConfig
封装了驱动器的配置参数，包括：
- 电机类型、脉冲控制模式、通讯端口模式
- 细分值、细分插补、自动熄屏功能
- 开环/闭环工作电流、电压、波特率、CAN 通讯速率
- 堵转保护相关参数等

### 4.2 SystemStatus
封装了系统状态参数，如：
- 总线电压、相电流
- 编码器校准值、目标位置、实时位置
- 位置误差以及电机状态标志等

## 5. 使用流程

1. **硬件接线**：  
   根据多机通讯和同步控制要求，将步进电机及其驱动器按协议要求接线。

2. **初始化驱动**：  
   创建 `StepperMotor` 对象，传入电机地址、对应的 HardwareSerial 对象、校验方式和命令超时值。例如：
   ```cpp
   HardwareSerial mySerial(1);
   mySerial.begin(115200);
   StepperMotor motor(1, &mySerial, ChecksumType::FIXED, 1000);
   ```

3. **控制电机**：  
   调用控制接口对电机进行启停、运动模式设置和同步控制。

4. **参数读取／修改**：  
   使用相应接口读取当前电机参数或修改配置参数（可指定是否存储到芯片中）。

5. **多机通讯及同步控制**：  
   分别下发各电机的参数和运动指令（设置同步标志），最后调用 `syncMove()` 命令使所有电机同时运动。

## 6. 注意事项

- **超时设置**：  
  每条命令在发送后都有超时等待（单位：毫秒），请根据实际情况调整。

- **校验方式选择**：  
  默认采用固定校验字节 `0x6B`，但也可通过构造函数选择 XOR 或 CRC8 校验。

- **存储标志**：  
  参数修改命令带有"存储标志"，决定参数是否写入非易失性存储，断电后是否保持修改状态。

- **同步控制**：  
  当多个电机需要同时运动时，应先下发设置同步标志的命令，最后调用 `syncMove()` 触发同步动作。

---

请参考 `src/main.cpp` 中的示例程序以了解具体使用方式。
