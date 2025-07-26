/**
 * @file PidDevice.cpp
 * @author Doug Fajardo
 * @brief Use a PID loop to control the motor speed
 * @version 0.1
 * @date 2025-06-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * Commands accepted by this controller:
 * SPED|<val>              set (or get) the actual speed.
 * SPID|<p>|<i>|<d>        (set or get PID values)
 * SMODE|<AUTO|MAN..>      Auto (pid controls) or Manual(no pid) 
 * STIM|<time>             PID loop rate (milliseconds)
 *
 */
#include "PidDevice.h"

PidDevice::PidDevice( const char *_name, MotorControl_config_t *cfg, 
        QuadDecoder *_quad, LN298 *_ln298 ) : DefDevice( _name)
{
    pid = nullptr;
    ln298 = _ln298;
    quad  = _quad;

    name = strdup(_name);
        //PID(double*, double*, double*,        // * constructor.  links the PID to the actual, Output, and 
        // double, double, double, int, int);   //   Setpoint.  Initial tuning parameters are also set here.
                                                //   (overload for specifying proportional mode)
    pid = new DefPID(&actual, &output, &setPoint,  // links the PID to the actual, Output, and setpoint
        cfg->kp, cfg->ki, cfg->kd, P_ON_E, 0);    // Kp, Ki, Kd, POn, invertFlag
    pid->SetOutputLimits(0.0, 100.0);           //  We cant do any better than 100 % !!!!
    pid->SetTunings(DEFAULT_Kp, DEFAULT_Ki, DEFAULT_Kd);
    pid->SetSampleTime(PID_SAMPLE_TIME_ms);
    mySampleTime=PID_SAMPLE_TIME_ms;
    pid->SetMode(AUTOMATIC); // MANUAL ????
    periodicEnabled=false;
}


PidDevice::~PidDevice()
{
    return;
}


/**
 * @brief periodically -  report  setValue, actual, output
 * @param arg - pointer to this instance
 */
ProcessStatus PidDevice::DoPeriodic()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    DataPacket.timestamp = millis();    
    sprintf(DataPacket.value, "PID|%lf|%lf|%lf", setPoint, actual, output);
    retVal = SUCCESS_DATA;

    return (retVal);
}


/**
 * run the PID controller.
 *
 */
ProcessStatus PidDevice::DoImmediate()
{
    return(SUCCESS_NODATA); // FOR NOW, DISABLE
    if (pid->GetMode()==MANUAL) return(SUCCESS_NODATA);
    // TODO: LOAD requested, actual, cur PWD Setting
    actual = quad->getSpeed();
    // setpoint = (requested speed)
    // output =  (current PWM setting)

    if (pid->Compute())
    {   // IF the PID re-calculated, then update
        // the motor speed accordingly
        ln298->setPulseWidth((int)output);
    }

    return(SUCCESS_NODATA);
}


/**
 * @brief Decode (and implement) SMAC commands
 * 
 * FORMAT:  SETP <Kp>
 * FORMAT:  SETI <Ki>
 * FORMAT:  SETD <Kd>
 * FORMAT:  SMODE <bool>         (true-automatic, false-manual)
 * FORMAT:  STIM <time_ms>      (sample time rate - via DOIMMEDIATE)
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::ExecuteCommand()
{
    ProcessStatus retVal = SUCCESS_DATA;
    DataPacket.timestamp = millis();
    retVal = Device::ExecuteCommand();
    if (retVal != NOT_HANDLED)
        return (retVal);
    retVal = NOT_HANDLED;

    scanParam();
    if (isCommand("SPED"))
    {   // Set speed
        retVal = cmdSetSpeed();

    } else if (isCommand("SETP"))
    {   // Set p parameter        
        retVal = cmdSetP();

    } else if (isCommand("SETI"))
    {  // Set i parameter
        retVal = cmdSetI();

    } else if (isCommand("SETD"))
    {   // Set D
        retVal = cmdSetD();
    }

    else if (isCommand("SMODE"))
    {    // Set mode (auto or manual)
        retVal = cmdSetMode();
    }

    else if (isCommand("STIM"))
    {    // Set pid update time (millisecs)
        retVal = cmdSetSTime();
    }

    else 
    {
        sprintf(DataPacket.value, "EROR|PID|Unknown command");
        retVal = FAIL_DATA;
    }

    return (retVal); // for now...
}


/**
 * @brief: Set the desired speed
 *    FORMAT:  SPED|<speed>
 *        <speed in cm/sec ???>
 *    Note: This works wether we are
 * in MANUAL or AUTOMATIC modes
 */
