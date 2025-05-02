/**
 * @file Driver.h
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-05-01
 * 
 * @copyright Copyright (c) 2025
 * 
 * This is the 'driver' for my two-wheel robot. 
 * 
 * It has entry point to allow it to define a new wheel
 * and add it to its in-house list.
 * 
 */
#pragma once

#include "MotorControl.h"
#include "Device.h"

#define MAX_MOTOR_COUNT 2
class Driver:public Device
{
private:
    MotorControl *motors[MAX_MOTOR_COUNT];
    int nextMotorIdx = 0;
    ProcessStatus QuadParameters();
    ProcessStatus PidParameters();
    ProcessStatus rate();
    ProcessStatus moveFwd();
    ProcessStatus rotate();
    ProcessStatus stop();

public:
    Driver();
    ~Driver();
    bool addNewMotor(const MotorControl_config_t &configuration);
    void loop(); // call this reasonably frequently
    ProcessStatus  ExecuteCommand () override;  // Override this method to handle custom commands
    
    // COMMAND SET: 
    //   Calibrate QUAD  <pulsesPerRev>, <circum>, <units>
    //   Calibrate PID   <Kp>,<Kd>,<Ki>
    //   CheckRate   <pidTime> <quadTime>
    // MOTION:
    //   MoveFwd(Target speed)
    //   Rotate(degrees to rotate)
    //   stop (int stopRate);

    void setCourse(float dir, float speed);
    void setDir(float direction);
    void setFwdSpeed(float speed);
    void stop(int stopRate); // 0..100  0 is drift, 100 is panic stop

};