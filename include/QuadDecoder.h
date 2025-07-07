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
 * A periodic 'high resolution timer' is also implemented
 * which will update our speed (based on the change of 
 * position in the interval since the previous call).
 * 
 * 
 * Note: Units are in MM.
 * To get distance, WHEEL_DIAM_MM is used.
 */
#pragma once
#include "DefDevice.h"
#include "config.h"
#include "Arduino.h"
#include <driver/gpio.h>
#include "esp_timer.h"
#include "sdkconfig.h"
// #include ""
class QuadDecoder: public DefDevice
{
    public:
        // The order corresponds to binary state assuming a is bit 1, b is bit 0
        typedef enum { AoffBoff=0, AoffBon=1, AonBoff=2,  AonBon=3, QuadInitState=4} QUAD_STATE_t;

        QuadDecoder(Node *_node, const char * InName);
        ~QuadDecoder();
        void setupQuad(MotorControl_config_t *cfg);
        void setQuadParams(dist_t wheel, uint32_t pulses);
        dist_t  getPosition();
        void    resetPos();
        int32_t getSpeed();
        ProcessStatus  DoPeriodic() override;              // Override this method to periodically send reports
        ProcessStatus  ExecuteCommand () override;         // Override this method to handle custom commands
        ProcessStatus cmdQSET();
        ProcessStatus cmdSetSpeedCheckInterval();


        // virtual ProcessStatus  DoImmediate() override;  // Override this method for processing your device continuously
        void setSpeedCheckInterval(time_t rate=SPEED_CHECK_INTERVAL_mSec);
        void calibrate (pulse_t pulsesPerRev, dist_t diameter);  
        void getCalibration(pulse_t *pulsesPerRev, dist_t *diameter);
        
    private:
        const char *translate(QUAD_STATE_t state); // for debugging
        uint8_t quadIdx; // Which motor is assigned to me?
        // QueueHandle_t queue;  // keep track of quad events
        gpio_num_t quad_pin_a;
        gpio_num_t quad_pin_b;

        // Robot Characteristics (Configuration, one-time calculations)
        pulse_t pulsesPerRev;           // 
        dist_t  wheelDiameter;          // in MM.        
        time_t speedCheckIntervaluSec;  // Config: Min rate we store speed (uSeconds) values.
        dist_t convertPulsesToDist;    // Calculated: the factor to convert ticks into mm. 

        static bool isrAlreadyInstalled; // Internal flag
        static void IRAM_ATTR ISR_handler(void *arg); // Interrupt handler for quad inputs

        // Speed updates - these are done periodically via a 'high-resolution' timer.
        static void        update_speed_CB(void *arg); // Callback to update the speed (called from timer);
        esp_timer_handle_t spdUpdateTimer;             // the timer for driving the speed checker
        pulse_t            last_position;              // Position when we last updated our speed  
        time_t             last_update_time;           // The time when we last updated the speed
        dist_t             speed;                      // How fast (at last update time?). Actual units!

    volatile uint32_t pulseCount;  // A simple statistic...
    volatile uint32_t speedUpdateCount; 

};