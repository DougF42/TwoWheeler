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
#include "esp_timer.h"
#include "atomic"


// - - - - - - - STATIC DEFS - - - - - - - - - - -
/**
 * @brief This keeps track of motor positions
 * 
 * It is static, and kept in DRAM so that the
 * ISR can run without waiting for flash to load
 * 
 */
typedef struct
{
    uint8_t last_state;   // state of the decoder (set by ISR)
    dist_t position;    // position (pulses). ISR increments this...
} Quad_t;

static Quad_t quads[MAX_NO_OF_QUAD_DECODERS]; // (static not const - should be in DRAM )

// For the quad array, what is the next available position?
static std::atomic_uint8_t nextQuadId;

// - - - - - - - - INITIALIZE - - - - - - - - - - - - - --
QuadDecoder::QuadDecoder()
{
    quadIdx=nextQuadId++;    // this is atomic!
    if (quadIdx >= MAX_NO_OF_QUAD_DECODERS)
    {
        quadIdx=-1;
        nextQuadId--;
        // Note: Cant print here - we are inside constructor.
        // Serial.printf("**** ERROR* ****  TOO MANY MOTORS DEFINED. Limit is %d!\r\n", MAX_NO_OF_QUAD_DECODERS);
        // delay(5000);
        return;
    }
 
}

QuadDecoder::~QuadDecoder()
{
    gpio_reset_pin(quad_pin_a);
    gpio_reset_pin(quad_pin_b);
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Set parameters for this Quad decoder
 * 
 */
void QuadDecoder::setupQuad(gpio_num_t _quad_pin_a, gpio_num_t _quad_pin_b,  bool isr_previously_installed)
{
    //  Sanity checks
    if (quadIdx < 0)
    {
        Serial.println("*** ERROR! TOO MANY QUADS - quadIdx not assigned!");
        return;
    }
    quad_pin_a = _quad_pin_a;
    quad_pin_b = _quad_pin_b;

    // force initial values (TBD: Use prefs?)
    setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);
    calibrate(QUAD_PULSES_PER_REV, WHEEL_DIAM_MM);
    quads[quadIdx].last_state =  QuadInitState;

    // CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
    gpio_config_t pGpioConfig=
    {
        .pin_bit_mask = (1ull << quad_pin_a) | (1ull<<quad_pin_b),
        .mode=GPIO_MODE_INPUT,               /*!< GPIO mode: set input/output mode  */
        .pull_up_en=GPIO_PULLUP_ENABLE,       /*!< GPIO pull-up                    */
        .pull_down_en=GPIO_PULLDOWN_DISABLE,   /*!< GPIO pull-down                 */
        .intr_type=GPIO_INTR_ANYEDGE,      /*!< GPIO interrupt type                */
    #if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        .hys_ctrl_mode=GPIO_HYS_SOFT_DISABLE;       /*!< GPIO hysteresis: hysteresis filter on slope input    */
    #endif
    };
    ESP_ERROR_CHECK( gpio_config( &pGpioConfig) );
    ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_a, ISR_handler, this) );    
    ESP_ERROR_CHECK(gpio_isr_handler_add(quad_pin_b, ISR_handler, this) );

    Serial.printf("Quad defined using index %d\r\n", quadIdx);   
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief ISR routine - update our position based on Encodert pulses
 *
 * @param arg - points to the current instance of QuadDecoder
 * 
 */
