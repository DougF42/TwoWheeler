/**
 * @file QuadDecoder.cpp 
 * @author Doug F (doug@fajaardo.hm)
 * @brief  device driver for the quadrature encoders.
 * @version 0.1
 * @date 2025-07-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "QuadDecoder.h"
#include <math.h>
#include "esp_err.h"
#include "esp_log_buffer.h"

static const char *TAG="QuadDecoder";
/**
 * @brief Construct a new Quad Decoder object
 * 
 * @param _node 
 * @param InName 
 */
QuadDecoder::QuadDecoder(Node *_node, const char *InName): DefDevice(_node, InName)
{
    myEncoder      = new ESP32Encoder;
    spdUpdateTimerhandle=nullptr;
    last_position  = 0;
    last_timecheck = 0;
    last_speed = 0;
    setPhysParams(QUAD_PULSES_PER_REV, WHEEL_DIAM_MM);
    currentSpdCheckRate = SPEED_CHECK_INTERVAL_mSec;
}


/**
 * @brief Destroyer - not really used in ESP environment
 * 
 */
QuadDecoder::~QuadDecoder()
{
    return;
}


/**
 * @brief Setup and start the Quad Decoder
 * 
 * @param cfg - pointer to motor configuration info
 */
void QuadDecoder::setup(MotorControl_config_t *cfg)
{
    // deocder setup
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
 * @param arg - pointer to the appropriate QuadDecoder instance
 */
void QuadDecoder::update_speed_cb(void *arg)
{
    QuadDecoder *me = (QuadDecoder *)arg;

    pulse_t pos_diff;
    time_t  now  = esp_timer_get_time();
    time_t  elapsed;
    pulse_t pos_now = me->myEncoder->getCount();
    pos_diff = pos_now - me->last_position;
    elapsed  = now - me->last_timecheck;
    // me->last_speed = ((double)pos_diff) / (double)elapsed;
    // speed in mm/millisecond
    me->last_speed = ( ((double)pos_diff) * me->convertPosition) / ((double)elapsed);

    //ESP_LOGE(TAG, "update_speed: %ld|%ld|%ld|%lld|%f", 
    //        pos_now, me->last_position, pos_diff, elapsed, me->last_speed);
    me->last_position = me->myEncoder->getCount();
    me->last_timecheck = now;

    return;
}


/**
 * @brief
 *
 * @return ProcessStatus
 */
ProcessStatus QuadDecoder::ExecuteCommand()
{
    ProcessStatus retVal = NOT_HANDLED;
    dist_t wheelDia;
    uint32_t pulseCnt;
    retVal = Device::ExecuteCommand();
    if (retVal != NOT_HANDLED )  return(retVal);

    scanParam();
   if (isCommand("QSET")) 
    {   // set wheel dia and pulses. 
        retVal = qsetCommand();

    } else if (isCommand("QRST"))
    {  // Reset position
        resetPosition();
        retVal=SUCCESS_NODATA;

    } else if (isCommand("QSCK"))
    {  // Set the speed Check Interval
        retVal = qsckCommand();        
    } else 
    {
        sprintf(DataPacket.value, "EROR|Quad|Unknown command:%s", CommandPacket.command);
        retVal=FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "OK");
        retVal = SUCCESS_DATA;
    }
    return(retVal);
}


/**
 * @brief Report current speed and position
 *
 * @return ProcessStatus
 */
ProcessStatus QuadDecoder::DoPeriodic()
{
    ProcessStatus retVal = SUCCESS_DATA;
    sprintf(DataPacket.value, "%f|%f|%s", getPosition(), last_speed, name);

    return(retVal);
}


/**
 * @brief Set the pulses/routation and wheel diam
 *   Format: QSET
 *          return the current pulses-per-rev  and diameter
 *   Format: QSET|<pulses>|<diam>
 *     <pulses is number of positive pulses per rev.
 *          (we configure as quad encoder, so we store
 *           this value *4 )
 *     <diam>  Diameter of wheel. Whatever units this
 *          was measured in will define the units used
 *          for speed.
 *
 * @return ProcessStatus
 */
ProcessStatus QuadDecoder::qsetCommand()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    double wheel;   // temporary wheel diameter
    pulse_t pulses; // temporary number of pulses
    if (argCount == 2)
    {
        if (0 != getInt32(1, &pulses, "Pulse Count: "))
        {
            retVal = FAIL_DATA;
        }
        else if (0 != getDouble(0, &wheel, "Wheel Diameter: "))
        {
            retVal = FAIL_DATA;
        }
        else
        {
            if (pulses < 0)
            {
                sprintf(DataPacket.value, "EROR|Pulse count must be >0");
                retVal = FAIL_DATA;
            }
            else if (wheel < 0)
            {
                sprintf(DataPacket.value, "EROR|WheelDiam must be >0");
                retVal = FAIL_DATA;
            }
            else
            {
                setPhysParams(pulses, wheel);
                retVal = SUCCESS_NODATA;
            }
        }

    } else if (argCount !=0 )
    {
        sprintf(DataPacket.value, "ERRR| wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
    {
        // Show the current parameters
        sprintf(DataPacket.value, "QSET|%f|%lld|%8.5f", wheelDiam, pulsesPerRev, convertPosition);
        retVal = SUCCESS_DATA;
    }

    return (retVal);
}

/**
 * @brief Set (or return) the speed check clock
 *    Format:  QSCK     - get the current clock rate (millisecs)
 *    Format:  QSCK|<period>
 *             <period> is the time period between speed
 *                      checks, in milliseconds
 * @return ProcessStatus
 */
ProcessStatus QuadDecoder::qsckCommand()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    uint32_t newclkRate = 0;
    if (argCount == 1)
    {
        if (0 != getUint32(0, &newclkRate, "Wheel Diameter: "))
        {
            retVal = FAIL_DATA;
        } else {
            setSpeedCheckInterval(newclkRate);
            retVal = SUCCESS_NODATA;
        }

    } else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERRR| wrong number of arguments");
        retVal = FAIL_DATA;

    }

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "OK|%f", currentSpdCheckRate);
        retVal=SUCCESS_DATA;
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
void QuadDecoder::setPhysParams(pulse_t pulseCnt, double diam)
{
    pulsesPerRev = pulseCnt;
    wheelDiam    = diam;
    convertPosition = (pulsesPerRev*4) / (diam* M_PI);
    return;
}


/**
 * @brief set the speed update interval
 *
 * @param interval - desired interval, in milliseconds
 * @return true  - normal return
 * @return false  - error detected - failed
 */
void QuadDecoder::setSpeedCheckInterval(time_t interval)
{
    currentSpdCheckRate = interval * 1000;
    if (esp_timer_is_active(spdUpdateTimerhandle))
    {
        ESP_ERROR_CHECK(esp_timer_stop(spdUpdateTimerhandle));
    }
    ESP_ERROR_CHECK(esp_timer_start_periodic(spdUpdateTimerhandle, interval * 1000));
    return;
}

/**
 * @brief Return the last calculated position.
 *   (this is in engineering units)
 * @return pulse_t
 */
double QuadDecoder::QuadDecoder::getPosition()
{
    double result= myEncoder->getCount() * convertPosition;
    return(result);
}

/**
 * @brief Reset the position, speed, etc to 0
 *
 */
void QuadDecoder::resetPosition()
{
    myEncoder->clearCount();
    last_position = 0;
    last_timecheck = millis();
    last_speed = 0;
}