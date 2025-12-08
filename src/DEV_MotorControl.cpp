/**
 * @file DEV_MotorControl.cpp
 * @author Doug Fajardo
 * @brief
 * @version 0.1
 * @date 2025-04-28
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "SMAC/Node.h"
#include "DEV_MotorControl.h"

DEV_MotorControl::DEV_MotorControl(const char * InName) : DefDevice(InName)
{
    piddev = nullptr;
 }


DEV_MotorControl::~DEV_MotorControl()
{
    return;
}


/**
 * @brief Define and initialize instances of the quad, ln298 and PID class
 * 
 * @param cfg       - pointer to the config structure
 * @param prefix    - A prefix for 'names' of the created devices
 */
void DEV_MotorControl::setup( MotorControl_config_t *cfg, DEV_QuadDecoder *_quad, DEV_LN298 * _ln298, DEV_Pid * _pid)
{
    char name[20];  // for building the actual device name
    myQuadDecoder = _quad;
    ln298         = _ln298;
    piddev        = _pid;

    periodicEnabled=false;
}

template <typename A>
A defmap(A x, A in_min, A in_max, A out_min, A out_max)
{
    A res = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return(res);
}

/**
 * @brief Handle Device commands
 *  MSPD <speed> - set motor speed  +/- 2048
 *  REPT <Y|N>   - enable periodic reports
 * @return ProcessStatus 
 */
ProcessStatus DEV_MotorControl::ExecuteCommand(char *command, char *params)
{
    ProcessStatus retVal;
    retVal = Device::ExecuteCommand(command, params);
    if (retVal == NOT_HANDLED)
    {
        scanParam(params);
        if (isCommand("MSPD"))
        { // Set motor speed
            retVal = cmdSetSpeed();
        }

        else
        {
            sprintf(SMACData.values, "EROR|DEV_MotorControl|Unknown command");
            retVal = FAIL_DATA;
        }
    }
    return (retVal);
}


/**
 * @brief handle the commmand to set speed.
   MSPD <speed> - set motor speed  +/- 2048
 * @return ProcessStatus 
 */
ProcessStatus DEV_MotorControl::cmdSetSpeed()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    double result;
    retVal = getDouble(1,&result, "Speed value - ");
    if (retVal == SUCCESS_NODATA)
    {
        setSpeed(result);
    }
    return(retVal);
}

/**
 * @brief Set the Speed.
 * 
 * @param ratemm_Sec - speed, mm per millisecond???
 */
void DEV_MotorControl::setSpeed(double rate_mm_mmsec)
{
    setpoint = rate_mm_mmsec;
}


/**
 * @brief Set the robot to drift.
 * 
 */
void DEV_MotorControl::setDrift()
{
    // TODO:
}


/**
 * @brief Stop the robot (Work in progress)
 * Format:   MSPD|<speed>
 *    The motor has  motors engaged, but st
 * opped.
 * (Later, we will ramp the speed down at the 'stopRate')
 *  
 * @param stopRate  - TBD:
 */
void DEV_MotorControl::setStop(int stopRate)
{
    setSpeed(0);  // for now, just stop...
}