void IRAM_ATTR QuadDecoder::ISR_handler(void *arg)
{  
    // Who am I?
    QuadDecoder *me = (QuadDecoder *)arg;     // point to my QuadDecoder
    Quad_t *thisQuad = &(quads[me->quadIdx]); // Point to my quad entry

    // save the current state of the pins
    int pina=gpio_get_level(me->quad_pin_a); // get the current value of pinA
    int pinb=gpio_get_level(me->quad_pin_b); // get the current value of pinB
    QUAD_STATE_t newState = (QUAD_STATE_t) ((pina<<1) | pinb); // combine pina+pinb to get state
    
    // DECODE the quad state, update our position
    switch (thisQuad->last_state)
    {
    case (QuadInitState):
        // Initialize the QUAD decoder
        thisQuad->position = 0;
        thisQuad->last_state = newState;
        break;

    case (AonBoff):
        if (newState == AonBon)
        {
            thisQuad->position++;
            thisQuad->last_state = newState;
        } else if (newState == AoffBoff)
        {
            thisQuad->position--;
            thisQuad->last_state = newState;
        }        
        break;

    case (AonBon):
        if (newState == AoffBon)
        {
            thisQuad->position++;
            thisQuad->last_state = newState;
         } else if (newState == AonBoff);
         {
            thisQuad->position--;
            thisQuad->last_state = newState;
        }
        break;

    case (AoffBon):
        if (newState == AoffBoff)
        {
            thisQuad->position ++;
            thisQuad->last_state = newState;
        } else if (newState == AonBon)
        {
            thisQuad->position --;
            thisQuad->last_state = newState;
        }
        break;

    case (AoffBoff):
        if (newState == AonBoff)
        {
            thisQuad->position++;
            thisQuad->last_state = newState;
        }
        else if (newState == AoffBon)
        {
            thisQuad->position++;
            thisQuad->last_state = newState;
            break;
        }
    }
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief This periodically looks to see how far we traveled.
 *  The check performed no more than once per speedCheckIntervaluSec.
 * 
 *   We store the actual interval and distance (number of pulses)
 *  since the last check.  These will be translated to actual speed 
 *  only when the user asks for it via getSpeed();
 * 
 *
 * (20000 uSecs = 20 msecs)
 */
void QuadDecoder::quadLoop()
{
    // Nothing to do..
    return;
}

// - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the current Position
 *     Position is in terms of 'ticks' of the quad encoder
 * we convert it here into the units of choice
 * 
 * @return double current position
 */
dist_t QuadDecoder::getPosition()
{
    noInterrupts();
    dist_t position= quads[quadIdx].position;
    interrupts();
    return( position * convertPulsesToDist);
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
    interrupts();
    lastPosition = 0;
    lastSpeedCheck = esp_timer_get_time();
    interrupts();
}

// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the average speed since last call.
 *   
 * @return int32_t - The Speed,  in millimeters per .
 */
int32_t QuadDecoder::getSpeed()
{
    // lastSpeedCheck 
    // lastPosition
    time_t timeNow = esp_timer_get_time(); // time now (in uSecs)
    noInterrupts();
    dist_t curPos = quads[quadIdx].position;
    interrupts();
    time_t elapsed = timeNow - lastSpeedCheck;
    dist_t dist = lastPosition - curPos;
    dist_t speed = (dist*convertPulsesToDist)/elapsed; // Speed in mm/msec
    Serial.print("SPEED:  elapsed="); Serial.print(elapsed);
    Serial.print("   pulses="); Serial.print(dist); 
    Serial.print("   Dist="); Serial.print(dist*convertPulsesToDist);
    Serial.print("   speed="); Serial.println(speed);

    lastSpeedCheck=timeNow;
    lastPosition=curPos;
    return(speed);
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief How often do we read our current speed?
 *   Note: This is the minimum time between speed checks - if
 * loop() isn't called fast enough, a longer interval
 * between checks will occur.
 * 
 * @param rate - The minimum time interval between speed checks(millisecs)
 */
void QuadDecoder::setSpeedCheckInterval(time_t rate)
{
    speedCheckIntervaluSec=rate;
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Set the parameters to convert the Quad Encoder ticks to a distance
 *     This generates the 'convertTickToDist' factor.
 * @param tickPerRev   - how many ticks (or marks) per revolution on one channel.
 * @param diameter     - What is the diameter of the wheel (millimeters).
 */
void QuadDecoder::calibrate (uint tickPerRev, dist_t diameter)
{
    pulsesPerRev = tickPerRev*4; // QUAD encoder - 4 states per pulse cycle
    wheelDiameter = diameter;
    convertPulsesToDist =   (wheelDiameter*M_PI) / (pulsesPerRev);
    Serial.print("CALBIRATE: pulses to dist:   pulses * "); Serial.println(convertPulsesToDist);
}


/**
 * @brief Get the current configuration
 *     We store the requested values in the memory pointed to 
 * by each of tthe arguments. 
 * 
 * @param tickPerRev   - how many ticks (or marks) per revolution on one channel.
 * @param diameter     - What is the diameter of the wheel (millimeters).
 */
void QuadDecoder::getCalibration(uint *tickPerRev, dist_t *diameter)
{
    *tickPerRev = pulsesPerRev/4;
    *diameter =  wheelDiameter;
}
