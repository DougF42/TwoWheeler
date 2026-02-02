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
#include <strings.h>
#include "Util.h"

DEV_Pid::DEV_Pid( const char *_name)  : Device( _name)
{
    name = strdup(_name);
    pid = nullptr;
    ln298 = nullptr;
    quad  = nullptr;
}
    
void DEV_Pid::setup (MotorControl_config_t *cfg, DEV_QuadDecoder * _quad, DEV_LN298 * _ln298 )
{
    ln298 = _ln298;
    quad  = _quad;

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
    sprintf(SMACData.values, "%d,%lf,%lf,%lf",  pid->GetMode(), setPoint, actual, output);
    return (WIDGET_DATA);
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
ProcessStatus DEV_Pid::ExecuteCommand(char *command, char *params)
{
    ProcessStatus retVal = NODATA;
    retVal = Device::ExecuteCommand(command, params);
    if (retVal == NOT_HANDLED)
    {
        if (0 == strcasecmp(command, "SPED"))
        { // Set speed (setpoint)
            retVal = cmdSetSpeed(command,params);
        }
        else if (0 == strcasecmp(command, "SETP"))
        { // Set 'p' parameter
            retVal = cmdSetP(command,params);
        }
        else if (0 == strcasecmp(command, "SETI"))
        { // Set i parameter
            retVal = cmdSetI(command,params);
        }
        else if (0 == strcasecmp(command, "SETD"))
        { // Set D
            retVal = cmdSetD(command,params);
        }

        else if (0 == strcasecmp(command, "SMOD"))
        { // Set mode (auto or manual)
            retVal = cmdSetMode(command,params);
        }

        else if (0 == strcasecmp(command, "STIM"))
        { // Set pid update time (millisecs)
            retVal = cmdSetSTime(command,params);
        }
    }
    return (retVal); // for now...
}

/**
 * @brief: Set the desired speed.
 *
 *    FORMAT:  SPED|<speed>
 *        <speed in cm/sec ???>
 *    Note: This works wether we are
 * in MANUAL or AUTOMATIC modes
 */
ProcessStatus DEV_Pid::cmdSetSpeed(char *command, char *params)
{
    ProcessStatus retVal=NODATA;
    double newSetPoint;
    retVal=Util::getDouble_t(params, &newSetPoint, "Speed ");
    if ( retVal == WIDGET_DATA)
        {
            setSpeed(newSetPoint);
            retVal = NODATA;
        }
    return(retVal);
}

/**
 * @brief set the desired motor speed
 * IF we are in 'manual' mode, this sets
 * both the ln298 speed AND the pidx input.
 * Otherwise, only set the 
 * 
 */
void DEV_Pid::setSpeed(double speed)
{
    if ( pid->GetMode() == MANUAL)
    { // manual mode - tell ln298 directly
        ln298->setPulseWidth(speed);
    }
    setPoint = speed; // tell the PIDX
    return;
}

/**
 * @brief Get or Set the PID parameters
 * Format:   SETP|<kp>
 * 
 * @return ProcessStatus 
 */
ProcessStatus DEV_Pid::cmdSetP(char *command, char *params)
{
    ProcessStatus retVal=NODATA;
    double tmpVal=0;
    
    retVal=Util::getDouble_t(params, &tmpVal, "Kp ");
    
    if (retVal == WIDGET_DATA)
    {
        kp=tmpVal;
        pid->SetTunings(kp, ki, kd);
        retVal=NODATA;
    }

    return(retVal);
}


/**
 * @brief Get or Set the PID parameters
 * Format:   SETI|<ki>
 * 
 * @return ProcessStatus 
 */
ProcessStatus DEV_Pid::cmdSetI(char *command, char *params)
{
    ProcessStatus retVal=NODATA;
    double tmpVal=0;
    retVal = Util::getDouble_t(params, &tmpVal, "Ki ");
    if (retVal == WIDGET_DATA)
    {
        ki = tmpVal;
        pid->SetTunings(kp, ki, kd);
    }

    return(retVal);
}


/**
 * @brief Get or Set the PID parameters
 * Format:   SETD|<kd>
 * 
 * @return ProcessStatus 
 */
ProcessStatus DEV_Pid::cmdSetD(char *command, char *params)
{
    ProcessStatus retVal=NODATA;
    double tmpVal;

    retVal = Util::getDouble_t(params, &tmpVal, "Kd");
    if (retVal == WIDGET_DATA)
    {
        kd = tmpVal;
        pid->SetTunings(kp, ki, kd);
        retVal = NODATA;
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
ProcessStatus DEV_Pid::cmdSetMode(char *command, char *params)
{
    ProcessStatus retVal = NODATA;
    bool val = false;
    retVal == Util::getbool(params, &val, "Bad mode ");
    if (retVal == WIDGET_DATA)
    {
        pid->SetMode(val);
        retVal = NODATA;
    }
    return (retVal);
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
ProcessStatus DEV_Pid::cmdSetSTime(char *command, char *params)
{
    ProcessStatus retVal = NODATA;
    int32_t stime=mySampleTime;
    retVal = Util::getint32_t(params, &stime, "Compute time ");

    if (retVal == WIDGET_DATA)
        {
            mySampleTime=stime;
            setSampleClock(mySampleTime);
            retVal = NODATA;
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
 *    This is a callback from the high-priority timer task, which
 *  triggers the compute step of the PID controler.
 * 
 *    This is responsible for
 *        (1) Setting the 'actual' speed before calling the PID controller
 *        (2) Controling the ln298 (from the PID output).
 *    (NOTE: nothing done if ln298 is disabled, or pid is manual)
 * @param arg pointer to 'this' instance of DEV_Pid
 */
void DEV_Pid::timer_callback(void *arg)
{
    DEV_Pid *me = (DEV_Pid *)arg;
    if (me->ln298->isDisabled() || (me->pid->GetMode()==MANUAL)) return;

    // Get the 'actual' speed value from the QUAD.
    me->actual = me->quad->getSpeed(); // get actual speed
    
    // RUN COMPUTE
    if (me->pid->ComputeFromTimer())
    {
        // Now share the 'output' value...
        me->ln298->setPulseWidth(me->output);
    }
}