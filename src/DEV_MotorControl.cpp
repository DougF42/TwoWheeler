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

#include "DEV_MotorControl.h"

DEV_MotorControl::DEV_MotorControl(const char * InName, Node *_nodePtr) : DefDevice(InName)
{
    piddev = nullptr;
    myNode = _nodePtr;
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
void DEV_MotorControl::setup( MotorControl_config_t *cfg, const char *prefix)
{
    char name[20];  // for building the actual device name
    
    // Create and add the quadrature decoder driver
    strcpy(name, prefix);
    strcpy(name+strlen(name), "QUAD");
    // quadDecoder    = new QuadDecoder(myNode, name);
    // quadDecoder->setupQuad(cfg);
    myQuadDecoder = new DEV_QuadDecoder(name);
    myQuadDecoder->setup(cfg);
    myNode->AddDevice(myQuadDecoder);

    // Create and add the ln298 driver
    strcpy(name, prefix);
    strcpy(name+strlen(name), "LN298");
    ln298   = new DEV_LN298(name);
    ln298->setupLN298(cfg);
    myNode->AddDevice(ln298);

    // Create and add the pid controler
    // Set up new PID. THIS IS NOT (currently) A DEVICE!
    //  Input, Output Setpoint, Kp, Ki, Kd, P_ON_E Flag,   controlerDirection
    sprintf(name, "%s%s", prefix,"PID");
    piddev = new DEV_Pid(name, cfg, myQuadDecoder, ln298);
    myNode->AddDevice(piddev);
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
ProcessStatus DEV_MotorControl::ExecuteCommand()
{
    ProcessStatus retVal;
    DataPacket.timestamp = millis();
    retVal = Device::ExecuteCommand();
    if (retVal == NOT_HANDLED)
    {
        scanParam();
        if (isCommand("MSPD"))
        { // Set motor speed
            retVal = cmdSetSpeed(argCount, arglist);
        }

        else
        {
            sprintf(DataPacket.value, "EROR|DEV_MotorControl|Unknown command");
            retVal = FAIL_DATA;
        }
    }
    return (retVal);
}

/**
 * @brief Set the overall ground speed of the robot
 */
 ProcessStatus DEV_MotorControl::cmdSetSpeed(int argCnt, char **argv)
 {
    // TODO:
    return(NOT_HANDLED);
 }



/**
 * @brief loop - call periodically to send status info
 * 
 */
ProcessStatus DEV_MotorControl::DoPeriodic()
{
    static double last_output_val = 0;

    // get current speed (from quad)
    // TODO: input_val = getSpeed();

#ifdef USE_PID
    pidctlr->Compute(); // determine change to power setting
#else
    // map input directly to ouput
    output_val = defmap<double>(input_val, -2048, 2048, -100, 100);
#endif

    if ( ISNOTEQUAL(last_output_val, output_val) )
    { // IF there was a noticable change, then update the ln298 pulse rate
        ln298->setPulseWidth(output_val);
    }

    last_output_val = output_val;
    return(SUCCESS_NODATA);
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
 *    The motor has  motors engaged, but stopped.
 * (Later, we will ramp the speed down at the 'stopRate')
 *  
 * @param stopRate  - TBD:
 */
void DEV_MotorControl::setStop(int stopRate)
{
    setSpeed(0);  // for now, just stop...
}