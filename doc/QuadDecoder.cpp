/**
 * @file QuadDecoder.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "config.h"
#include "QuadDecoder.h" 
#include "esp_log.h"
// This defines a pin that we pull high upon entetring the ISR, and
// set low when we exit. This can be used as a time index (trigger)
// on an oscilliscope, and to determine how long the ISR is taking.
#define USE_TRACKER_PIN GPIO_NUM_13

// For testing, it may be convenient to suppress the clock timer
// which controls our speed update check routine
// This is UNDEFINED to suppress the speed check.
#define USE_SPEED_CHECK

// For logging
#define DEBUG_QUAD 1
#ifdef DEBUG_QUAD
#include "esp_log.h"
static const char *TAG="*QuadDecoder*";
// esp_log_level_set( TAG, ESP_LOG_VERBOSE);
// ESP_DRAM_LOGE(TAG,"format", vars);
#endif

// DONT CHANGE THE assignments!!!
//     The assignments corresponds to binary state of the
//         inputs, assuming a is bit 1, b is bit 0
//
#define AoffBoff 0
#define AoffBon 1
#define AonBoff 2
#define AonBon 3

// - - - - - - - STATIC DEFS - - - - - - - - - - -
bool QuadDecoder::isrAlreadyInstalled=false;

// This MUST be separate from the class, and must
// be kept as small as possible - it determines
// the size of a structure that is stored in DRAM for the
// ISR routines will work properly.
#define MAX_NO_OF_QUAD_DECODERS 2

static QuadDecoder::Quad_t quads[MAX_NO_OF_QUAD_DECODERS];
int QuadDecoder::nextQuadIdx = 0;

// To avoid problems with overlaping interrupts and
//   mainline code... This is used by portEnter/Exit Critical section
//  to control access to the QUADS structure.
//
static portMUX_TYPE quad_ctrl_mutex = portMUX_INITIALIZER_UNLOCKED;

/** = = = = = = = = = = = = = = =
 *  Initialize
 * = = = = = = = = = = = = = = =
 */
QuadDecoder::QuadDecoder(Node *_node, const char * InName) : DefDevice(_node, InName)
{
    quadPtr=nullptr;
    speedUpdateCount = 0;
    last_position    = 0;
    last_speed       = 0.0;
    last_update_time=esp_timer_get_time();
    #ifdef DEBUG_QUAD
     esp_log_level_set( TAG, ESP_LOG_VERBOSE);
    #endif
    return;
}


/** = = = = = = = = = = = = = = =
 *  Desructor - never happens on ESP32
 * = = = = = = = = = = = = = = =
 */
QuadDecoder::~QuadDecoder()
{
    return;
}


/** = = = = = = = = = = = = = = =
 * @brief Set up the fast timer
 *     This triggers our periodic 
 * update of the speed and position
 * 
 * = = = = = = = = = = = = = = = 
 */
