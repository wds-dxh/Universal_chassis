#include "CarController/CarController.h"
#include <cmath>
#include <array>

// 构造函数：保存传入对象指针，并初始化默认参数
CarController::CarController(StepperMotor* motorRF, StepperMotor* motorRR,
                             StepperMotor* motorLR, StepperMotor* motorLF,
                             StepperMotor* motor0, KinematicsModel* kinematicsModel)
    : motorRF(motorRF), motorRR(motorRR), motorLR(motorLR), motorLF(motorLF),
      motor0(motor0), kinematics(kinematicsModel)
{
    // 初始化默认控制参数
    defaultConfig.defaultAcceleration = 10.0f;
    defaultConfig.defaultSubdivision = 256;
    defaultConfig.defaultSpeed = 1.0f;

    // 初始化状态
    currentState.vx = 0;
    currentState.vy = 0;
    currentState.omega = 0;
    currentState.wheelSpeeds[0] = 0;
    currentState.wheelSpeeds[1] = 0;
    currentState.wheelSpeeds[2] = 0;
    currentState.wheelSpeeds[3] = 0;
 
    //使能所有的电机
    motorRF->enableMotor(true, false);
    motorRR->enableMotor(true, false);
    motorLR->enableMotor(true, false);
    motorLF->enableMotor(true, false);
}

// 设置默认控制参数（加速度、细分数）配置接口
void CarController::configure(const CarControllerConfig& config) {
    defaultConfig = config;
}

// 速度模式控制（使用默认加速度）
bool CarController::setSpeed(float vx, float vy, float omega) {
    return setSpeed(vx, vy, omega, defaultConfig.defaultAcceleration, defaultConfig.defaultSubdivision);       //加速度调用默认加速度
}

// 速度模式控制（自定义加速度）  
// 本函数通过运动学模型计算各电机的转速指令，并将负值转为方向信息传递给电机控制
bool CarController::setSpeed(float vx, float vy, float omega, float acceleration, uint16_t subdivision) {
    // 计算速度指令
    // 注意：这里假设运动学模型的 calculateSpeedCommands 输出类型已修改为 std::array<int16_t, 4>
    std::array<int16_t, 4> speedCommands;
    kinematics->calculateSpeedCommands(vx, vy, omega, reinterpret_cast<std::array<uint16_t, 4>&>(speedCommands));
    
    bool success = true;
    
    // 对每个电机，分解速度正负得到方向和速度幅值
    // 轮序：0-右前轮, 1-右后轮, 2-左后轮, 3-左前轮
    int16_t cmd = speedCommands[0];
    uint8_t dirRF = (cmd >= 0) ? 1 : 0;
    uint16_t rpmRF = static_cast<uint16_t>(std::abs(cmd));
    if (!motorRF->setSpeedMode(dirRF, rpmRF, static_cast<uint8_t>(acceleration), false))
        success = false;
    
    cmd = speedCommands[1];
    uint8_t dirRR = (cmd >= 0) ? 1 : 0;
    uint16_t rpmRR = static_cast<uint16_t>(std::abs(cmd));
    if (!motorRR->setSpeedMode(dirRR, rpmRR, static_cast<uint8_t>(acceleration), false))
        success = false;
    
    cmd = speedCommands[2];
    uint8_t dirLR = (cmd >= 0) ? 1 : 0;
    uint16_t rpmLR = static_cast<uint16_t>(std::abs(cmd));
    if (!motorLR->setSpeedMode(dirLR, rpmLR, static_cast<uint8_t>(acceleration), false))
        success = false;
    
    cmd = speedCommands[3];
    uint8_t dirLF = (cmd >= 0) ? 1 : 0;
    uint16_t rpmLF = static_cast<uint16_t>(std::abs(cmd));
    if (!motorLF->setSpeedMode(dirLF, rpmLF, static_cast<uint8_t>(acceleration), false))
        success = false;
    
    // 触发多机同步运动
    if (!motorRF->syncMove())
        success = false;
    
    return success;
}

// 位置模式控制（使用默认控制参数）
bool CarController::moveDistance(float dx, float dy, float dtheta) {
    return moveDistance(dx, dy, dtheta, defaultConfig.defaultAcceleration, defaultConfig.defaultSpeed, defaultConfig.defaultSubdivision);
}

