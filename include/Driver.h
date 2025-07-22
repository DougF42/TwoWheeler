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
#include "DefDevice.h"

#define MAX_MOTOR_COUNT 2
class Driver:public DefDevice
{
private:
    int nextMotorIdx;
    int mySpeed;
    int myDirect;
    Node *myNode;
        
    MotorControl  *leftMtr;
    MotorControl  *rightMtr;
    
    // COMMAND SET: 
    ProcessStatus cmdMOV(int argcnt, char *argv[]);   // FWD  <speed> <dir> (if no dir, then straight ahead)
    ProcessStatus cmdSTOP(int argcnt, char *argv[]);  // Stop - setting stop rate.
    ProcessStatus cmdSPEED(int argcnt, char *argv[]); // Set speed (used by joystick)
    ProcessStatus cmdROTATION(int argcnt, char *argv[]);  // Set rotation rate (used by joystick)
    ProcessStatus cmdDrift(int argcnt, char *argv[]);   // disable drivers


public:
    Driver(const char * name, Node *_Node);
    ~Driver();
    void setup(MotorControl_config_t *left_cfg, MotorControl_config_t *right_cfg); // Instantiate all the subtasks...
    ProcessStatus  ExecuteCommand () override;  // Override this method to handle custom commands
    ProcessStatus  DoPeriodic() override;       

    void setMotion(int speed, int _rotation);
};