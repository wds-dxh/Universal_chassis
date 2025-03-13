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
    defaultConfig.defaultSubdivision = 256*6; //256*6是减速比
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
bool CarController::setSpeed(float vx, float vy, float omega, uint32_t duration_ms) {
    return setSpeed(vx, vy, omega, duration_ms, defaultConfig.defaultAcceleration);       //加速度调用默认加速度
}

// 速度模式控制（自定义加速度）  
// 本函数通过运动学模型计算各电机的转速指令，并将负值转为方向信息传递给电机控制
bool CarController::setSpeed(float vx, float vy, float omega, uint32_t duration_ms, float acceleration) {
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
    
    
    
    // 运动时间结束后停止所有电机
    if (duration_ms != 0){  
    // 延时运动时间（使用 FreeRTOS 延时，避免阻塞）
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        stop();
    }
    
    // 单独启动一个任务，延时duration_ms后停止，防止在这里堵塞其他任务
    // 注意：需要在CarController类中添加成员变量 uint32_t stopDelayMs; 来存储延时时间

    // 创建任务
    // stopDelayMs = duration_ms;
    // if (stopTaskHandle == nullptr) {
    // xTaskCreate(
    //     [](void* param) {  // 使用无捕获列表的lambda表达式
    //         CarController* controller = static_cast<CarController*>(param);
    //         vTaskDelay(pdMS_TO_TICKS(controller->stopDelayMs));  // 从类成员获取延时时间
    //         controller->stop();
    //         vTaskDelete(nullptr);  // 删除当前任务
    //     },  // Ensure the lambda is cast to TaskFunction_t
    //     "stopTask",
    //     4096,
    //     this,
    //     1,
    //     &stopTaskHandle
    // );
    // }
    // else {
    //     vTaskDelete(stopTaskHandle);
    //     this->stop();
    //     //删除任务后创建任务
    //     xTaskCreate(
    //         [](void* param) {
    //             CarController* controller = static_cast<CarController*>(param);
    //             vTaskDelay(pdMS_TO_TICKS(controller->stopDelayMs));
    //             controller->stop();
    //         },
    //         "stopTask",
    //         4096,
    //         this,
    //         1,
    //         &stopTaskHandle
    //     );
    // }
    return success;
}

// 位置模式控制（使用默认控制参数）
bool CarController::moveDistance(float dx, float dy, float dtheta) {//打印配置参数
    return moveDistance(dx, dy, dtheta, defaultConfig.defaultAcceleration, defaultConfig.defaultSubdivision);
    
}

// 位置模式控制（自定义加速度和细分数）  
// 通过运动学模型计算各电机需要旋转的脉冲数，然后调用位置模式接口控制电机运动
bool CarController::moveDistance(float dx, float dy, float dtheta, float acceleration, uint16_t subdivision) {
    std::array<int32_t, 4> pulseCommands;
    kinematics->calculatePositionCommands(dx, dy, dtheta, pulseCommands, subdivision);
    
    bool success = true;    
    // 采用一个固定的速度参数（例如 100 RPM）用于位置模式下的运动
    uint16_t defaultSpeedRpm = 100;
    
    int32_t pulses = pulseCommands[0];
    uint8_t dirRF = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesRF = static_cast<uint32_t>(std::abs(pulses));
    if (!motorRF->setPositionMode(dirRF, defaultSpeedRpm, static_cast<uint8_t>(acceleration), absPulsesRF, false, false))
        success = false;
    
    pulses = pulseCommands[1];
    uint8_t dirRR = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesRR = static_cast<uint32_t>(std::abs(pulses));
    if (!motorRR->setPositionMode(dirRR, defaultSpeedRpm, static_cast<uint8_t>(acceleration), absPulsesRR, false, false))
        success = false;
    
    pulses = pulseCommands[2];
    uint8_t dirLR = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesLR = static_cast<uint32_t>(std::abs(pulses));
    if (!motorLR->setPositionMode(dirLR, defaultSpeedRpm, static_cast<uint8_t>(acceleration), absPulsesLR, false, false))
        success = false;
    
    pulses = pulseCommands[3];
    uint8_t dirLF = (pulses >= 0) ? 1 : 0;
    uint32_t absPulsesLF = static_cast<uint32_t>(std::abs(pulses));
    if (!motorLF->setPositionMode(dirLF, defaultSpeedRpm, static_cast<uint8_t>(acceleration), absPulsesLF, false, false))
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
    
    currentState.wheelSpeeds = speeds;
    // 这里未做逆运动学计算，故 vx,vy,omega 暂置 0
    currentState.vx = 0;
    currentState.vy = 0;
    currentState.omega = 0;
    
    return currentState;
}
