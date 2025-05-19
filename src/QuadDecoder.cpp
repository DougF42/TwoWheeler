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
    uint8_t last_state;
    double position;
    uint32_t pulseCount;
} Quad_t;

static Quad_t quads[MAX_NO_OF_QUAD_DECODERS]; // should be in DRAM 

// For the quad array, what is the next available position?
static std::atomic_uint8_t nextQuadId;

// - - - - - - - - INITIALIZE - - - - - - - - - - - - - --
QuadDecoder::QuadDecoder()
{
    quadIdx=nextQuadId++;    // this is atomic!
    if (quadIdx >= MAX_NO_OF_QUAD_DECODERS)
    {
        Serial.printf("**** ERROR* ****  TOO MANY MOTORS DEFINED. Limit is %d!\r\n", MAX_NO_OF_QUAD_DECODERS);
        delay(5000);
        return;
    }

    quads[quadIdx].last_state = QuadInitState;
    quads[quadIdx].position   = 0;
    quads[quadIdx].pulseCount = 0; 

     // everything in the class gets *some* value...
    pulsesPerRev = QUAD_PULSES_PER_REV*4;
    quad_pin_a=GPIO_NUM_NC;
    quad_pin_b=GPIO_NUM_NC;
    convertPulsesToDist      = 0;
    lastLoopTime             = 0;
    lastPulsesPerSecond=0;
    speedCheckIntervaluSec=0;
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
    quad_pin_a = _quad_pin_a;
    quad_pin_b = _quad_pin_b;

    setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);
    calibrate(QUAD_PULSES_PER_REV, 4);
    lastLoopTime= esp_timer_get_time();

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

    gpio_isr_handler_add(quad_pin_a, ISR_handler, this);
    gpio_isr_handler_add(quad_pin_b, ISR_handler, this);
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
    QuadDecoder *me = (QuadDecoder *)arg;     // point to my QuadDecoder
    Quad_t *thisQuad = &(quads[me->quadIdx]); // Point to my quad entry
    
    int pina=gpio_get_level(me->quad_pin_a); // get the current value of pinA
    int pinb=gpio_get_level(me->quad_pin_b); // get the current value of pinB
    QUAD_STATE_t newState = (QUAD_STATE_t) ((pina<<1) | pinb);

    thisQuad->pulseCount++;
    switch (thisQuad->last_state)
    {
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
 * @brief This is where we determine our speed
 * Call this frequently!
 * 
 *    Speed is determined once every speedCheckIntervaluSec,
 *  which defaults to SPEED_CHECK_INTERVAL_uSec
 *
 */
#define CHECK_INTERVAL_uSec 1000
void QuadDecoder::quadLoop()
{
    uint32_t timeNow = esp_timer_get_time();
    
    uint32_t elapsedTime = lastLoopTime - timeNow;
    if (elapsedTime > speedCheckIntervaluSec)
    {
        lastPulsesPerSecond = quads[quadIdx].pulseCount / elapsedTime; // ticks per microsecond
        quads[quadIdx].pulseCount = 0;
        lastLoopTime = timeNow;
    }
    return;
}

// - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the current Position
 *     Position is in terms of 'ticks' of the quad encoder
 * we convert it here into the units of choice
 * @return double current position
 */
double QuadDecoder::getPosition()
{
    return( quads[quadIdx].position * convertPulsesToDist);
}


// - - - - - - - - - - - - - - - - - - - - - -
// Reset the position (and quad decoder logic)
//   (note: this also resets the quad state)

// Param: newPos - Set the current position
void QuadDecoder::resetPos(uint32_t newPos)
{
    Quad_t *myquad = & quads[quadIdx];        
    myquad->last_state = QuadInitState;
    myquad->position=newPos;
    myquad->pulseCount=0;    
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Get the latest Speed
 *    The units will match the value set by 'calibrate'.
 * 
 * @return int32_t - The Speed,  in millimeters per .
 */
int32_t QuadDecoder::getSpeed()
{
    return(lastPulsesPerSecond * convertPulsesToDist);
}


// - - - - - - - - - - - - - - - - - - - - - --
uint32_t QuadDecoder::getPulseCount()
{
    return( quads[quadIdx].pulseCount);
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
void QuadDecoder::setSpeedCheckInterval(uint32_t rate)
{
    speedCheckIntervaluSec=rate;
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Set the parameters to convert the Quad Encoder ticks to a distance
 *     This generates the 'convertTickToDist' factor.
 * @param tickPerRev   - how many ticks (or marks) per revolution on one channel.
 * @param diameter     - What is the diameter of the wheel (millimeters).
 * @param _units       - what units should we use (UNITS_MM or UNITS_IN)
 */
void QuadDecoder::calibrate (uint tickPerRev, uint diameter)
{
    convertPulsesToDist = (diameter*M_PI) / (tickPerRev*4.0);
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief Set configuration so that dist and pulses are '1'.
 *   (This is really only usefull for testing)
 */
void QuadDecoder::calibrate_raw_pos()
{
    convertPulsesToDist=1;    
}