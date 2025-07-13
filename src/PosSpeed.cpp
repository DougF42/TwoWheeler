/**
 * @file PosSpeed.cpp
 * @author Doug Fajardo
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "PosSpeed.h"

/** = = = = = = = = = = = = = = = = = = = =
 * Initialize a new instance of the class
 *  = = = = = = = = = = = = = = = = = = = =
 */
PosSpeed::PosSpeed(Node *_node, const char *InName,  QuadReader *myQuad): DefDevice(_node, InName)
{
        dist_t wheelDia      = WHEEL_DIAM_MM;
        pulse_t pulsesPerRev = QUAD_PULSES_PER_REV;
        myQuadPtr            = myQuad;
        prevPulsePos         = myQuadPtr->getPosition();
}

/** = = = = = = = = = = = = = = = = = = = =
 * Destructor - nerver happens
 *  = = = = = = = = = = = = = = = = = = = =
 */
PosSpeed::~PosSpeed()
{
    return;
}


/** = = = = = = = = = = = = = = = = = = = =
 * Set up the update timer
 *  = = = = = = = = = = = = = = = = = = = =
 */
bool PosSpeed::setup(MotorControl_config_t *cfg)
{
    // speed check timer
    esp_timer_create_args_t speed_timer_args =
        {
            .callback = &update_speed_cb,      //!< Callback function to execute when timer expires
            .arg = this,                       //!< Argument to pass to callback
            .dispatch_method = ESP_TIMER_TASK, //!< Dispatch callback from task or ISR; if not specified, esp_timer task
                                               //!< is used; for ISR to work, also set Kconfig option
                                               //!< `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`
            .name = "SpeedTimer",              //!< Timer name, used in esp_timer_dump() function
            .skip_unhandled_events = true      //!< Setting to skip unhandled events in light sleep for periodic timers
        };

    ESP_ERROR_CHECK(esp_timer_create(&speed_timer_args, &spdUpdateTimerhandle));
    Serial.print("... Speed timer created");

    setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);
    Serial.printf("... Interval is %d (mseconds)\n\r", SPEED_CHECK_INTERVAL_mSec);

    resetPos();
    periodicEnabled = false; // Default is no report.
    Serial.print("Setup for "); Serial.print(name); Serial.println(" finished");
    return(true);
}


/** = = = = = = = = = = = = = = = = = = = =
 * This is called by the timer to update our speed
 *  = = = = = = = = = = = = = = = = = = = =
 */
void PosSpeed::update_speed_cb(void *arg)
{
    PosSpeed *me = (PosSpeed *) arg;

    // OBSERVED
    time_t now  = esp_timer_get_time();
    pulse_t newPos = me->myQuadPtr->getPosition();
    // ELAPSED
    time_t elapsed = now - me->lastUpdateTime;
    pulse_t pulseDiff = newPos - me->prevPulsePos;
    // RATE
    me->lastpulseRate = (double )pulseDiff / (double)elapsed;
    
}

/** = = = = = = = = = = = = = = = = = = = =
 * If enabled, report the position and speed
 *  = = = = = = = = = = = = = = = = = = = =
 */
// Override this method to periodically send reports
ProcessStatus PosSpeed::DoPeriodic()
{
    ProcessStatus retVal = NOT_HANDLED;

    return (retVal);
}

/** = = = = = = = = = = = = = = = = = = = =
 * decode and dispatch to SMAC commands
 *  = = = = = = = = = = = = = = = = = = = =
 */
// Override this method to handle custom commands
ProcessStatus PosSpeed::ExecuteCommand()
{
    ProcessStatus retVal = NOT_HANDLED;

    return (retVal);
}
