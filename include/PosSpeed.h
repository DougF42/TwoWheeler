/**
 * @file PosSpeed.h
 * @author Doug Fajardo
 * @brief handle converting position to real units, and detemrining speed
 * @version 0.1
 * @date 2025-07-12
 * 
 * @copyright Copyright (c) 2025
 * 
 * Speed is calculated by a regular (as detemrined by the FAST_TIMER clock) 
 *    call to detemrine position.
 */
#pragma once
#include "DefDevice.h"
#include "config.h"
#include "Arduino.h"
#include <driver/gpio.h>
#include "esp_timer.h"
#include "sdkconfig.h"
#include "QuadReader.h"
#include "atomic"

class PosSpeed: public DefDevice
{
    public: 
        PosSpeed(Node *_node, const char *InName,  QuadReader *myQuad);
        ~PosSpeed();
        bool setup(MotorControl_config_t *cfg);
        ProcessStatus  DoPeriodic() override;              // Override this method to periodically send reports
        ProcessStatus  ExecuteCommand () override;         // Override this method to handle custom commands
        ProcessStatus  cmdSetSpeedCheckInterval();
        ProcessStatus  cmdCalibrate();
        ProcessStatus  resetPos();
        void setSpeedCheckInterval(time_t spd);
        void calibrate (pulse_t pulsesPerRev, dist_t diameter);  
        void getCalibration(pulse_t *pulsesPerRev, dist_t *diameter);

    private:
        static void update_speed_cb(void *arg);
        esp_timer_handle_t spdUpdateTimerhandle;
        time_t lastUpdateTime;       // 
        dist_t  lastpulseRate;       // last pulse rate (pulses per millisecond)
        QuadReader *myQuadPtr;       // Who is collecting pulse counts?
        dist_t wheelDia;             // Configuration of the Wheel
        pulse_t pulsesPerRev;        // Configuratoin of the encoder
        pulse_t prevPulsePos; // This position is based on the QuadReader, in pulses
};