ProcessStatus PidDevice::cmdSetSpeed()
{
    ProcessStatus retVal=SUCCESS_NODATA;

    if (argCount == 1)
    {
        retVal=getDouble(0, &setPoint, "Speed ");
    } else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERR|Wrong number of arguments in SPED command");
        retVal=FAIL_DATA;
    }

    if (retVal==SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "OK|%ld", setPoint);
        retVal = SUCCESS_DATA;
    }
    return(retVal);
}

/**
 * @brief set the desired motor speed
 */
void PidDevice::setSpeed(double speed)
{
    setPoint = speed;
    return;
}

/**
 * @brief Get or Set the PID parameters
 * Format:   SETP|<kp>
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetP()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    if (argCount == 1)
    {
        retVal=getDouble(0, &kp, "Kp ");
    } else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERR|Wrong number of arguments in SETP command");
        retVal=FAIL_DATA;
    }

    if (retVal==SUCCESS_NODATA)
    {
        if (argCount==1) pid->SetTunings(kp, ki, kd);
        sprintf(DataPacket.value, "OK|%ld", kp);
        retVal = SUCCESS_DATA;
    }
    return(retVal);
}


/**
 * @brief Get or Set the PID parameters
 * Format:   SETI|<ki>
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetI()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    if (argCount == 1)
    {
        retVal=getDouble(0, &ki, "Ki ");
    } else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERR|Wrong number of arguments in SETI command");
        retVal=FAIL_DATA;
    }

    if (retVal==SUCCESS_NODATA)
    {
        if (argCount==1) pid->SetTunings(kp, ki, kd);
        sprintf(DataPacket.value, "OK|%ld", ki);
        retVal = SUCCESS_DATA;
    }
    return(retVal);
}


/**
 * @brief Get or Set the PID parameters
 * Format:   SETD|<ki>
 * 
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::cmdSetD()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    if (argCount == 1)
    {
        retVal=getDouble(0, &kd, "Kd ");
    } else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERR|Wrong number of arguments in SETD command");
        retVal=FAIL_DATA;
    }

    if (retVal==SUCCESS_NODATA)
    {
        if (argCount==1) pid->SetTunings(kp, ki, kd);
        sprintf(DataPacket.value, "OK|%ld", kd);
        retVal = SUCCESS_DATA;
    }
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
    bool val=false;
    if (argCount == 1)
    {
 
        if (retVal == getBool(1, &val, "Bad mode ") )
        {
            retVal=FAIL_NODATA;
        }
    }
    else
    {
        // Error - wrong arg count
        sprintf(DataPacket.value,"EROR| Missing Argument or wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        if (argCount == 1)
        {
            pid->SetMode(val) ;
        }
        sprintf(DataPacket.value, "SMOD|%s",  (pid->GetMode() ? "Enabled": "Disabled" ));
        retVal=SUCCESS_DATA;
    }

    return(retVal);
}

/**
 * @brief Set the mode (auto or manual)
 */
 void PidDevice::setMode(bool modeIsAuto)
 {
    pid->SetMode(modeIsAuto);
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
    int32_t stime=mySampleTime;
    if (argCount == 1)
    {
        if ( 0 != getInt32(1, &stime, "Bad mode "))
        {
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
        if (argCount == 1)
        {
            pid->SetSampleTime(stime);
            mySampleTime=stime;
        }
        sprintf(DataPacket.value,"STIM|%s",  (pid->GetMode() ? "Enabled": "Disabled" ));
        retVal=SUCCESS_DATA;
    }

    return(retVal);
}

/**
 * @brief set how often the PID loop re-calculates.
 * MUST be longer than QuadDecoder's sample time!
 */
void PidDevice::setSampleClock(time_t intervalMs)
{
    pid->SetSampleTime(intervalMs);
    mySampleTime = intervalMs;
}