void QuadDecoder::setupQuad(MotorControl_config_t *cfg)
{
    //  Sanity checks, assign quad index number, populate it
    if (nextQuadIdx < MAX_NO_OF_QUAD_DECODERS)
    {
        quadPtr = &(quads[nextQuadIdx++]);

    } else  {
        Serial.println("*** ERROR! TOO MANY QUADS - quadIdx not assigned!");
        return;
    }

    quadPtr->name = strdup(GetName());
    quadPtr->quad_pin_a = cfg->quad_pin_a;
    quadPtr->quad_pin_b = cfg->quad_pin_b;
    quadPtr->last_state = AoffBoff;
    quadPtr->pulseCount = 0;

    // Set default pulse count and wheel diam
    calibrate(QUAD_PULSES_PER_REV, WHEEL_DIAM_MM);

    // - - - - - -
    // CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
    gpio_config_t pGpioConfig =
    {
        .pin_bit_mask = (1ull << quadPtr->quad_pin_a) | (1ull << quadPtr->quad_pin_b),
        .mode = GPIO_MODE_INPUT,               /*!< GPIO mode: set input/output mode  */
        .pull_up_en = GPIO_PULLUP_ENABLE,      /*!< GPIO pull-up                    */
        .pull_down_en = GPIO_PULLDOWN_DISABLE, /*!< GPIO pull-down                 */
        .intr_type = GPIO_INTR_ANYEDGE,        /*!< GPIO interrupt type                */
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        .hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE; /*!< GPIO hysteresis: hysteresis filter on slope input    */
#endif
    };
    ESP_ERROR_CHECK(gpio_config(&pGpioConfig));
    Serial.printf("Installed quad i/o. about to install interrupt ISR\n\r");
    if (!isrAlreadyInstalled)
    {
        isrAlreadyInstalled = true;
        ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
        Serial.println("GPIO ISR Service installed");
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(quadPtr->quad_pin_a, ISR_handler, quadPtr));
    ESP_ERROR_CHECK(gpio_isr_handler_add(quadPtr->quad_pin_b, ISR_handler, quadPtr));
    Serial.println("... Quad ISR handlers added");

    #ifdef USE_SPEED_CHECK
    // - - - - - - - - - - 
    // NOW SET UP THE SPEED CHECK TIMER)
    esp_timer_create_args_t speed_timer_args =
        {
            .callback = &update_speed_CB,     //!< Callback function to execute when timer expires
            .arg = this,                       //!< Argument to pass to callback
            .dispatch_method = ESP_TIMER_TASK, //!< Dispatch callback from task or ISR; if not specified, esp_timer task
                                               //!< is used; for ISR to work, also set Kconfig option
                                               //!< `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`
            .name = "SpeedTimer",              //!< Timer name, used in esp_timer_dump() function
            .skip_unhandled_events = true      //!< Setting to skip unhandled events in light sleep for periodic timers
        };

    ESP_ERROR_CHECK(esp_timer_create(&speed_timer_args, &spdUpdateTimer));
    Serial.print("... Speed timer created");

    setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);
    Serial.printf("... Interval is %d (mseconds)\n\r", SPEED_CHECK_INTERVAL_mSec);
    #endif
    resetPos();
    periodicEnabled = false;  // Default is no report.



#ifdef USE_TRACKER_PIN
    gpio_config_t pTrackerCfg =
    {.pin_bit_mask = (1ull << USE_TRACKER_PIN),
     .mode = GPIO_MODE_OUTPUT,              /*!< GPIO mode: set input/output mode  */
     .pull_up_en = GPIO_PULLUP_DISABLE,     /*!< GPIO pull-up                    */
     .pull_down_en = GPIO_PULLDOWN_DISABLE, /*!< GPIO pull-down                 */
     .intr_type = GPIO_INTR_DISABLE,        /*!< GPIO interrupt type                */
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
     .hys_ctrl_mode = GPIO_HYS_SOFT_DISABLE; /*!< GPIO hysteresis: hysteresis filter on slope input    */
#endif
    }
    ;
    ESP_ERROR_CHECK(gpio_config(&pTrackerCfg));
    ESP_ERROR_CHECK(gpio_set_level(USE_TRACKER_PIN, 1)); //set high    
    gpio_dump_io_configuration(stdout, pTrackerCfg.pin_bit_mask | pGpioConfig.pin_bit_mask);
#else
    gpio_dump_io_configuration(stdout, pGpioConfig.pin_bit_mask);
#endif
    return;
    }

/** = = = = = = = = = = = = = = =
 * @brief ISR routine - update our position when a pulse happens
 *
 * @param arg - points to the current quads structure
 * 
 * = = = = = = = = = = = = = = =
 */
