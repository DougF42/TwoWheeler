/**
 * @file ln298.h
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 * Use the LCD PWM driver on the ESP32 to 
 * drive one motor. 
 * 
 * The 'timer' is always timer0, only the channel no
 * changes.
 * 
 */
#pragma once
#include <atomic>
#include "driver/ledc.h"


class LN298
{
    private:
        ledc_channel_t led_channel; // the LEDC channel number
        // TIMER is one per motor
        ledc_timer_config_t timerCfg;
        gpio_num_t ena_pin;
        gpio_num_t dir_pin_a;
        gpio_num_t dir_pin_b;

    public:
        LN298();
        void setupLN298(ledc_channel_t chnlNo, gpio_num_t ena_pin, gpio_num_t dir_pin_a, gpio_num_t dir_pin_b);
        ~LN298();
        void setPulseWidth(int pcnt); // Set the pulse width (0..100)
        void drift();    // allow motor to drift to a stop (no brakes)
        void stop(int stopRate);  // 'gently' stop (brake) this motor
        void hardStop();
        // TBD:  void setAccel();

};