/**
 * @file DEV_Driver.h
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

#include "DEV_Pid.h"

#define MAX_MOTOR_COUNT 2
class DEV_Driver:public Device
{
private:
    int nextMotorIdx;
    int mySpeed;
    int myDirect;
        
    DEV_Pid *leftPid;
    DEV_Pid *rightPid;
    
    // COMMAND SET: 
    ProcessStatus cmdMOV     (char *command, char *param);   // FWD  <speed> <dir> (if no dir, then straight ahead)
    ProcessStatus cmdSTOP    (char *command, char *param);  // Stop - setting stop rate.
    ProcessStatus cmdSPEED   (char *command, char *param); // Set speed (used by joystick)
    ProcessStatus cmdROTATION(char *command, char *param);  // Set rotation rate (used by joystick)
    ProcessStatus cmdDrift   (char *command, char *param);   // disable drivers
    ProcessStatus cmdTANK    (char *command, char *param);   // move like a tank

public:
    DEV_Driver(const char * name);
    ~DEV_Driver();
    void setup(DEV_Pid *_left, DEV_Pid *_right);
    ProcessStatus  ExecuteCommand (char *command, char *params=NULL); // Override this method to handle custom commands
    ProcessStatus  DoPeriodic() override;

    void setMotion(int speed, int _rotation);
};