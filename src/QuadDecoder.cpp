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

#include "atomic"

// This MUST be kept as small as possible - it determines
// the size of a structure that is stored in DRAM for the
// ISR routines will work properly.
 #define MAX_NO_OF_QUAD_DECODERS 2


// - - - - - - - STATIC DEFS - - - - - - - - - - -
/**
 * @brief This keeps track of motor positions
 * 
 * It is static, and kept in DRAM so that the
 * ISR can run without waiting for flash to load -
 * we must keep it short!
 */
typedef struct
{
    QuadDecoder::QUAD_STATE_t last_state;   // state of the decoder (set by ISR)
    pulse_t position;    // Current position (pulses). ISR increments this...
    double speed;        // how fast am I going?
    dist_t prev_position;               // The previous position (for determining speed)
    time_t  last_update_time;           // The last time we updated the speed
} Quad_t;


static DRAM_ATTR Quad_t quads[MAX_NO_OF_QUAD_DECODERS]; // (static not const - should be in DRAM )
// For the quad array, what is the next available position?
static std::atomic_uint8_t nextQuadId;


// - - - - - - - - INITIALIZE - - - - - - - - - - - - - --
QuadDecoder::QuadDecoder()
{        
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
void QuadDecoder::setupQuad(gpio_num_t _quad_pin_a, gpio_num_t _quad_pin_b)
{
    //  Sanity checks
    int quadIdx = nextQuadId++;
    if (quadIdx > MAX_NO_OF_QUAD_DECODERS)
    {
        Serial.println("*** ERROR! TOO MANY QUADS - quadIdx not assigned!");
        return;
    }
    quad_pin_a = _quad_pin_a;
    quad_pin_b = _quad_pin_b;

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
    .name="SpeedTimer", //!< Timer name, used in esp_timer_dump() function
    .skip_unhandled_events = true    //!< Setting to skip unhandled events in light sleep for periodic timers
};

ESP_ERROR_CHECK(esp_timer_create(&speed_timer_args, &spdUpdateTimer));
setSpeedCheckInterval(SPEED_CHECK_INTERVAL_mSec);

return;
}

// - - - - - - - - - - - - - - - - - - - - - --
/**
 * @brief ISR routine - update our position based on Encoder pulses
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
         } else if (newState == AonBoff);
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
 *    Driven from speed_timer as an ISR (not 
 * task dispatch?), this determines our speed,
 * based on the change in position since the 
 * last call, and the time period since that call.
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
    qptr->speed = (qptr->prev_position - qptr->position) / (esp_timer_get_time() - qptr->last_update_time);
    qptr->last_update_time = esp_timer_get_time();
    qptr->prev_position = qptr->position;
    return;
}


// - - - - - - - - - - - - - - - - - - - - - --
/**
 * 
 *
 * (20000 uSecs = 20 msecs)
 */
#ifdef NOTTHEDROIDSYOUARELOOKINGFOR
void QuadDecoder::quadLoop()
{
    // Nothing to do..
    return;
}
#endif 


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
    noInterrupts();  // thread safe - get position
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
    quads[quadIdx].position  =0;
    quads[quadIdx].prev_position = 0;
    quads[quadIdx].last_update_time = esp_timer_get_time();

    interrupts();
    setSpeedCheckInterval(speedCheckIntervaluSec*1000); 

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
    curSpeed = curSpeed * (wheelDiameter/ pulsesPerRev);
    return(curSpeed);
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
    speedCheckIntervaluSec=rateMsec*1000;
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
void QuadDecoder::calibrate (pulse_t tickPerRev, dist_t diameter)
{
    pulsesPerRev = tickPerRev*4; // QUAD encoder - 4 states per pulse cycle
    wheelDiameter = diameter;
    convertPulsesToDist =   (wheelDiameter*M_PI) / (pulsesPerRev);
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
    *tickPerRev = pulsesPerRev/4;
    *diameter =  wheelDiameter;
}
