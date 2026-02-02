/**
 * @file DEV_QuadDecoder.cpp 
 * @author Doug F (doug@fajaardo.hm)
 * @brief  Device driver for the quadrature encoders.
 * @version 0.1
 * @date 2025-07-15
 * 
 * @copyright Copyright (c) 2025
 *   The ESP32Encoder library is set to have the encoder generate an
 * interrupt on any pulse, and record the change in position.
 * 
 *   To get speed, We run in a separate task. 
 *   Each time thru the loop we wait for an external request for a known period
 *   We read the current count then read the current count, and calculate the 
 *   current speed.
 *
 *
 */
#include "DEV_QuadDecoder.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "Util.h"
#include "esp_err.h"
#include "esp_log_buffer.h"

static const char *TAG="DEV_QuadDecoder";

/**
 * @brief Construct a new Quad Decoder object
 * 
 * @param _node 
 * @param InName 
 */
DEV_QuadDecoder::DEV_QuadDecoder(const char *InName): Device(InName)
{
    myEncoder      = new ESP32Encoder;  //create a default encoder
    spdUpdateTimerhandle=nullptr;
    last_position  = 0;
    last_timecheck = 0;
    last_speed = 0;
    pulsesPerRev = QUAD_PULSES_PER_REV;
    setPhysParams(QUAD_PULSES_PER_REV, WHEEL_DIAM_MM);
    currentSpdCheckRate = SPEED_CHECK_INTERVAL_mSec;
    
}


/**
 * @brief Destroyer - not really used in ESP environment
 * 
 */
DEV_QuadDecoder::~DEV_QuadDecoder()
{
    return;
}


/**
 * @brief Setup and start the Quad Decoder
 * 
 * @param cfg - pointer to motor configuration info
 */
void DEV_QuadDecoder::setup(MotorControl_config_t *cfg)
{
    // deocder setup.
    ESP32Encoder::useInternalWeakPullResistors = puType::none;
    myEncoder->attachFullQuad(cfg->quad_pin_a, cfg->quad_pin_b);
    resetPosition();

    // Set up the speed update clock
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

    periodicEnabled = false; // Default is no report.
    return;
}


/**
 * @brief Called by High res timer to update the speed
 * 
 * @param arg - pointer to the appropriate DEV_QuadDecoder instance
 */
void DEV_QuadDecoder::update_speed_cb(void *arg)
{
    DEV_QuadDecoder *me = (DEV_QuadDecoder *)arg;
    pulse_t pos_diff;
    time_t  now  = esp_timer_get_time();
    time_t  elapsed;
    uint64_t pos_now = me->myEncoder->getCount();

    // Deltas
    pos_diff = (pos_now - me->last_position);
    elapsed  = (now - me->last_timecheck)/1000;

    // Calc speed
    me->last_speed = ( ((double)pos_diff) * me->pulsesToDist) / ((double)elapsed);
    me->last_position = pos_now;
    me->last_timecheck = now;

    return;
}


/**
 * @brief Execute commands for this device
 * Commands:
 *   QSET  <pulsesPerRev>,<Diameter>
 *   QRST
 *   QSCK  // set speed check interval
 *
 * @return ProcessStatus
 */
ProcessStatus DEV_QuadDecoder::ExecuteCommand(char *command, char *params)
{
    ProcessStatus retVal = NOT_HANDLED;
    dist_t wheelDia;
    uint32_t pulseCnt;

    retVal = Device::ExecuteCommand(command, params);
    if (retVal != NOT_HANDLED)
        return (retVal);

    if (0 == strcmp(command, "QSET"))
    { // set wheel dia and pulses.
        retVal = qsetCommand(command, params);
    }
    
    else if (0 == strcmp(command, "QRST"))
    { // Reset position
        resetPosition();
        retVal = NODATA;
    }
    
    else if (0 == strcmp(command, "QSCK"))
    { // Set the speed Check Interval
        retVal = qsckCommand(command, params);
    }
    
    else if (0 == strcmp(command, "STAT"))
    {
        retVal = statusCommand(command, params);
    }
    
    else
    {
        sprintf(SMACData.values, "EROR|Quad|Unknown command:%s", command);
        retVal = SYSTEM_DATA;
    }

    return (retVal);
}


/**
 * @brief Report current speed and position
 *
 * @return ProcessStatus. SMACData.values is 
 *    loaded with a '1' (record type),<position>, <last_speed>
 */
ProcessStatus DEV_QuadDecoder::DoPeriodic()
{
    sprintf(SMACData.values, "0, %d, %f,%f", getPosition(), last_speed);
    return(WIDGET_DATA);
}


/** 
 * @brief Report configuration  parameters
 * @return WIDGET_DATA (record type 2)
 *   SMACData.values is loaded with parameters as follows:
 * '1' (record Identifier), <pulsesPerRev-int32>,<diameter-double>, <update_rate-long long>, <pulsesToDist>
 */
