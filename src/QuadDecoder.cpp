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
#include <atomic>

// For logging
//#define DEBUG_QUAD 1
#ifdef DEBUG_QUAD
#include "esp_log.h"
// esp_log_level_set( TAG, ESP_LOG_VERBOSE);
// ESP_DRAM_LOGE(TAG,"format", vars);
static const char *TAG="QuadDecoder:";
#endif

// This MUST be kept as small as possible - it determines
// the size of a structure that is stored in DRAM for the
// ISR routines will work properly.
 #define MAX_NO_OF_QUAD_DECODERS 2


// - - - - - - - STATIC DEFS - - - - - - - - - - -
bool QuadDecoder::isrAlreadyInstalled=false;  // a static definition

/**
 * @brief This keeps track of motor positions
 * 
 * These variables are used by the ISR routines,
 * and so they must be available - static, and in
 * PSRAM (RAM?) memory. Since the memoryy is a 
 * precious resource, these are NOT part of the 
 * class, so they can be stored separatly.
 * 
 * This needs to be an array, because there are two QuadDecoder
 * instances which must be kept separate!
 * 
 */
typedef struct
{
    QuadDecoder::QUAD_STATE_t last_state;   // state of the decoder (set by ISR)
    pulse_t absPosition;    // change in Current position (in pulses)
    dist_t  speed;        // how fast am I going?
} Quad_t;
static DRAM_ATTR Quad_t quads[MAX_NO_OF_QUAD_DECODERS]; // (static but not const - should be in DRAM )

// For the quad array, what is the next available position?
static std::atomic_uint8_t nextQuadId=0;

 // To avoid problems with overlaping interrupts and 
 //   mainline code... This is used by portEnter/Exit Critical section
 //  to control access to the QUADS structure.
 // NOTE: We expect only one thread to access a given instance of
 //   the decoder at a time.
 static portMUX_TYPE my_mutex= portMUX_INITIALIZER_UNLOCKED;


/** - - - - - - - 
 *  Initialize
 */
QuadDecoder::QuadDecoder(Node *_node, const char * InName) : DefDevice(_node, InName)
{
    pulseCount=0;
    speedUpdateCount=0;
    last_position=0;
    last_update_time=esp_timer_get_time();
    #ifdef DEBUG_QUAD
     esp_log_level_set( TAG, ESP_LOG_VERBOSE);
    #endif
    return;
}


// Never happens on the ESP32
QuadDecoder::~QuadDecoder()
{
    return;
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Set parameters for this Quad decoder
 * 
 */
void QuadDecoder::setupQuad(MotorControl_config_t *cfg)
{
    //  Sanity checks, assign quad index number
    int quadIdx = nextQuadId++;
    if (quadIdx > MAX_NO_OF_QUAD_DECODERS)
    {
        Serial.println("*** ERROR! TOO MANY QUADS - quadIdx not assigned!");
        return;
    }
    Serial.printf("*** QUAD ASSIGNED TO index %d\n\r", quadIdx);
    quad_pin_a = cfg->quad_pin_a;
    quad_pin_b = cfg->quad_pin_b;

    // Set default pulse count and wheel diam
    calibrate(QUAD_PULSES_PER_REV, WHEEL_DIAM_MM);
    quads[quadIdx].last_state = QuadInitState;

    // - - - - - -
    // CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
    gpio_config_t pGpioConfig =
    {
        .pin_bit_mask = (1ull << quad_pin_a) | (1ull << quad_pin_b),
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
        Serial.println("GPIO ISR Service installed ");
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_a, ISR_handler, this));
    ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_b, ISR_handler, this));
    Serial.printf("... Quad ISR handlers added\r\n");

    // - - - - - - - - - - 
    // NOW SET UP THE SPEED CHECK TIMER
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
    Serial.printf("... Speed timer created\n\r");

    setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);
    Serial.printf("... Interval is %d (mseconds)n\r", SPEED_CHECK_INTERVAL_mSec);
    resetPos();
    periodicEnabled = false;  // Default is no report.

    // DEBUG 
    gpio_dump_io_configuration(stdout, (1ull << quad_pin_a) | (1ull << quad_pin_b));
    return;
    }

const char *QuadDecoder::translate(QUAD_STATE_t state)
{
    const char *res = nullptr;
    switch (state)
    {
    case AoffBoff:
        res = "AoffBoff";
        break;

    case AoffBon:
        res = "AoffBon";
        break;

    case AonBoff:
        res = "AonBoff";
        break;

    case AonBon:
        res = "AonBon";
        break;

        case QuadInitState:
        res = "QuadInitState ";
        break;

    default:
        res = "UNKN STATE";
    }
    return (res);
}

// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief ISR routine - update our position when a pulse happens
 *
 * @param arg - points to the current instance of QuadDecoder
 * 
 */
void IRAM_ATTR QuadDecoder::ISR_handler(void *arg)
{  
    // Who am I?
    QuadDecoder *me = (QuadDecoder *)arg;     // point to my QuadDecoder
    me->pulseCount = me->pulseCount+1;   // STATISTICS...
    Quad_t *qptr = &(quads[me->quadIdx]); // Point to my quad entry

    // save the current state of the pins
    int pina=gpio_get_level(me->quad_pin_a); // get the current value of pinA
    int pinb=gpio_get_level(me->quad_pin_b); // get the current value of pinB
    QUAD_STATE_t newState = (QUAD_STATE_t) ((pina<<1) | pinb); // combine pina+pinb to get state

    portENTER_CRITICAL_ISR(&my_mutex);
    // DECODE the quad state, update our position
    switch (qptr->last_state)
    {
    case (QuadInitState):
        // Initialize the QUAD decoder
        qptr->last_state = newState;
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
    }
    portEXIT_CRITICAL_ISR(&my_mutex);
}


