/**
 * @file MotorDef.h
 * @author Doug Fajardo
 * @brief  This class defines the pins for motor and encoder, and pts to instances to process each
 * @version 0.1
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 * Each time the 'addMotor' generator method is invoked, it allowcates an instance of 'MotorDef' in
 * the STATIC and GLOBAL array Motors. This is required by the interrupt handlers to be able to
 * find the appropriate instance.
 * 
 * The max number of motors is limited by a number of hardware factors - dont change it unless
 * you are *sure* you know what you are doing!
 */
#pragma once
#define MAXNOOFMOTORS 4
#include "atomic"
#include <driver/gpio.h>

#define MAX_MOTORS 4
// - - - - - - - - - - - - - - - - -
class MotorDefs
{
private:
    MotorDefs();
    ~MotorDefs();
    static std::atomic<uint8_t> HaveBeenInited;

public:
    gpio_num_t ena_Pin;
    gpio_num_t dir_pin_a;
    gpio_num_t dir_pin_b;
    gpio_num_t quad_pin_a;
    gpio_num_t quad_pin_b;
    // ln298_drvr // pt to ln 298 instance
    // Quad_drvr  // pt to Encoder instance
    // Pid_drvr   // pt to PID Motor driver instance

    bool defineMotor(int mtr, gpio_num_t _enaPin, gpio_num_t _dir_pin_a, gpio_num_t _dir_pin_b,
        gpio_num_t _quad_pin_a, gpio_num_t quadB_pin);

    static MotorDefs MotorTable[MAXNOOFMOTORS];
};