void IRAM_ATTR QuadDecoder::ISR_handler(void *arg)
{
    static time_t initial_time = 0;
    initial_time=esp_timer_get_time();

#ifdef USE_TRACKER_PIN
    gpio_set_level(USE_TRACKER_PIN, 0);
#endif

    Quad_t *qptr = (Quad_t *)arg;     // point to my QuadDecoder    
    
    // save the current state of the pins
    int pina, pinb;
    pina=gpio_get_level(qptr->quad_pin_a); // get the current value of pinA    
    pinb=gpio_get_level(qptr->quad_pin_b); // get the current value of pinB
    uint8_t newState = ((pina<<1) | pinb); // combine pina+pinb to get state

    //portENTER_CRITICAL_ISR(&quad_ctrl_mutex);
    qptr->pulseCount= qptr->pulseCount+1L; // statistics...

    // DECODE the quad state, update our position and state
    switch (qptr->last_state)
    {
    case (AoffBoff):
        if (newState == AonBoff)
        {
            qptr->absPosition++;
            qptr->last_state = newState;
        }
        else if (newState == AoffBon)
        {
            qptr->absPosition--;
            qptr->last_state = newState;
            break;
        }

    case (AoffBon):
        if (newState == AoffBoff)
        {
            qptr->absPosition++;
            qptr->last_state = newState;
        } else if (newState == AonBon)
        {
            qptr->absPosition--;
            qptr->last_state = newState;
        }
        break;

    case (AonBoff):
        if (newState == AonBon)
        {
            qptr->absPosition++;
            qptr->last_state = newState;
        } else if (newState == AoffBoff)
        {
            qptr->absPosition--;
            qptr->last_state = newState;
        }        
        break;

    case (AonBon):
        if (newState == AoffBon)
        {
            qptr->absPosition++;
            qptr->last_state = newState;
         } else if (newState == AonBoff)
         {
            qptr->absPosition--;
            qptr->last_state = newState;
        }
        break;

    }

    // portEXIT_CRITICAL_ISR(&quad_ctrl_mutex);
    // How long was the interrupt?  Only here for testing!
    #ifdef NONONO
    time_t time_now = esp_timer_get_time();
    time_t diff     = time_now - initial_time;
    ESP_DRAM_LOGE(TAG, "end: %llu start %llu  Elapsed:%llu", initial_time, time_now, diff);
    #endif

#ifdef USE_TRACKER_PIN
    gpio_set_level(USE_TRACKER_PIN, 1);
#endif
    return;
}

/** = = = = = = = = = = = = = = =
 * @brief Re-calculate the speed
 * 
 *    Driven from speed_timer (as a task dispatch?),
 * this determines our speed, based on the change in 
 * position since the last call and the time period
 * since that call.
 * 
 * The resulting speed is in 'pulses' (not scaled
 * to physical units)
 * 
 * NOTE: This is called from the high-priority
 *       timer task.
 * 
 * @param arg - points to the 'quadDecoder' instance
 *   that this was called from.
 * = = = = = = = = = = = = = = =
 */
void QuadDecoder::update_speed_CB(void *arg)
{
    QuadDecoder *me=(QuadDecoder *)arg;    
    me->speedUpdateCount = me->speedUpdateCount+1;  // statistics..
    
    pulse_t cur_pos;
    time_t cur_time = esp_timer_get_time();

    // portENTER_CRITICAL_ISR(&quad_ctrl_mutex);
    cur_pos=me->quadPtr->absPosition;
    // portEXIT_CRITICAL_ISR(&quad_ctrl_mutex);
    
    pulse_t delta_pos = (double) (cur_pos - me->last_position);
    time_t elapsed = cur_time - me->last_update_time;

    //Serial.print("**UDATESPEED: elapsed(msec) = "); Serial.print(elapsed/1000);
    //Serial.print("   DeltaPos=");                Serial.println(delta_pos);
    if (labs(delta_pos) < 1)
    {
        me->last_speed = 0;       
    } else {       
        me->last_speed =  (((double)delta_pos) * me->convertPulsesToDist)/ ((double)elapsed*1000.0);
    }
    me->last_position = cur_pos;
    me->last_update_time = cur_time;
    return;
}

/** = = = = = = = = = = = = = = =
 * @brief Run this periodically...
 * 
 * Output the current position
 * = = = = = = = = = = = = = = =
 */
ProcessStatus QuadDecoder::DoPeriodic()
{
    pulse_t pos;
    pulse_t count;

    portENTER_CRITICAL(&quad_ctrl_mutex);
    count=quadPtr->pulseCount;
    pos= quadPtr->absPosition;
    portEXIT_CRITICAL(&quad_ctrl_mutex);

    dist_t tmpSpeed=last_speed; 
    sprintf(DataPacket.value, "QUAD absPos=%ld, ISR count=%lu", pos, count);
    defDevSendData(millis(), false);
    return (SUCCESS_DATA);
}

