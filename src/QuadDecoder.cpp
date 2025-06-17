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

// This MUST be kept as small as possible - it determines
// the size of a structure that is stored in DRAM for the
// ISR routines will work properly.
 #define MAX_NO_OF_QUAD_DECODERS 2


// - - - - - - - STATIC DEFS - - - - - - - - - - -
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
    pulse_t position;    // change in Current position since last speed check.  (ISR increments this...)
    dist_t  speed;        // how fast am I going?
    dist_t  absPosition; // my absolute position
    
    time_t  last_update_time;           // The last time we updated the speed
} Quad_t;

static DRAM_ATTR Quad_t quads[MAX_NO_OF_QUAD_DECODERS]; // (static not const - should be in DRAM )

// For the quad array, what is the next available position?
static std::atomic_uint8_t nextQuadId=0;



QuadDecoder::QuadDecoder(Node *_node, const char * InName) : DefDevice(_node, InName)
{
    reportEnableFlag = false;
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
    //  Sanity checks
    int quadIdx = nextQuadId++;
    if (quadIdx > MAX_NO_OF_QUAD_DECODERS)
    {
        Serial.println("*** ERROR! TOO MANY QUADS - quadIdx not assigned!");
        return;
    }
    quad_pin_a =  cfg->dir_pin_a;
    quad_pin_b =  cfg->dir_pin_b;

    calibrate(QUAD_PULSES_PER_REV, WHEEL_DIAM_MM);
    quads[quadIdx].last_state = QuadInitState;

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
ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE  ));
ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_a, ISR_handler, this));
ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_b, ISR_handler, this));

Serial.printf("Quad defined using index %d\r\n", quadIdx);

// NOW SET UP THE SPEED CHECK TIMER
esp_timer_create_args_t speed_timer_args = 
{
    .callback = &update_speed_ISR,    //!< Callback function to execute when timer expires
    .arg =this,                       //!< Argument to pass to callback
    .dispatch_method=ESP_TIMER_TASK,  //!< Dispatch callback from task or ISR; if not specified, esp_timer task
    //                                !< is used; for ISR to work, also set Kconfig option
    //                                !< `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`
    .name="SpeedTimer",               //!< Timer name, used in esp_timer_dump() function
    .skip_unhandled_events = true     //!< Setting to skip unhandled events in light sleep for periodic timers
};

ESP_ERROR_CHECK(esp_timer_create(&speed_timer_args, &spdUpdateTimer));
setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);
resetPos();

return;
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
    Quad_t *qptr = &(quads[me->quadIdx]); // Point to my quad entry

    // save the current state of the pins
    int pina=gpio_get_level(me->quad_pin_a); // get the current value of pinA
    int pinb=gpio_get_level(me->quad_pin_b); // get the current value of pinB
    QUAD_STATE_t newState = (QUAD_STATE_t) ((pina<<1) | pinb); // combine pina+pinb to get state
    
    // DECODE the quad state, update our position
    switch (qptr->last_state)
    {
    case (QuadInitState):
        // Initialize the QUAD decoder
        qptr->position = 0;
        qptr->last_state = newState;
        break;

    case (AonBoff):
        if (newState == AonBon)
        {
            qptr->position++;
            qptr->last_state = newState;
        } else if (newState == AoffBoff)
        {
            qptr->position--;
            qptr->last_state = newState;
        }        
        break;

    case (AonBon):
        if (newState == AoffBon)
        {
            qptr->position++;
            qptr->last_state = newState;
         } else if (newState == AonBoff)
         {
            qptr->position--;
            qptr->last_state = newState;
        }
        break;

    case (AoffBon):
        if (newState == AoffBoff)
        {
            qptr->position ++;
            qptr->last_state = newState;
        } else if (newState == AonBon)
        {
            qptr->position --;
            qptr->last_state = newState;
        }
        break;

    case (AoffBoff):
        if (newState == AonBoff)
        {
            qptr->position++;
            qptr->last_state = newState;
        }
        else if (newState == AoffBon)
        {
            qptr->position++;
            qptr->last_state = newState;
            break;
        }
    }
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
 * @param arg - points to the 'quadDecoder' instance
 *   that this was called from.
 */
void QuadDecoder::update_speed_ISR(void *arg)
{
    QuadDecoder *me=(QuadDecoder *)arg;
    Quad_t *qptr = &(quads[me->quadIdx]); // Point to my quad entry
    qptr->speed = (qptr->position) / (esp_timer_get_time() - qptr->last_update_time);
    qptr->last_update_time = esp_timer_get_time();
    qptr->position = 0;
    return;
}


/**
 * @brief Run this periodically...
 * 
 * Output the current position and speed for this motor
 */
