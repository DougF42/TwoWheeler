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
#include "Device.h"

#define MAX_MOTOR_COUNT 2
class Driver:public Device
{
private:
    MotorControl *motors[MAX_MOTOR_COUNT];
    int nextMotorIdx = 0;
    
    // COMMAND SET: 
    //   QUAD Calibrate Quadrature encoders  <pulsesPerRev>, <circum>
    ProcessStatus cmdQUAD();

    //   Calibrate PID  stepTime, <Kp>,<Ki>,<Kd>
    ProcessStatus cmdPID();

    //   SROTsetRotRate   <pidTime> <rate>
    ProcessStatus cmdSROT();

    //   FWD   <speed>   // set forward speed
    ProcessStatus cmdFWD();

    //   STOP    Stop.
    ProcessStatus cmdSTOP(); // Stop - setting stop rate.

public:
    Driver();
    ~Driver();
    bool addNewMotor(const MotorControl_config_t &configuration);
    void loop(); // call this reasonably frequently
    ProcessStatus  ExecuteCommand () override;  // Override this method to handle custom commands

    void setQuadParams(uint32_t stepTime, float kp, float ki, float kd);
    void setPidParams(uint32_t stepTime, uint pulsesPerRev, uint circumfrence, QuadDecoder::QuadUnits_t _units=QuadDecoder::UNITS_MM);

};