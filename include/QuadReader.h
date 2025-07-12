/**
 * @file QuadRead.h
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "DefDevice.h"
#include "config.h"
#include "Arduino.h"
#include <driver/gpio.h>
#include "esp_timer.h"
#include "sdkconfig.h"
#include "atomic"

class QuadReader: public DefDevice
{

public:   
    QuadReader(Node *_node, const char * InName);
    ~QuadReader();
     bool setup(MotorControl_config_t *cfg);

    ProcessStatus  ExecuteCommand () override;    // Override this method to handle custom commands
    ProcessStatus  DoPeriodic() override;         // Override this method to periodically send reports

    pulse_t getPosition();
    void   resetPosition();

protected:
    static bool isrAlreadyInstalled; // Internal flag
    uint8_t last_state;            // state of the decoder (set by ISR)
    pulse_t curPosition;                // change in Current position (in pulses)
    gpio_num_t quad_pin_a;              // the 'a' pin for this quad
    gpio_num_t quad_pin_b;              // the 'b' pin for this quad
    friend void gpio_interupt_isr(void *arg);
};