/** = = = = = = = = = = = = = = =
 * @brief Decode commands for the QUAD decoder
 * 
 * Commands:
 *      QSET- set <wheel diameter> and <number of pulses per rotation>
 *  QSTA <ON..OFF>     enable/disable report
 * @return ProcessStatus 
 * = = = = = = = = = = = = = = =
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
        retVal = cmdQSET();

    } else if (isCommand("QRST"))
    {  // Reset position
        resetPos();
        retVal=SUCCESS_NODATA;

    } else if (isCommand("QSCK"))
    {  // Set the speed Check Interval
        retVal = cmdSetSpeedCheckInterval();        
    } else 
    {
        sprintf(DataPacket.value, "EROR|Quad|Unknown command:%s", CommandPacket.command);
        retVal=FAIL_DATA;
    }

    return (retVal);
}

/** = = = = = = = = = = = = = = =
 * @brief Decode the command to Set the Wheel diameter and pulses per rev
 *    format:   QSET|<wheelDia>|<pulseCnt>   -set these params
 *          wheel dia in mm
 *          pulsecnt    - how many pulse holes in 1 full turn?
 *    format:   QSET                         - just report params
 * 
 *  NOTE: Whatever units wheelDia are in, thats the units used to report distance!
 * 
 * @return - we always return the current parameters
 * = = = = = = = = = = = = = = =
 */
ProcessStatus QuadDecoder::cmdQSET()
{
    ProcessStatus res = SUCCESS_NODATA;

    if (argCount == 2)
    {
        double wheel;       // temporary wheel diameter
        uint32_t pulses;   // temporary number of pulses
        if (argCount == 2)
        {
            if (0 != getDouble(0, &wheel, "Wheel Diameter: ") )
            {
                res = FAIL_DATA;
            }
            else if (0 != getUint32(1, &pulses, "Pulse Count: "))
            {
                res = FAIL_DATA;
            }
            else
            {
                setQuadParams(wheel, pulses);
                res = SUCCESS_NODATA;
            }
        }
        else if (argCount != 0)
        {
            sprintf(DataPacket.value, "ERRR| wrong number of arguments");
            res = FAIL_DATA;
        }
    }

    if (res == SUCCESS_NODATA)
    {
        // Show the current parameters
        sprintf(DataPacket.value, "QSET|%f|%d|%8.4f", wheelDiameter, pulsesPerRev / 4, convertPulsesToDist);
        res = SUCCESS_DATA;
    }

    defDevSendData(0L, false);
    return (res);
}

/** = = = = = = = = = = = = = = =
 * @brief Set the actual wheel diameter and number of pulses
 * 
 * @param wheel - wheel diameter (in mm)
 * @param pulses - number of holes (or pulses) per revolution
 * = = = = = = = = = = = = = = =
 */
void QuadDecoder::setQuadParams(dist_t wheel, uint32_t pulses)
{
    wheelDiameter = wheel;      
    pulsesPerRev = pulses * 4;  // This is, after all, a QUAD decoder!
    convertPulsesToDist = ((M_PI * wheelDiameter) / pulsesPerRev);
    Serial.print("CONVERT_PULSES factor = "); Serial.println(convertPulsesToDist);
}

/** = = = = = = = = = = = = = = =
 * @brief Get the current Position (mm)
 *     Position is in terms of 'ticks' of the quad encoder
 * we convert it here into the units of choice
 *
 * @return current position
 * = = = = = = = = = = = = = = =
 */
dist_t QuadDecoder::getPosition()
{
    pulse_t position;
    portENTER_CRITICAL(&quad_ctrl_mutex); // thread safe - get position
    position = quadPtr->absPosition;
    portEXIT_CRITICAL(&quad_ctrl_mutex);

    return( position * convertPulsesToDist );
}

/** = = = = = = = = = = = = = = =
 * Reset the position (and quad decoder logic)
 *   (This simply sets the 'QuadInitState', the
 * actual reset happens on the next pulse inside the ISR.
 * = = = = = = = = = = = = = = =
 */
