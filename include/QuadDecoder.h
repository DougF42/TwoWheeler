/**
 * @file QuadDecoder.h
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 * Listen to - and decode - a quad decoder
 * 
 * This sets up an interrupt handler that responds
 * to all phase_a and phase_b transitions, and updates
 * our idea of position (TBD: and speed?) accordingly.
 * 
 * We also track the RATE at which the A phase goes high
 * to determine average speed.
 * 
 * When loop is called, it used the elapsed time from
 * the last time it was called, and the difference in
 * position, to determine the speed.
 * 
 * Note: Units are in MM.
 * To get distance, WHEEL_DIAM_MM is used.
 */
#pragma once
#include "config.h"
#include "atomic"
#include "freertos/queue.h"
#include <driver/gpio.h>


 #define MAX_NO_OF_QUAD_DECODERS 2


class QuadDecoder
{
    public:

    private:
        // The order corresponds to binary state assuming a is bit 1, b is bit 0
        typedef enum { AoffBoff=0, AoffBon=1, AonBoff=2,  AonBon=3, QuadInitState=4} QUAD_STATE_t;

        uint8_t quadIdx; // Which motor is assigned to me?
        QueueHandle_t queue;  // keep track of quad events
        gpio_num_t quad_pin_a;
        gpio_num_t quad_pin_b;

        // Robot Characteristics
        int    pulsesPerRev;
        double wheelDiameter; 
        double convertPulsesToDist;    // Calculated: the factor to convert ticks into distance. 

        uint32_t lastLoopTime;
        uint32_t lastPulsesPerSecond; // last rate (pulses per second)
        uint32_t speedCheckIntervaluSec;        // How often we calculate speed (uSeconds)

        static void IRAM_ATTR ISR_handler(void *arg);

    public:
        QuadDecoder();
        ~QuadDecoder();
        void setupQuad(gpio_num_t _quad_pin_a, gpio_num_t _quad_pin_b, bool is_isr_installed=false);
        void quadLoop();
     
        double getPosition();
        void   resetPos(uint32_t newPos=0);
        int32_t getSpeed();
 
        uint32_t getPulseCount();

        void setSpeedCheckInterval(uint32_t rate=SPEED_CHECK_INTERVAL_mSec);
        void calibrate (uint pulsesPerRev, uint circumfrence);  
        void calibrate_raw_pos();

};