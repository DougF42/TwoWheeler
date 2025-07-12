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
#include <atomic>

// We define this separatly so that it
// can be placed in IRAM for fast
// handling by the ISR routine
typedef struct { 
    std::atomic_char16_t   curPosition;    // current motor position. NOTE: Must match pos_t!!!
    std::atomic_uint8_t    lastState;   // Last state of the quadtrature encoding
    gpio_num_t phaseApin;   // pin number for phase A
    gpio_num_t phaseBpin;   // pin number for phase B
} quad_reader_info_t;

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
    quad_reader_info_t *myInfo;   // Point to my info 

    friend void gpio_interupt_isr(void *arg);
};