void QuadDecoder::resetPos()
{
     //portENTER_CRITICAL(&quad_ctrl_mutex);    

    myQuadPtr->resetPosition();
    portENTER_CRITICAL(&quad_ctrl_mutex); 
    quadPtr->last_state = QuadInitState;
    quadPtr->absPosition = 0L;
    quadPtr->pulseCount = 0;
    portEXIT_CRITICAL(&quad_ctrl_mutex);

    last_update_time = esp_timer_get_time();
    last_speed = 0.0;

    // portEXIT_CRITICAL(&quad_ctrl_mutex);
    setSpeedCheckInterval(speedCheckIntervaluSec/1000);
}

/** = = = = = = = = = = = = = = =
 *  Desructor - never happens on ESP32
 * = = = = = = = = = = = = = = =
 */
// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the average speed since last call.
 *
 * @return int32_t - The Speed,  in millimeters per msecond.
 */
int32_t QuadDecoder::getSpeed()
{
    double curSpeed = last_speed * (wheelDiameter / pulsesPerRev);
    return (curSpeed);
}

/** = = = = = = = = = = = = = = =
 *  Desructor - never happens on ESP32
 * = = = = = = = = = = = = = = =
 */
// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Decode the command to Get/Set the speed check interval
 *      Format:  QSCK <msecs>
 * @return ProcessStatus 
 */
ProcessStatus QuadDecoder::cmdSetSpeedCheckInterval()
{
    ProcessStatus retVal = SUCCESS_NODATA;

    time_t interv;
    if (argCount==1)
    {
         if (0 != getLLint(0, &interv, "Error in speed check interval ") )
            retVal=FAIL_DATA;
            else
            setSpeedCheckInterval(interv);

    } else if (argCount != 0)
    { 
        sprintf(DataPacket.value, "ERR: Wrong number of arguments");
        retVal = FAIL_DATA;
    } 

    // Echo the result - or report the current
    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "Interval: %LLD", speedCheckIntervaluSec/1000);
        retVal = SUCCESS_DATA;
    }

    defDevSendData(0,false);
    return(retVal);
}


/** - - - - - - - - - - - - - - - - - - - - - --
 * @brief How often do we update the current speed?
 *
 * @param rate - The minimum time interval between speed checks(millisecs)
 */
void QuadDecoder::setSpeedCheckInterval(time_t rateMsec)
{
#ifndef USE_SPEED_CHECK
    return;
#endif
    // Note:  The internal clock is in microseconds
    speedCheckIntervaluSec = rateMsec * 1000;

    // Stop the timer, restart with new period
    esp_timer_stop(spdUpdateTimer);
    
    // Enable with new paramers
    ESP_LOGE(TAG, "Start Time - period is %llu", speedCheckIntervaluSec);
    ESP_ERROR_CHECK(esp_timer_start_periodic(spdUpdateTimer, speedCheckIntervaluSec));
}


/**  - - - - - - - - - - - - - - - - - - - - - --
 * @brief Set the parameters to convert the Quad Encoder ticks to a distance
 *     This generates the 'convertTickToDist' factor.
 * @param tickPerRev   - how many ticks (or marks) per revolution on one channel.
 * @param diameter     - What is the diameter of the wheel (millimeters).
 */
void QuadDecoder::calibrate(pulse_t tickPerRev, dist_t diameter)
{
    pulsesPerRev = tickPerRev * 4; // QUAD encoder - 4 states per pulse cycle
    wheelDiameter = diameter;
    convertPulsesToDist = (wheelDiameter * M_PI) / (pulsesPerRev);
}


/** - - - - - - - - - - - - - - - - - - - - - --
 * @brief Get the current configuration
 *     We store the requested values in the memory pointed to
 * by each of tthe arguments.
 *
 * @param tickPerRev   - how many ticks (or marks) per revolution on one channel.
 * @param diameter     - What is the diameter of the wheel (millimeters).
 */
void QuadDecoder::getCalibration(pulse_t *tickPerRev, dist_t *diameter)
{
    *tickPerRev = pulsesPerRev / 4;
    *diameter = wheelDiameter;
}
