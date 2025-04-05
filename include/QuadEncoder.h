/**
 * @file QuadEncoder.h
 * @author doug fajardo
 * @brief  Read the A and B phases from a Quadrature encoder
 * @version 0.1
 * @date 2025-04-04
 * 
 * @copyright Copyright (c) 2025
 * 
 * This reads both encoder signals, operates in 'FULL'
 *    encoding mode (position resolution = 4*pulses per revolution)
 */
#pragma once
#include <atomic>
#include "Arduino.h"

class QuadEncoder
{
    private:
    gpio_num_t pina;
    gpio_num_t pinb;
    std::atomic_long position;
    static void IRAM_ATTR gpio_isr_handler(void* arg);

    typedef enum { AoffBoff, AonBoff, AoffBon, AonBon } QUAD_STATE_t;
    QUAD_STATE_t last_state;

    public:
        QuadEncoder();
        void begin(gpio_num_t _pina, gpio_num_t _pinb);
        ~QuadEncoder();
        void forcePos(uint32_t pos);
        uint32_t getPosition();

};