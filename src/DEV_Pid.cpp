/**
 * @file DEF_Pid.cpp
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
 * 7/26/2026 DEF Use timer to drive PID compute.
 */

#include "DEV_Pid.h"

DEV_Pid::DEV_Pid( const char *_name, MotorControl_config_t *cfg, 
        DEV_QuadDecoder *_quad, DEV_LN298 *_ln298 ) : DefDevice( _name)
{
    pid = nullptr;
    ln298 = _ln298;
    quad  = _quad;

    name = strdup(_name);
        //PID(double*, double*, double*,        // * constructor.  links the PID to the actual, Output, and 
        // double, double, double, int, int);   //   Setpoint.  Initial tuning parameters are also set here.
                                                //   (overload for specifying proportional mode)
    pid = new PIDX(&actual, &output, &setPoint,  // links the PID to the actual, Output, and setpoint
        cfg->kp, cfg->ki, cfg->kd, P_ON_E, 0);    // Kp, Ki, Kd, POn, invertFlag
    pid->SetOutputLimits(0.0, 100.0);           //  We cant do any better than 100 % !!!!
    pid->SetTunings(DEFAULT_Kp, DEFAULT_Ki, DEFAULT_Kd);
    pid->SetMode(AUTOMATIC); // MANUAL ????
    periodicEnabled=false;

    esp_timer_create_args_t timer_cfg {
        .callback=timer_callback,        //!< Callback function to execute when timer expires
        .arg=this,                       //!< Argument to pass to callback
        .dispatch_method=ESP_TIMER_TASK, //!< Dispatch callback from task or ISR; if not specified, esp_timer task
                                         //!< is used; for ISR to work, also set Kconfig option
                                         //!< `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`
        .name="PIDtimer",          //!< Timer name, used in esp_timer_dump() function
        .skip_unhandled_events=true     //!< Setting to skip unhandled events in light sleep for periodic timers
    };
    ESP_ERROR_CHECK (esp_timer_create( &timer_cfg, &pidTimerhandle)); // DEFINE A TIMER
    setSampleClock(PID_SAMPLE_TIME_ms);   // set sample time using default
    // ESP_ERROR_CHECK (esp_timer_start_periodic(pidTimerhandle,  mySampleTime*1000) ); // And start it!

}


DEV_Pid::~DEV_Pid()
{
    return;
}


/**
 * @brief periodically -  report  current values
 * @param arg
 */
ProcessStatus DEV_Pid::DoPeriodic()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    DataPacket.timestamp = millis();    
    sprintf(DataPacket.value, "PID|%lf|%lf|%lf", setPoint, actual, output);
    retVal = SUCCESS_DATA;

    return (retVal);
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
ProcessStatus DEV_Pid::ExecuteCommand()
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
ProcessStatus DEV_Pid::cmdSetSpeed()
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
void DEV_Pid::setSpeed(double speed)
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
ProcessStatus DEV_Pid::cmdSetP()
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
ProcessStatus DEV_Pid::cmdSetI()
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
ProcessStatus DEV_Pid::cmdSetD()
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
ProcessStatus DEV_Pid::cmdSetMode()
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
    else if (argCount != 0)
    {
        // Error - wrong arg count
        sprintf(DataPacket.value,"EROR| Wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        if (argCount == 1)
        {
            pid->SetMode(val) ;
        }
        sprintf(DataPacket.value, "SMOD|%s",  (pid->GetMode()==AUTOMATIC) ? "Automatic": "Manual" );
        retVal=SUCCESS_DATA;
    }

    return(retVal);
}

/**
 * @brief Set the mode (auto or manual)
 */
 void DEV_Pid::setMode(bool modeIsAuto)
 {
    pid->SetMode(modeIsAuto);
 }


/**
 * @brief  Command to Set the PID compute time (milliseconds)
 *    FORMAT: STIM|<time>
 * 
 * @return ProcessStatus 
 */
ProcessStatus DEV_Pid::cmdSetSTime()
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
            setSampleClock(mySampleTime);
            mySampleTime=stime;
        }
        sprintf(DataPacket.value,"STIM|%s",  (pid->GetMode() ? "Enabled": "Disabled" ));
        retVal=SUCCESS_DATA;
    }

    return(retVal);
}


/**
 * @brief set how often the PID loop re-calculates.
 * MUST be longer than DEV_QuadDecoder's sample time!
 */
void DEV_Pid::setSampleClock(time_t intervalMs)
{
    mySampleTime = intervalMs;

    pid->SetSampleTime(mySampleTime);

    // IF timer is active, stop it and restart
    if (esp_timer_is_active(pidTimerhandle))
    {
        esp_timer_restart(pidTimerhandle, mySampleTime*1000);
    } else {
        esp_timer_start_periodic(pidTimerhandle, mySampleTime*1000);
    }
}


/**
 * @brief Run the PID Comput function
 *    This is a callback from the high-priority timer task
 *    (NOTE: nothing done if ln298 is disabled, or pid is manual)
 * @param arg pointer to 'this' instance of DEV_Pid
 */
void DEV_Pid::timer_callback(void *arg)
{
    DEV_Pid *me = (DEV_Pid *)arg;
    if (me->ln298->isDisabled() || (me->pid->GetMode()==MANUAL)) return;

    // The setpoint was previously set by calling 'setSpeed()'
    // Get the 'actual' speed value from the QUAD.
    me->actual = me->quad->getSpeed(); // get actual speed
    
    // RUN COMPUTE, set the new output
    if (me->pid->ComputeFromTimer())
    {
        // Now share the 'output' value...
        me->ln298->setPulseWidth(me->output);
    }
}