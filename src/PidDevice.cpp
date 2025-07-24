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
    pid = new PID(&actual, &output, &setPoint,  // links the PID to the actual, Output, and setpoint
        cfg->kp, cfg->ki, cfg->kd, P_ON_E, 0);    // Kp, Ki, Kd, POn, invertFlag
        
    periodicEnabled=false;
        /// TODO: Force MANUAL mode?

    // - - - -  Initialize and Start the timer
       // Set up the speed update clock
     // speed check timer
    esp_timer_create_args_t speed_timer_args =
        {
            .callback = &update_pid_cb,      //!< Callback function to execute when timer expires
            .arg = this,                       //!< Argument to pass to callback
            .dispatch_method = ESP_TIMER_TASK, //!< Dispatch callback from task or ISR; if not specified, esp_timer task
                                               //!< is used; for ISR to work, also set Kconfig option
                                               //!< `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`
            .name = "SpeedTimer",              //!< Timer name, used in esp_timer_dump() function
            .skip_unhandled_events = true      //!< Setting to skip unhandled events in light sleep for periodic timers
        };

    ESP_ERROR_CHECK(esp_timer_create(&speed_timer_args, &pidTimerhandle));
    Serial.print("... PID timer created");


}


PidDevice::~PidDevice()
{
}

/**
 * @brief periodically -  report  setValue, actual, output
 * @param arg - pointer to this instance
 */
ProcessStatus PidDevice::DoPeriodic()
{
    ProcessStatus retVal = SUCCESS_NODATA;

    sprintf(DataPacket.value, "PID|%lf|%lf|%lf", setPoint, actual, output);
    retVal = SUCCESS_DATA;

    return (retVal);
}


/**
 * @brief Run the PID loop when the timer expires
 *  The argument is a pointer to the current PidDevice instance
 */
 void PidDevice::update_pid_cb(void *arg)
 {
    PidDevice *me = (PidDevice *)arg;
    me->pid->Compute();
    me->ln298->setPulseWidth((int)me->output);
    return;
 }


/**
 * @brief Decode (and implement) SMAC commands
 * 
 * FORMAT:  SPID <Kp> <Ki> <Kd>
 * FORMAT:  SMODE <bool>         (true-automatic, false-manual)
 * FORMAT:  STIM <time_ms>      (sample time rate - via DOIMMEDIATE)
 * @return ProcessStatus 
 */
ProcessStatus PidDevice::ExecuteCommand()
{
    ProcessStatus retVal = SUCCESS_DATA;

    retVal = Device::ExecuteCommand();
    if (retVal != NOT_HANDLED)
        return (retVal);
    retVal = NOT_HANDLED;

    scanParam();
    if (isCommand("SPED"))
    {   // Set s[eed]
        retVal = cmdSetSpeed();

    } else if (isCommand("SPID"))
    {   // Set pid parameters
        // TODO:
        retVal = cmdSetPid();
    }

    else if (isCommand("SMODE"))
    {    // Set mode (auto or manual)
        retVal = cmdSetPid();
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
    // TODO:

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
 * @brief set the PID control values
 */
void setPid(double _kp, double ki, double kd)
{
    // TODO:
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

    return(retVal);
}

/**
 * @brief Set the mode (auto or manual)
 */
 void PidDevice::setMode(bool modeIsAuto)
 {
    // TODO
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

    return(retVal);
}

void PidDevice::setSampleClock(time_t intervalMs)
{
    // TODO:
}

