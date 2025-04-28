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
 * We also track the RATE at which the A phase goes high
 * to determine average speed.
 * 
 * When loop is called, it used the elapsed time from
 * the last time it was called, and the difference in
 * position, to determine the speed.
 */
#pragma once
#include "config.h"
#include "atomic"
#include <driver/gpio.h>


class QuadDecoder
{
    public:
        typedef enum {UNITS_MM, UNITS_IN}QuadUnits_t;

    private:
        // Encoder definitions
        QuadUnits_t units;     // What unit of measure? this is for documentation purposes
        double convertTickToDist;     // this is calculated!
        double convertPositionToDist;
        typedef enum { AoffBoff, AonBoff, AoffBon, AonBon } QUAD_STATE_t;
        QUAD_STATE_t last_state;

        //MotorDefs *pmdefs; // Points to my motor definitions
        int motorIdx;

        uint32_t lastLoopTime;
        std::atomic<unsigned long> pulseCount; // number of pulses since last speed check
        std::atomic<int32_t> position;  // Position in pulses- Position *can* be negative!
        std::atomic<double> lastTicksPerSecond; // last rate (pulses per second)
        uint32_t speedCheckIntervaluSec;
        static void ISR_handlePhaseA(void *arg);
        static void ISR_handlePhaseB(void *arg);

    public:
        QuadDecoder();
        ~QuadDecoder();
        void setup(int mtrNumber);
        uint32_t getCurPos();
        void resetPos(uint32_t newPos=0);
        void loop();
        int32_t getSpeed();
        double getPosition(); 
        void setSpeedCheckInterval(uint32_t rate=SPEED_CHECK_INTERVAL_uSec);
        void calibrate (uint pulsesPerRev, uint circumfrence, QuadUnits_t _units);       
};