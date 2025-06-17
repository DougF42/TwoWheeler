/**
 * @file PidDevice.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-06-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "PidDevice.h"

PidDevice::PidDevice(Node *_node, const char *_name, MotorControl_config_t *cfg) : DefDevice(_node, _name)
{
    pid = nullptr;
    name = strdup(_name);
        //PID(double*, double*, double*,        // * constructor.  links the PID to the Input, Output, and 
        // double, double, double, int, int);   //   Setpoint.  Initial tuning parameters are also set here.
                                                //   (overload for specifying proportional mode)
    pid = new PID(&input, &output, &setPoint,  // links the PID to the Input, Output, and setpoint
        cfg->kp, cfg->ki, cfg->kd, P_ON_E, 0);    // Kp, Ki, Kd, POn, invertFlag
}


PidDevice::~PidDevice()
{
}


/**
 * @brief Set up the PID class.
 * 
 */
void PidDevice::setPID()
{
    //(1) Define the PID device
    // TODO:
    //    PID(double*, double*, double*,        // * constructor.  links the PID to the Input, Output, and
    // double, double, double, int, int);//   Setpoint.  Initial tuning parameters are also set here.
    //   (overload for specifying proportional mode)
    pid = new PID(&input, &output, &setPoint, kp, ki, kd, P_ON_E, 0);
}


/**
 * @brief periodically Run compute to update the PID.
 *
 * @param arg - pointer to this instance
 */
ProcessStatus PidDevice::DoPeriodic()
{
    pid->Compute();
    return(SUCCESS_NODATA);
}

/**
 * @brief Decode (and implement) SMAC commands
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::ExecuteCommand ()
{
    // TODO: 
    return(FAIL_NODATA); // for now...
}