ProcessStatus QuadDecoder::DoPeriodic()
{
    if (! reportEnableFlag) return(FAIL_NODATA);  // no reports

    Quad_t *qptr = &(quads[quadIdx]); // Point to my quad entry
    noInterrupts();
    dist_t pos= qptr->absPosition;
    dist_t spd=qptr->speed;
    interrupts();

    sprintf(DataPacket.value, "QUAD,%f, %ld", qptr->speed, qptr->position);
    DataPacket.timestamp = millis();
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
    ProcessStatus retVal = SUCCESS_DATA;
    dist_t wheelDia;
    uint32_t pulseCnt;
    retVal = Device::ExecuteCommand();
    if (retVal != NOT_HANDLED )  return(retVal);

    scanParam();
    if (isCommand("QSET")) 
    {   // set wheel dia and pulses. 
        retVal = cmdQSET();
    }
    else if (isCommand("QRPT"))
    {   // enable reporting position
        retVal = cmdQRPT();
    } else if (isCommand("QRST"))
    {
        resetPos();
        retVal = SUCCESS_NODATA;
    } else if (isCommand("QSCK"))
    {
        retVal = cmdSetSpeedCheckInterval();        
    }

    return (retVal);
}


/**
 * @brief Decode the command to Set the Wheel diameter and pulses per rev
 *    format:   QSET|<wheelDia>|<PulsePerRev>   -set these params
 *    format:   QSET                            -- just report params
 * 
 * @return - we always return the current parameters
 */
ProcessStatus QuadDecoder::cmdQSET()
{
    ProcessStatus res=SUCCESS_NODATA;

    if (argCount == 2)
    {
        double wheel;
        uint32_t pulses;
        if (0 != getDouble(0, &wheel, "Wheel Diameter: "))
        {
            res = FAIL_DATA;

        } else if (0 != getUint32(1, &pulses, "Pulse Count: "))
        {

            res = FAIL_DATA;
        }
        else
        {
            setQuadParams(wheel, pulses);
            res = SUCCESS_NODATA;
        }
        
    } else if (argCount != 0)  {
        sprintf(DataPacket.value, "ERRR| wrong number of arguments");
        res = FAIL_DATA;
    }

    if (res == SUCCESS_NODATA)
    {
        // Show the current parameters
        sprintf(DataPacket.value, "QPARM|%f|%d", getPosition(), pulsesPerRev / 4);
        res = SUCCESS_DATA;
    }

    defDevSendData( 0L,false);
    return (res);
}


/**
 * @brief Set the actual wheel diameter and number of pulses
 * 
 * @param wheel 
 * @param pulses 
 */
void QuadDecoder::setQuadParams(dist_t wheel, uint32_t pulses)
{
    wheelDiameter = wheel * 4;
    pulsesPerRev = pulses;
    convertPulsesToDist = ((M_PI * wheelDiameter) / pulsesPerRev);
}

/**
 * @brief decode and implement command to Enable/disable report
 *   Format:   QRPT|on  (turn on reports)
 *             QRPT|off   (turn off reports)
 * @return ProcessStatus
 */
ProcessStatus QuadDecoder::cmdQRPT()
{
    ProcessStatus retVal=SUCCESS_NODATA;
    scanParam();
    if (argCount==1)
    {   // Set the 
        if (0==strcasecmp(arglist[0], "YES"))
            reportEnableFlag=true;
        else if (0==strcasecmp(arglist[0], "NO"))
            reportEnableFlag=false;
        else
        {
            sprintf(DataPacket.value, "ERROR in QRPT command: DataPacket.value must be YES or NO");
            retVal=FAIL_DATA;
        }
    }

    if (retVal != FAIL_DATA)
    {
        sprintf(DataPacket.value, "Reporting is %s", (reportEnableFlag)? "YES":"NO");
    }
    defDevSendData(0, false);
    return (retVal);
}

// - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the current Position
 *     Position is in terms of 'ticks' of the quad encoder
 * we convert it here into the units of choice
 *
 * @return current position
 */
dist_t QuadDecoder::getPosition()
{
    double result;
    noInterrupts(); // thread safe - get position
    dist_t position = quads[quadIdx].position;
    interrupts();
    result = position * convertPulsesToDist;
    return (result);
}

// - - - - - - - - - - - - - - - - - - - - - -
// Reset the position (and quad decoder logic)
//   (This simply sets the 'QuadInitState', the
// actual reset happens on the next pulse inside the ISR.
//
void QuadDecoder::resetPos()
{
    noInterrupts();

    quads[quadIdx].last_state = QuadInitState;
    quads[quadIdx].position = 0;
    quads[quadIdx].absPosition = 0;
    quads[quadIdx].last_update_time = esp_timer_get_time();
    quads[quadIdx].speed = 0;

        interrupts();
    setSpeedCheckInterval(speedCheckIntervaluSec * 1000);
}

// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the average speed since last call.
 *
 * @return int32_t - The Speed,  in millimeters per msecond.
 */
int32_t QuadDecoder::getSpeed()
{
    noInterrupts();
    double curSpeed = quads[quadIdx].speed;
    interrupts();
    curSpeed = curSpeed * (wheelDiameter / pulsesPerRev);
    return (curSpeed);
}

// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Decode the command to Get/Set the speed check interval
 * 
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

    } if (argCount!=0)
    {
        sprintf(DataPacket.value, "ERR: Wrong number of arguments");
        retVal = FAIL_DATA;
    }

    if (retVal == SUCCESS_NODATA)
        defDevSendData(0, false);
    return(retVal);
}
// - - - - - - - - - - - - - - - - - - - - - --
/**
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
    ESP_ERROR_CHECK(esp_timer_start_periodic(spdUpdateTimer, speedCheckIntervaluSec));
}

// - - - - - - - - - - - - - - - - - - - - - --
/**
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

/**
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
