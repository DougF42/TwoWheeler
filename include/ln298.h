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
#include "config.h"
#include "driver/ledc.h"
#include "DefDevice.h"


class LN298 : public DefDevice
{
    private:
        volatile static uint8_t timer_is_inited;
        ledc_channel_t led_channel; // the LEDC channel number

        // TIMER is one per motor
        ledc_timer_config_t timerCfg;
        gpio_num_t ena_pin;
        gpio_num_t dir_pin_a;
        gpio_num_t dir_pin_b;
        void setDirection(int pcnt);   // use the sign of pcnt to set direction
        enum  MotorStatus {MOTOR_DIS, MOTOR_IDLE, MOTOR_FWD, MOTOR_REV, MOTOR_STOP} motorStatus;
        int lastPcnt;
        
    public:
        LN298( Node *_node, const char * Name);
        void setupLN298(MotorControl_config_t *cfg);
        ~LN298();
        ProcessStatus  ExecuteCommand () override;
        ProcessStatus  DoPeriodic()  override;
        ProcessStatus  setPulseWidthCommand();
        void setReportStatus(bool enaFlag);
        bool setPulseWidth(int pcnt); // Set the pulse width (0..100)
        ProcessStatus enable(bool isRemoteCmd=false);
        ProcessStatus disable(bool isRemoteCmd=false);
        ProcessStatus hardStop(bool isRemoteCmd=false);

};