ProcessStatus DEV_QuadDecoder::statusCommand(char *command, char *params)
{
    sprintf(SMACData.values, "1, %d,%f,%lld,%f", pulsesPerRev,  wheelDiam, currentSpdCheckRate, pulsesToDist);
    return(WIDGET_DATA);
}


/**
 * @brief Set the pulses/routation and wheel diam
 *   Format: QSET
 *
 *   Format: QSET|<pulses>|<diam>
 *     <pulses is number of positive pulses per rev.
 *          (we configure as quad encoder, so we store
 *           this value *4 )
 *     <diam>  Diameter of wheel. Whatever units this
 *          was measured in will define the units used
 *          for speed.
 *
 * @return ProcessStatus - NODATA normally SYSTEM_DATA if an error
 */
ProcessStatus DEV_QuadDecoder::qsetCommand(char *command, char *params)
{
    ProcessStatus retVal = NODATA;
    double wheel;   // temporary wheel diameter
    pulse_t pulses; // temporary number of pulses
    char *tmp=nullptr;
    char *endptr=nullptr;
    errno=0;

    // Get the first arg (pulses per rev)
    tmp = strtok(params, ",\r\n");
    if ((tmp == nullptr) || (strlen(params) == 0))
    {
        sprintf(SMACData.values, "EROR - Missing arguments to qset command");
        retVal = SYSTEM_DATA;
    }
    else
    {
        // Get the second argument
        retVal = Util::getint32_t(tmp, &pulses, "Number of pulses");
        if (retVal == NODATA)
        {
            tmp = strtok(nullptr, ",\r\n");
            if (tmp == nullptr)
            {
                sprintf(SMACData.values, "ERROR - Missing 2nd argument to QSET command");
                retVal = SYSTEM_DATA;
            }
            else
            {
                errno = 0;
                wheel = strtod(tmp, &endptr);
                if ( (errno != 0) || (*endptr != '\0') )
                    {
                        sprintf(SMACData.values, "ERROR - 2nd Argument is invalid ");
                        retVal = SYSTEM_DATA;
                    }
                else
                {
                    setPhysParams(pulses, wheel);
                    retVal = NODATA;
                }
            }

        }
    }
    return (retVal);

}


/**
 * @brief Set the speed check clock
 *    Format:  QSCK|<period>
 *             <period> is the time period between speed
 *                      checks, in milliseconds
 * @return ProcessStatus - NODATA normally, SYSTEM_DATA if error
 */
ProcessStatus DEV_QuadDecoder::qsckCommand(char *command, char *param)
{
    ProcessStatus retVal = NODATA;
    time_t newclkRate = 0;

    char *periodString=strtok(param, ",\r\n");

    if (periodString==nullptr)
    {
        sprintf(SMACData.values, "EROR - missing argument to qsck command");
        retVal=SYSTEM_DATA;

    } else {
        retVal = Util::getLL_t(periodString, &newclkRate, "Period");
        if (retVal == NODATA)
        {
            setSpeedCheckInterval(newclkRate);
            retVal = NODATA;
        }
    }
    return (retVal);
}


/**
 * @brief Set the Phys Paramers
 *    The number of pulses per rotation and diameter are
 * configured, and the conversion from pulse count to distance
 * is calculated.
 * 
 * @param pulseCnt  - number of positive pulses per revolution
 * @param diam      - diameter of the wheel.
 */
void DEV_QuadDecoder::setPhysParams(pulse_t pulseCnt, double diam)
{
    pulsesPerRev = pulseCnt;
    wheelDiam    = diam;
    pulsesToDist = (pulsesPerRev*4) / (diam* M_PI);
    return;
}


/**
 * @brief set the speed update interval
 *
 * @param interval - desired interval, in milliseconds
 * @return true  - normal return
 * @return false  - error detected - failed
 */
void DEV_QuadDecoder::setSpeedCheckInterval(time_t interval)
{
    currentSpdCheckRate = interval * 1000;
    if (esp_timer_is_active(spdUpdateTimerhandle))
    {
        ESP_ERROR_CHECK(esp_timer_restart(spdUpdateTimerhandle, interval*1000));
    } else {
        ESP_ERROR_CHECK(esp_timer_start_periodic(spdUpdateTimerhandle, interval * 1000));
    }
    return;
}

/**
 * @brief Return the last calculated position.
 *   (this is in engineering units)
 * @return pulse_t
 */
double DEV_QuadDecoder::DEV_QuadDecoder::getPosition()
{
    double result= myEncoder->getCount() * pulsesToDist;
    return(result);
}

/**
 * @brief Reset the position, speed, etc to 0
 *
 */
void DEV_QuadDecoder::resetPosition()
{
    myEncoder->clearCount();
    last_position = 0;
    last_timecheck = millis();
    last_speed = 0;
}

/**
 * Retrieve the last calculated speed
 */
double DEV_QuadDecoder::getSpeed()
{
    return(last_speed);
}