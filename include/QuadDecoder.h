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
#include "Arduino.h"
#include "atomic"
#include "freertos/queue.h"
#include <driver/gpio.h>


 #define MAX_NO_OF_QUAD_DECODERS 2


class QuadDecoder
{
    public:
        // The order corresponds to binary state assuming a is bit 1, b is bit 0
        typedef enum { AoffBoff=0, AoffBon=1, AonBoff=2,  AonBon=3, QuadInitState=4} QUAD_STATE_t;

    private:
        uint8_t quadIdx; // Which motor is assigned to me?
        QueueHandle_t queue;  // keep track of quad events
        gpio_num_t quad_pin_a;
        gpio_num_t quad_pin_b;

        // Robot Characteristics (Configuration, one-time calculations)
        pulse_t pulsesPerRev;           // 
        dist_t  wheelDiameter;          // in MM.
        time_t speedCheckIntervaluSec;  // Config: Min rate we store speed (uSeconds) values.
        dist_t convertPulsesToDist;    // Calculated: the factor to convert ticks into mm. 
        

        // LOOP is used to calculate time interval and speed.
        time_t   lastSpeedCheck;       // When last loop happened (uSecs)
        pulse_t  lastPosition;          // position (pulses)

      //  time_t   elapsedTime;          // How long since prev loop (uSecs)
        pulse_t  distPerLoop;          // dist (pulses) traveled since prev loop

        static void IRAM_ATTR ISR_handler(void *arg); // Interrupt handler

    public:
        QuadDecoder();
        ~QuadDecoder();
        void setupQuad(gpio_num_t _quad_pin_a, gpio_num_t _quad_pin_b);
        //  void quadLoop();   nothing to do..
     
        dist_t getPosition();
        void   resetPos();
        int32_t getSpeed();
 
        void setSpeedCheckInterval(time_t rate=SPEED_CHECK_INTERVAL_mSec);
        void calibrate (pulse_t pulsesPerRev, dist_t diameter);  
        void getCalibration(pulse_t *pulsesPerRev, dist_t *diameter);


};