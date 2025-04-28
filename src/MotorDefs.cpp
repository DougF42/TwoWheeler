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
MotorDefs MotorTable[MAXNOOFMOTORS];

std::atomic<uint8_t> MotorDefs::HaveBeenInited = 0;

MotorDefs::MotorDefs()
{
    ena_pin = GPIO_NUM_NC;
    dir_pin_a = GPIO_NUM_NC;
    dir_pin_b = GPIO_NUM_NC;
    quad_pin_a = GPIO_NUM_NC;
    quad_pin_b = GPIO_NUM_NC;
    ln298      = nullptr;
    quad       = nullptr;
    // pid_drvr  =
}


MotorDefs::~MotorDefs()
{
}

/// @brief 
/// @param mtr 
/// @param _enaPin 
/// @param _dir_pin_a 
/// @param _dir_pin_b 
/// @param _quad_pin_a 
/// @param _quad_pin_b 
/// @return 
bool MotorDefs::defineMotor(int mtr, gpio_num_t _enaPin, gpio_num_t _dir_pin_a, gpio_num_t _dir_pin_b,
                            gpio_num_t _quad_pin_a, gpio_num_t _quad_pin_b)
{
    // TODO:

    MotorTable[mtr].ena_pin    = _enaPin;
    MotorTable[mtr].dir_pin_a = _dir_pin_a;
    MotorTable[mtr].dir_pin_b = _dir_pin_b;
    MotorTable[mtr].quad_pin_a= _quad_pin_a;
    MotorTable[mtr].quad_pin_b= _quad_pin_b;
    MotorTable[mtr].ln298=new LN298();
    MotorTable[mtr].ln298->setup(mtr);
    MotorTable[mtr].quad=new QuadDecoder();
    MotorTable[mtr].quad->setup(mtr);
    return(false);
}