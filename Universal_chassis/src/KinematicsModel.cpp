#include "KinematicsModel/KinematicsModel.h"
#include <cmath>
#include <array>
#include <Arduino.h>

//======================= NormalWheelKinematics 实现 =======================

NormalWheelKinematics::NormalWheelKinematics(float wheelRadius, float trackWidth,float reductionRatio)
    : wheelRadius(wheelRadius), trackWidth(trackWidth),reductionRatio(reductionRatio)
{
    // 计算轮子周长
    wheelCircumference = 2.0f * static_cast<float>(M_PI) * wheelRadius;
}

void NormalWheelKinematics::calculateSpeedCommands(float vx, float vy, float omega, std::array<uint16_t, 4>& speeds) {
    // 计算右侧和左侧轮子对应的旋转速度（单位：RPM）
    // 注意：右侧轮前进时为顺时针旋转(CW)，左侧轮前进时为逆时针旋转(CCW)
    float rightRpm = (vx + (trackWidth / 2.0f) * omega) * 60.0f / wheelCircumference;
    float leftRpm  = (vx - (trackWidth / 2.0f) * omega) * 60.0f / wheelCircumference;
    
    // 右侧轮（前进为CW，对应负RPM）
    speeds[0] = static_cast<uint16_t>(-rightRpm*reductionRatio); // 右前轮
    speeds[1] = static_cast<uint16_t>(-rightRpm*reductionRatio); // 右后轮
    // 左侧轮（前进为CCW，对应正RPM）
    speeds[2] = static_cast<uint16_t>(leftRpm*reductionRatio);   // 左后轮
    speeds[3] = static_cast<uint16_t>(leftRpm*reductionRatio);   // 左前轮
}

void NormalWheelKinematics::calculatePositionCommands(float dx, float dy, float dtheta, 
                                                     std::array<int32_t, 4>& pulses,
                                                     uint16_t subdivision) {


    // 计算每圈对应的脉冲数
    float pulsesPerRotation = 200.0f * subdivision; // 200 * 细分数 = 每圈脉冲数

    // 计算纯前进的脉冲数
    float pulses_forward = (dx / wheelCircumference) * pulsesPerRotation;

    // 计算旋转部分的脉冲数
    float pulses_rotation = ((trackWidth / 2.0f) * dtheta) / wheelCircumference * pulsesPerRotation;


    // 右侧轮（前进为CW，对应负脉冲）
    pulses[0] = static_cast<int32_t>(-std::lround(pulses_forward + pulses_rotation)*reductionRatio); // 右前轮
    pulses[1] = static_cast<int32_t>(-std::lround(pulses_forward + pulses_rotation)*reductionRatio); // 右后轮
    // 左侧轮（前进为CCW，对应正脉冲）
    pulses[2] = static_cast<int32_t>(std::lround(pulses_forward - pulses_rotation)*reductionRatio);  // 左后轮
    pulses[3] = static_cast<int32_t>(std::lround(pulses_forward - pulses_rotation)*reductionRatio);  // 左前轮
}

void NormalWheelKinematics::calculateWheelSpeeds(std::array<int16_t, 4>& speeds,
                             float& vx, float& vy, float& omega) {
                                

    float wheelSpeed1 = speeds[0] * wheelCircumference / 60.0f / reductionRatio;
    float wheelSpeed2 = speeds[1] * wheelCircumference / 60.0f / reductionRatio;
    float wheelSpeed3 = speeds[2] * wheelCircumference / 60.0f / reductionRatio;
    float wheelSpeed4 = speeds[3] * wheelCircumference / 60.0f / reductionRatio;
    Serial.println("wheelSpeed1: ");
    Serial.println(wheelSpeed1);
    Serial.println("wheelSpeed2: ");
    Serial.println(wheelSpeed2);
    Serial.println("wheelSpeed3: ");
    Serial.println(wheelSpeed3);
    Serial.println("wheelSpeed4: ");
    Serial.println(wheelSpeed4);

    //由轮子的转速计算轮子的线速度--0123分别对应右上，右下，左下，左上.注意轮速度
    vx = (wheelSpeed1 + wheelSpeed2) / 2.0f;
    vy = 0;
    omega = (wheelSpeed1 + wheelSpeed3) / (2.0f * trackWidth);
}

//======================= MecanumKinematics 空实现 ========================

MecanumKinematics::MecanumKinematics(float wheelRadius, float wheelBase, float trackWidth)
{
    // 暂未实现
}

void MecanumKinematics::calculateSpeedCommands(float vx, float vy, float omega, std::array<uint16_t, 4>& speeds) {
    // 暂未实现，全部返回0
    speeds.fill(0);
}

void MecanumKinematics::calculatePositionCommands(float dx, float dy, float dtheta, std::array<int32_t, 4>& pulses, uint16_t subdivision) {
    // 暂未实现，全部返回0
    pulses.fill(0);
}

//================= OmnidirectionalKinematics 空实现 ====================

OmnidirectionalKinematics::OmnidirectionalKinematics(float wheelRadius, float wheelBase, float trackWidth)
{
    // 暂未实现
}

void OmnidirectionalKinematics::calculateSpeedCommands(float vx, float vy, float omega, std::array<uint16_t, 4>& speeds) {
    // 暂未实现，全部返回0
    speeds.fill(0);
}

void OmnidirectionalKinematics::calculatePositionCommands(float dx, float dy, float dtheta, std::array<int32_t, 4>& pulses, uint16_t subdivision) {
    // 暂未实现，全部返回0
    pulses.fill(0);
}
