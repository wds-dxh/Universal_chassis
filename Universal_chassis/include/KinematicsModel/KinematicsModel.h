/*
 * @Author: wds2dxh wdsnpshy@163.com
 * @Date: 2025-02-23 15:18:56
 * @Description: 运动学模型
 * Copyright (c) 2025 by ${wds2dxh}, All Rights Reserved. 
 */

#pragma once

#include <cstdint>
#include <array>
#include <cmath> // 为了计算周长等

/**
 * @brief 抽象基类：运动学模型
 *
 * 用于根据小车运动命令（速度或位移）计算各个电机的控制参数。
 */
class KinematicsModel {
public:
    virtual ~KinematicsModel() {}

    /**
     * @brief 根据期望速度计算各个电机的速度指令
     * 
     * @param vx 小车 X 方向线速度 (m/s)，正值表示前进，负值表示后退
     * @param vy 小车 Y 方向线速度 (m/s)
     * @param omega 旋转角速度 (rad/s)，正值表示逆时针旋转
     * @param speeds 输出电机速度指令（顺序：轮1~轮4），正负号表示方向
     */
    virtual void calculateSpeedCommands(float vx, float vy, float omega,
                                      std::array<uint16_t, 4>& speeds) = 0;

    /**
     * @brief 根据期望位移计算各个电机的脉冲数
     * 
     * @param dx X方向位移 (m)，正值表示前进，负值表示后退
     * @param dy Y方向位移 (m)
     * @param dtheta 旋转角度 (rad)，正值表示逆时针旋转
     * @param pulses 输出各电机目标脉冲数（顺序：轮1~轮4），正负号表示方向
     * @param subdivision 细分数，用于计算每圈脉冲数
     */
    virtual void calculatePositionCommands(float dx, float dy, float dtheta,
                                         std::array<int32_t, 4>& pulses,
                                         uint16_t subdivision) = 0;
};

/**
 * @brief 麦克纳姆轮运动学模型
 *
 * 实现基于麦克纳姆轮的运动学公式，用于计算电机的速度和位移指令。
 */
class MecanumKinematics : public KinematicsModel {
public:
    /**
     * @brief 构造函数
     * @param wheelRadius 轮子半径 (m)
     * @param wheelBase 前后轮距离 (m)
     * @param trackWidth 左右轮距 (m)
     */
    MecanumKinematics(float wheelRadius, float wheelBase, float trackWidth);

    virtual void calculateSpeedCommands(float vx, float vy, float omega,
                                      std::array<uint16_t, 4>& speeds) override;

    virtual void calculatePositionCommands(float dx, float dy, float dtheta,
                                         std::array<int32_t, 4>& pulses,
                                         uint16_t subdivision) override;
private:
    float wheelRadius;
    float wheelBase;
    float trackWidth;
    // 其他运动学参数，例如每转脉冲数，可在实现中设置
};

/**
 * @brief 全向轮运动学模型
 *
 * 针对全向轮组配置设计的运动学模型。
 */
class OmnidirectionalKinematics : public KinematicsModel {
public:
    /**
     * @brief 构造函数
     * @param wheelRadius 轮子半径 (m)
     * @param wheelBase 前后轮距离 (m)
     * @param trackWidth 左右轮距 (m)
     */
    OmnidirectionalKinematics(float wheelRadius, float wheelBase, float trackWidth);

    virtual void calculateSpeedCommands(float vx, float vy, float omega,
                                      std::array<uint16_t, 4>& speeds) override;

    virtual void calculatePositionCommands(float dx, float dy, float dtheta,
                                         std::array<int32_t, 4>& pulses,
                                         uint16_t subdivision) override;
private:
    float wheelRadius;
    float wheelBase;
    float trackWidth;
};

/**
 * @brief 普通轮运动学模型
 *
 * 针对普通四轮配置设计的运动学模型，适用于直线或简单转弯控制。
 * 轮子编号定义：
 *  - 轮子1：右前轮
 *  - 轮子2：右后轮
 *  - 轮子3：左后轮
 *  - 轮子4：左前轮
 */
class NormalWheelKinematics : public KinematicsModel {
public:
    /**
     * @brief 构造函数
     * @param wheelRadius 轮子半径 (m)
     * @param trackWidth 左右轮距 (m)
     * @param reductionRatio 减速比
     */
    NormalWheelKinematics(float wheelRadius, float trackWidth,float reductionRatio);

    virtual void calculateSpeedCommands(float vx, float vy, float omega,
                                      std::array<uint16_t, 4>& speeds) override;

    virtual void calculatePositionCommands(float dx, float dy, float dtheta,
                                         std::array<int32_t, 4>& pulses,
                                         uint16_t subdivision) override;
private:   //-------硬件参数都是用运动学模型输入，软件参数如细分数，加速度，速度等都是用CarController输入
    float wheelRadius;       // 轮子半径
    float wheelCircumference;  // 内部计算得出：2 * PI * wheelRadius
    float trackWidth;         // 左右轮距
    float reductionRatio;     // 减速比
};