/**
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
 * NOTE: Contrary to the name, this is NOT called as
 *       an ISR server - it is called from the high-priority
 *       timer task.
 * 
 * @param arg - points to the 'quadDecoder' instance
 *   that this was called from.
 */
void QuadDecoder::update_speed_CB(void *arg)
{
    QuadDecoder *me=(QuadDecoder *)arg;    
    Quad_t *qptr = &(quads[me->quadIdx]); // Point to my quad entry
    me->speedUpdateCount = me->speedUpdateCount+1;  // statistics..
    pulse_t cur_pos;

    portENTER_CRITICAL_ISR(&my_mutex);
    cur_pos=qptr->absPosition;
    portEXIT_CRITICAL_ISR(&my_mutex);

    qptr->speed          = ((double)cur_pos) / ((double)(esp_timer_get_time() - me->last_update_time));
    me->speed *= me->convertPulsesToDist;
    me->last_update_time = esp_timer_get_time();
    me->last_position    = cur_pos;
 
    return;
}


/**
 * @brief Run this periodically...
 * 
 * Output the current position and speed for this motor
 */
ProcessStatus QuadDecoder::DoPeriodic()
{
    Quad_t *qptr = &(quads[quadIdx]); // Point to my quad entry
    portENTER_CRITICAL(&my_mutex);
    dist_t pos= qptr->absPosition;
    portEXIT_CRITICAL(&my_mutex);

    sprintf(DataPacket.value, "XQUD spd=%f, absPos=%f, ISR=%lu SPD=%lu", 
            speed, getPosition(), pulseCount, speedUpdateCount);
    defDevSendData(millis(), false);
    return (SUCCESS_DATA);
}


/**
 * @brief Decode commands for the QUAD decoder
 * 
 * Commands:
 *      QSET- set <wheel diameter> and <number of pulses per rotation>
 *  QSTA <ON..OFF>     enable/disable report
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
        retVal = cmdQSET();

    } else if (isCommand("QRST"))
    {  // Reset position
        resetPos();
        retVal=SUCCESS_NODATA;

    } else if (isCommand("QSCK"))
    {  // Set the speed Check Interval
        retVal = cmdSetSpeedCheckInterval();        
    }

    return (retVal);
}


/**
 * @brief Decode the command to Set the Wheel diameter and pulses per rev
 *    format:   QSET|<wheelDia>|<pulseCnt>   -set these params
 *    format:   QSET                         - just report params
 * 
 *  NOTE: Whatever units wheelDia are in, thats the units used to report distance!
 * 
 * @return - we always return the current parameters
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
        sprintf(DataPacket.value, "QSET|%f|%d", wheelDiameter, pulsesPerRev / 4);
        res = SUCCESS_DATA;
    }

    defDevSendData(0L, false);
    return (res);
}


/**
 * @brief Set the actual wheel diameter and number of pulses
 * 
 * @param wheel - wheel diameter.
 * @param pulses - number of holes (or pulses) per revolution
 */
void QuadDecoder::setQuadParams(dist_t wheel, uint32_t pulses)
{
    wheelDiameter = wheel;      
    pulsesPerRev = pulses * 4;  // This is, after all, a QUAD decoder!
    convertPulsesToDist = ((M_PI * wheelDiameter) / pulsesPerRev);
    Serial.print("CONVERT_PULSES factor = "); Serial.println(convertPulsesToDist);
}


/** - - - - - - - - - - - - - - - - - --
 * @brief Get the current Position
 *     Position is in terms of 'ticks' of the quad encoder
 * we convert it here into the units of choice
 *
 * @return current position
 */
dist_t QuadDecoder::getPosition()
{
    pulse_t position;

    portENTER_CRITICAL(&my_mutex); // thread safe - get position
    position = quads[quadIdx].absPosition;
    portEXIT_CRITICAL(&my_mutex);

    return( position * convertPulsesToDist );
}


// - - - - - - - - - - - - - - - - - - - - - -
// Reset the position (and quad decoder logic)
//   (This simply sets the 'QuadInitState', the
// actual reset happens on the next pulse inside the ISR.
//
void QuadDecoder::resetPos()
{
     portENTER_CRITICAL(&my_mutex);

    quads[quadIdx].last_state = QuadInitState;
    last_position = 0L;
    quads[quadIdx].absPosition = 0L;
    last_update_time = esp_timer_get_time();
    quads[quadIdx].speed = 0.0;

     portEXIT_CRITICAL(&my_mutex);
    setSpeedCheckInterval(speedCheckIntervaluSec/1000);
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the average speed since last call.
 *
 * @return int32_t - The Speed,  in millimeters per msecond.
 */
int32_t QuadDecoder::getSpeed()
{
    portENTER_CRITICAL(&my_mutex);
    double curSpeed = quads[quadIdx].speed;
    portEXIT_CRITICAL(&my_mutex);
    curSpeed = curSpeed * (wheelDiameter / pulsesPerRev);
    return (curSpeed);
}


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
    // Note:  The internal clock is in microseconds
    speedCheckIntervaluSec = rateMsec * 1000;
    // Stop the timer, restart with new period
    esp_timer_stop(spdUpdateTimer);
    
    // Enable with new paramers
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
