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
    enableReportFlag=false;

    name = strdup(_name);
        //PID(double*, double*, double*,        // * constructor.  links the PID to the Input, Output, and 
        // double, double, double, int, int);   //   Setpoint.  Initial tuning parameters are also set here.
                                                //   (overload for specifying proportional mode)
    pid = new PID(&input, &output, &setPoint,  // links the PID to the Input, Output, and setpoint
        cfg->kp, cfg->ki, cfg->kd, P_ON_E, 0);    // Kp, Ki, Kd, POn, invertFlag
        
    periodicEnabled=false;
}


PidDevice::~PidDevice()
{
}


/**
 * @brief periodically - generate reports, 
 *    input, output, setValue
 * @param arg - pointer to this instance
 */
ProcessStatus PidDevice::DoPeriodic()
{
    // GENERATE REPORTS
    ProcessStatus retVal=SUCCESS_NODATA;
    if (! enableReportFlag) 
    {
        sprintf(DataPacket.value, "PID|%f|%f|%f" );
        retVal=SUCCESS_DATA;
    }
    return(retVal);
}


/**
 * @brief run the PID loop
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::DoImmediate()
{
    pid->Compute();
    return(SUCCESS_NODATA);
}


/**
 * @brief Decode (and implement) SMAC commands
 * 
 * FORMAT:  SPID <Kp> <Ki> <Kd>
 * FORMAT:  SMODE <bool>         (true-automatic, false-manual)
 * FORMAT:  STIM <time_ms>      (sample time rate - via DOIMMEDIATE)
 * FORMAT:  REPT <bool>          (should we generate a report?)
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::ExecuteCommand()
{
    ProcessStatus retVal = SUCCESS_DATA;
    retVal = Device::ExecuteCommand();
    if (retVal != NOT_HANDLED)
        return (retVal);
    retVal = NOT_HANDLED;
    if (isCommand("SPID"))
    {
        // TODO:
        retVal = cmdSetPid();
    }

    else if (isCommand("SMODE"))
    {
        retVal = cmdSetPid();
    }

    else if (isCommand("STIM"))
    {
        retVal = cmdSetSTime();
    }

    else if (isCommand("REPT"))
    {
        retVal = cmdSetRept();
    }

    return (retVal); // for now...
}

/**
 * @brief Get or Set the PID parameters
 * Format:   SPID|<kp>|<ki>|<kd>
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetPid()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    if (argCount == 3)
    {  // We have three parameters

    } else if (argCount !=0)
    { // Only other option is no parameters- 
        sprintf(DataPacket.value, "ERROR: Bad Argument count");
        retVal = FAIL_DATA;
    }

    if ( retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "Kp=%f  Ki=%f  Kd=%g", 
            pid->GetKp(), pid->GetKi(), pid->GetKd());
    }
    retVal=SUCCESS_DATA;
    return(retVal);
}


/**
 * @brief Command to set the mode 
 *    Mode can be TRUE (automatic - PID in use) OR
 *                FALSE (manual - PID NOT in use)
 *    FORMAT: SMODE|<bool>
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetMode()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    if (argCount == 1)
    {
        bool val;
        if (0 == getBool(1, &val, "Bad mode ") )
        {
            (val) ?pid->SetMode(1) : pid->SetMode(0);
        }
    }
    else
    {
        // Error - wrong arg count
        sprintf(DataPacket.value,"ERRO| Missing Argument or wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "SMOD|%s",  (pid->GetMode() ? "Enabled": "Disabled" ));
        retVal=SUCCESS_DATA;
    }

    defDevSendData(0, false);
    return(retVal);
}

/**
 * @brief  Command to Set the PID compute time (milliseconds)
 *    FORMAT: STIM|<time>
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetSTime()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    if (argCount == 1)
    {
        int32_t stime;
        if ( 0 == getInt32(1, &stime, "Bad mode "))
        {
            pid->SetSampleTime(stime);
        }  else {
            retVal=FAIL_DATA;
        }

    } else if (argCount != 0) 
    {
        // Error - wrong arg count
        sprintf(DataPacket.value,"ERRO| wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value,"STIM|%s",  (pid->GetMode() ? "Enabled": "Disabled" ));
        retVal=SUCCESS_DATA;
    }
    defDevSendData(0, false);
    return(retVal);
}


/**
 * @brief Command to enable/disable reports
 *  FORMAT: REPT <bool>
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetRept()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    if (argCount == 1)
    {
        bool enaRept;
        if ( 0 == getBool(1, &enaRept, "Set  Report Command: "))
        {
            enableReportFlag = enaRept;
        } 

    } else if (argCount != 0) 
    {
        // Error - wrong arg count
        sprintf(DataPacket.value, "ERRO| wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "STIM|%s",  (pid->GetMode() ? "Enabled": "Disabled" ));
        retVal = SUCCESS_DATA;
    }
    defDevSendData(0, false);
    return(retVal);
}

