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
 */
#pragma once
#include "MotorDefs.h"
#include "atomic"
#include <driver/gpio.h>

class QuadDecoder
{
    private:
        MotorDefs *pmdefs; // Points to my motor definitions
        typedef enum { AoffBoff, AonBoff, AoffBon, AonBon } QUAD_STATE_t;
        QUAD_STATE_t last_state;
        std::atomic<int32_t> position;  // NOTE: Position *can* be negative!
        static void ISR_handlePhaseA(void *arg);
        static void ISR_handlePhaseB(void *arg);

    public:
        QuadDecoder(int mtrNumber);
        ~QuadDecoder();
        uint32_t getCurPos();
        void resetPos(uint32_t newPos=0);
};