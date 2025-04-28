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
#include "MotorDefs.h"
#include "esp_timer.h"

#ifndef MTR 
#define MTR MotorTable[motorIdx]
#endif

QuadDecoder::QuadDecoder()
{
    motorIdx=-1; 
}


/**
 * @brief Set parameters for this Quad decoder
 * 
 * @param motor_index 
 */
void QuadDecoder::setup(int motor_index)
{
    motorIdx = motor_index;
    last_state = AoffBoff;
    pulseCount=0;
    position = 0;
    setSpeedCheckInterval(SPEED_CHECK_INTERVAL_uSec);
    calibrate(QUAD_PULSES_PER_REV, 4, UNITS_IN);
    lastLoopTime= esp_timer_get_time();

    // TODO: CONFIGURE QUAD PINS FOR INPUT WITH INTERRUPT
    gpio_config_t pGpioConfig=
    {
        .pin_bit_mask = (1ull << MTR.quad_pin_a) | (1ull<<MTR.quad_pin_b),
        .mode=GPIO_MODE_INPUT,               /*!< GPIO mode: set input/output mode  */
        .pull_up_en=GPIO_PULLUP_ENABLE,       /*!< GPIO pull-up                    */
        .pull_down_en=GPIO_PULLDOWN_DISABLE,   /*!< GPIO pull-down                 */
        .intr_type=GPIO_INTR_ANYEDGE,      /*!< GPIO interrupt type                */
    #if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        .hys_ctrl_mode=GPIO_HYS_SOFT_DISABLE;       /*!< GPIO hysteresis: hysteresis filter on slope input    */
    #endif
    };

    gpio_config( &pGpioConfig);
    ESP_ERROR_CHECK (gpio_install_isr_service(ESP_INTR_FLAG_LEVEL4) );
    gpio_isr_handler_add(MTR.quad_pin_a, ISR_handlePhaseA, this);
    gpio_isr_handler_add(MTR.quad_pin_b, ISR_handlePhaseB, this);

}


QuadDecoder::~QuadDecoder()
{
    gpio_uninstall_isr_service();
    gpio_reset_pin(MTR.quad_pin_a);
    gpio_reset_pin(MTR.quad_pin_b);

}

/**
 * @brief Phase A changed.
 *
 * @param arg - points to the current instance of QuadDecoder
 */
void QuadDecoder::ISR_handlePhaseA(void *arg)
{
    QuadDecoder *me = (QuadDecoder *)arg;
    me->pulseCount++;
    switch(me->last_state)
    {
        case(AoffBoff):
            me->position++;
            me->last_state=AonBoff;
        break;

        case(AonBoff):
            me->position--;
            me->last_state=AoffBoff;
        break;

        case(AoffBon):
        me->position++;
        me->last_state=AonBon;
        break;

        case(AonBon):
        me->position--;
        me->last_state=AoffBon;
        break;
    }
}


/**
 * @brief Phase B changed
 * 
 * @param arg - points to the current instance of QuadDecoder
 */
void QuadDecoder::ISR_handlePhaseB(void *arg)
{
    QuadDecoder *me = (QuadDecoder *)arg;
    me->pulseCount++;
    switch(me->last_state)
    {
        case(AoffBoff):
            me->position--;
            me->last_state=AoffBon;
        break;

        case(AonBoff):
            me->position++;
            me->last_state=AonBon;
        break;

        case(AoffBon):
        me->position++;
        me->last_state=AoffBoff;
        break;

        case(AonBon):
        me->position--;
        me->last_state=AonBoff;
        break;
    }

}


/**
 * @brief This is where we determine our speed
 * Call this frequently!
 *    Speed is determined once every speedCheckIntervaluSec,
 *  which defaults to SPEED_CHECK_INTERVAL_uSec
 *
 */
#define CHECK_INTERVAL_uSec 1000
void QuadDecoder::loop()
{
    uint32_t timeNow = esp_timer_get_time();
    uint32_t elapsedTime = lastLoopTime - timeNow;
    if (elapsedTime > speedCheckIntervaluSec)
    {
        lastTicksPerSecond = pulseCount / elapsedTime; // ticks per microsecond
        pulseCount = 0;
        lastLoopTime = timeNow;
    }
    return;
}


/**
 * @brief Get the latest Speed
 *    The units will match the value set by 'calibrate'.
 * 
 * @return int32_t 
 */
int32_t QuadDecoder::getSpeed()
{
    return(lastTicksPerSecond * convertTickToDist);
}

/**
 * @brief Get the current Position
 *     Position is in terms of 'ticks' of the quad encoder
 * we convert it here into the units of choice
 * @return double current position
 */
double QuadDecoder::getPosition()
{
    return(position * convertTickToDist);
}

/**
 * @brief How often do we read our current speed?
 *   Note: This is the minimum time between speed checks - if
 * loop() isn't called fast enough, a longer interval
 * between checks will occur.
 * 
 * @param rate - The minimum time interval between speed checks.
 */
void QuadDecoder::setSpeedCheckInterval(uint32_t rate)
{
    speedCheckIntervaluSec=rate;
}


/**
 * @brief Set the parameters to convert the Quad Encoder ticks to a distance
 *     This generates the 'convertTickToDist' factor.
 * @param tickPerRev   - how many ticks (or marks) per revolution on one channel.
 * @param diameter     - What is the diameter of the wheel.
 * @param _units       - what units should we use (UNITS_MM or UNITS_IN)
 */
void QuadDecoder::calibrate (uint tickPerRev, uint diameter, QuadUnits_t _units)
{
    units = _units;
    convertTickToDist = (diameter*M_PI) / (tickPerRev*4.0);
}