// 位置模式控制（完整参数版本）  
bool CarController::moveDistance(float dx, float dy, float dtheta, float acceleration, float speed, uint16_t subdivision) {
    std::array<int32_t, 4> pulseCommands;
    kinematics->calculatePositionCommands(dx, dy, dtheta, pulseCommands, subdivision);
    
    // 计算合适的速度
    std::array<int16_t, 4> speedCommands;
    // 使用dx的方向计算速度
    float vx = (dx != 0) ? (dx > 0 ? speed : -speed) : 0;
    float vy = (dy != 0) ? (dy > 0 ? speed : -speed) : 0;
    float omega = (dtheta != 0) ? (dtheta > 0 ? 0.5 : -0.5) : 0;
    
    kinematics->calculateSpeedCommands(vx, vy, omega, reinterpret_cast<std::array<uint16_t, 4>&>(speedCommands));
    
    // 找出最大速度值作为基准
    uint16_t maxSpeed = 0;
    for (auto cmd : speedCommands) {
        uint16_t absSpeed = std::abs(cmd);
        if (absSpeed > maxSpeed) {
            maxSpeed = absSpeed;
        }
    }
    
    // 如果计算出的速度为0，使用默认速度
    uint16_t speedRpm = (maxSpeed > 0) ? maxSpeed : 100;
    
    bool success = true;    
    
    int32_t pulses = pulseCommands[0];
    uint8_t dirRF = (pulses >= 0) ? 1 : 0;  // 正方向为1，负方向为0
    uint32_t absPulsesRF = static_cast<uint32_t>(std::abs(pulses));
    if (!motorRF->setPositionMode(dirRF, speedRpm, static_cast<uint8_t>(acceleration), absPulsesRF, false, false))
        success = false;
    
    pulses = pulseCommands[1];
    uint8_t dirRR = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesRR = static_cast<uint32_t>(std::abs(pulses));
    if (!motorRR->setPositionMode(dirRR, speedRpm, static_cast<uint8_t>(acceleration), absPulsesRR, false, false))
        success = false;
    
    pulses = pulseCommands[2];
    uint8_t dirLR = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesLR = static_cast<uint32_t>(std::abs(pulses));
    if (!motorLR->setPositionMode(dirLR, speedRpm, static_cast<uint8_t>(acceleration), absPulsesLR, false, false))
        success = false;
    
    pulses = pulseCommands[3];
    uint8_t dirLF = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesLF = static_cast<uint32_t>(std::abs(pulses));
    if (!motorLF->setPositionMode(dirLF, speedRpm, static_cast<uint8_t>(acceleration), absPulsesLF, false, false))
        success = false;
    
    // 触发多机同步运动
    if (!motorRF->syncMove())
        success = false;
    
    return success;
}

// 紧急停止所有电机
bool CarController::stop() {
    bool success = true;
    //停止主控板步进电机
    if (!motor0->stopMotor(false))
        success = false;

    return success;
}

// 获取当前小车状态
// 读取各个步进电机的反馈转速，并填充到 currentState.wheelSpeeds 中；其他速度信息此处暂设为0
CarState CarController::getCarState() {
    std::array<int16_t, 4> speeds;
    int16_t sRF, sRR, sLR, sLF;
    if (!motorRF->readRealTimeSpeed(sRF))
        sRF = 0;
    if (!motorRR->readRealTimeSpeed(sRR))
        sRR = 0;
    if (!motorLR->readRealTimeSpeed(sLR))
        sLR = 0;
    if (!motorLF->readRealTimeSpeed(sLF))
        sLF = 0;
    speeds[0] = static_cast<uint16_t>(sRF);
    speeds[1] = static_cast<uint16_t>(sRR);
    speeds[2] = static_cast<uint16_t>(sLR);
    speeds[3] = static_cast<uint16_t>(sLF);
    // 如果有符号整数的值是负数，那么转换为无符号类型后，它的值会变成一个大的正数
    kinematics->calculateWheelSpeeds(reinterpret_cast<std::array<int16_t, 4>&>(speeds),currentState.vx,currentState.vy,currentState.omega);
    
    currentState.wheelSpeeds = speeds;
    return currentState;
}
