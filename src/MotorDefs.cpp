/**
 * @file MotorDef.cpp
 * @author Doug Fajardo
 * @brief  Defines the ports and pointers to the various drivers related to a motor
 * @version 0.1
 * @date 2025-04-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "atomic"
#include "MotorDefs.h"

// static definitions
std::atomic<uint8_t> MotorDefs::HaveBeenInited = 0;

MotorDefs MotorTable[MAXNOOFMOTORS];

/**
 * @brief Construct a MotorDefs object
 * 
 */
MotorDefs::MotorDefs()
{
    HaveBeenInited++;
    if (HaveBeenInited > 1)
    {
        HaveBeenInited--;
        return;
    }

    ena_Pin = GPIO_NUM_NC;
    dir_pin_a = GPIO_NUM_NC;
    dir_pin_b = GPIO_NUM_NC;
    quad_pin_a = GPIO_NUM_NC;
    quad_pin_b = GPIO_NUM_NC;
    // ln298_drvr =
    // Quad_drvr =
    // pid_drvr  =
}

MotorDefs::~MotorDefs()
{
}

bool MotorDefs::defineMotor(int mtr, gpio_num_t _enaPin, gpio_num_t _dir_pin_a, gpio_num_t _dir_pin_b,
                            gpio_num_t _quad_pin_a, gpio_num_t quadB_pin)
{
}