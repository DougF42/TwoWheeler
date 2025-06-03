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
 * rate-of-turn input is in +/- Radians per millisecond, with 0 being straight forward. 
 *   The rate-of-turn is limited to +/- PI/2 (i.e.: +/- 90 degrees) radians per millisecond.
 * 
 * The speed is in mm per millisecond. 
 */
#pragma once

#include "MotorControl.h"
#include "PID_v1.h"
#include "Device.h"

#define MAX_MOTOR_COUNT 2
class Driver:public Device
{
private:
    MotorControl *motors[MAX_MOTOR_COUNT];  // we need two motors
    int nextMotorIdx = 0;
    int mySpeed;
    int myDirect;
    // COMMAND SET: 
    ProcessStatus cmdQUAD();   //   QUAD Calibrate Quadrature encoders  <pulsesPerRev>, <circum>
    ProcessStatus cmdPID();    //   Calibrate PID  stepTime, <Kp>,<Ki>,<Kd>
    ProcessStatus cmdMOV();   //   FWD  <speed> <dir> (if no dir, then straight ahead)
    ProcessStatus cmdSTOP(); // Stop - setting stop rate.

public:
    Driver();
    ~Driver();
    bool addNewMotor(const MotorControl_config_t &configuration);
    void loop(); // call this reasonably frequently
    ProcessStatus  ExecuteCommand () override;  // Override this method to handle custom commands

    void setQuadParams(time_t stepTime, double kp, double ki, double kd);
    void setPidParams(time_t stepTime, pulse_t pulsesPerRev, dist_t circumfrence);
};