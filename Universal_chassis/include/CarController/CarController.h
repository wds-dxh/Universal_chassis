/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-23 15:18:43
 * @Description: 小车控制类头文件
 * Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
 */

#pragma once

#include "StepperMotor/StepperMotor.h"
#include "KinematicsModel/KinematicsModel.h"
#include <cstdint>
#include <array>

/**
 * @brief 小车状态结构体
 *
 * 用于存储小车当前运动状态与反馈信息
 */
struct CarState {
    float vx;      // 线速度 X (m/s)
    float vy;      // 线速度 Y (m/s)
    float omega;   // 角速度 (rad/s)
    std::array<int16_t, 4> wheelSpeeds;   // 四个轮子的当前速度反馈信息
};

/**
 * @brief 小车控制配置结构体
 *
 * 用于一次性设置默认的加速度、细分数等参数
 */
struct CarControllerConfig {
    float defaultAcceleration;  ///< 默认加速度（单位由具体实现决定），例如 10.0
    float defaultSubdivision;   ///< 默认细分数，例如 16（16细分下3200脉冲为一圈）
    float defaultSpeed;         ///< 默认速度，例如 1.0
};

/**
 * @brief 小车控制类
 *
 * 本类负责接收用户的运动指令，调度底层步进电机控制（已封装于 StepperMotor 类），
 * 并利用运动学模型（KinematicsModel）计算各个电机的控制参数。
 * 
 * 轮子编号定义：
 *  - 轮子1：右前轮
 *  - 轮子2：右后轮
 *  - 轮子3：左后轮
 *  - 轮子4：左前轮
 */
class CarController {
public:
    /**
     * @brief 构造函数
     * @param motorRF 右前轮步进电机驱动对象
     * @param motorRR 右后轮步进电机驱动对象
     * @param motorLR 左后轮步进电机驱动对象
     * @param motorLF 左前轮步进电机驱动对象
     * @param motor0 主控板步进电机驱动对象--用于控制所有电机
     * @param kinematicsModel 指向运动学模型对象（多态实现：支持不同轮型）
     */
    CarController(StepperMotor* motorRF, StepperMotor* motorRR,
                  StepperMotor* motorLR, StepperMotor* motorLF, StepperMotor* motor0,
                  KinematicsModel* kinematicsModel);


    /**
     * @brief 通过速度模式控制小车运动
     *
     * 根据给定的期望车速（vx, vy, omega）和加速度计算各个电机的速度，
     * 并调用步进电机的速度模式接口进行控制，同时设置运动持续时间。
     *
     * @param vx X方向线速度 (m/s)
     * @param vy Y方向线速度 (m/s)
     * @param omega 旋转角速度 (rad/s)
     * @param duration_ms 运动持续时间 (毫秒)
     * @param acceleration 加速度 (默认值 10.0)
     * @return true 成功下发命令至所有电机
     * @return false 至少一个电机命令下发失败
     */
    bool setSpeed(float vx, float vy, float omega, uint32_t duration_ms);

    /**
     * @brief 通过位置模式控制小车运动
     *
     * 根据期望位移（dx, dy）和旋转角度 (dtheta)、
     * 以及加速度和细分数参数计算各电机的目标脉冲数，
     * 并调用步进电机的位置模式接口进行控制。
     *
     * @param dx X方向位移 (m)
     * @param dy Y方向位移 (m)
     * @param dtheta 旋转角度 (rad)
     * @param acceleration 加速度 (默认值 10.0)
     * @param subdivision 细分数 (默认 16，即16细分下3200脉冲为一圈)
     * @return true 成功下发命令至所有电机
     * @return false 至少一个电机命令下发失败
     */
    bool moveDistance(float dx, float dy, float dtheta);

    /**
     * @brief 带加速度（及细分参数）的接口，用户可以通过此版本自定义
     *
     * @param vx X方向线速度 (m/s)
     * @param vy Y方向线速度 (m/s)
     * @param omega 旋转角速度 (rad/s)
     * @param duration_ms 运动持续时间 (毫秒)
     * @param acceleration 加速度
     * @param subdivision 细分数
     * @return true 成功下发命令至所有电机
     * @return false 至少一个电机命令下发失败
     */
    bool setSpeed(float vx, float vy, float omega, uint32_t duration_ms, float acceleration);

    /**
     * @brief 带加速度（及细分参数）的接口，用户可以通过此版本自定义
     *
     * @param dx X方向位移 (m)
     * @param dy Y方向位移 (m)
     * @param dtheta 旋转角度 (rad)
     * @param acceleration 加速度
     * @param subdivision 细分数
     * @return true 成功下发命令至所有电机
     * @return false 至少一个电机命令下发失败
     */
    bool moveDistance(float dx, float dy, float dtheta, float acceleration, uint16_t subdivision);  //uint16_t subdivision 表示细分数,8会溢出

    /**
     * @brief 紧急停止小车运动
     *
     * 调用所有步进电机的紧急停止接口，实现小车立即停止。
     *
     * @return true 所有电机停止成功
     * @return false 至少有一个电机停止失败
     */
    bool stop();

    /**
     * @brief 获取当前小车状态
     *
     * 在返回状态前内部会自动更新状态反馈信息
     * @return CarState 当前小车的状态结构体
     */
    CarState getCarState();

    /**
     * @brief 设置默认控制参数
     * @param config 配置结构体，包含默认加速度、默认细分数等
     */
    void configure(const CarControllerConfig& config);

private:
    StepperMotor* motorRF;   // 右前轮
    StepperMotor* motorRR;   // 右后轮
    StepperMotor* motorLR;   // 左后轮
    StepperMotor* motorLF;   // 左前轮
    StepperMotor* motor0;   // 主控板步进电机

    KinematicsModel* kinematics; // 运动学模型


    CarState currentState;

    //初始化CarControllerConfig, 默认加速度为10.0, 默认细分数为16
    CarControllerConfig defaultConfig;

};
