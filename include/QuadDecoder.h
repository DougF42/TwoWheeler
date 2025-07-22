/**
 * @file QuadDecoder.h
 * @author Doug F (doug@fajaardo.hm)
 * @brief  Device driver for the quadrature decoders
 * @version 0.1
 * @date 2025-07-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include <Arduino.h>
#include "config.h"
#include "config.h"
#include "DefDevice.h"
#include "ESP32Encoder.h"
#include "esp_timer.h"

class QuadDecoder : public DefDevice
{
    private:
    ESP32Encoder  *myEncoder;
    pulse_t pulsesPerRev;
    double wheelDiam;
    pulse_t last_position;
    time_t last_timecheck;
    double last_speed;
    time_t currentSpdCheckRate;
    double convertPosition;  // converts pulse count to engineering units 
    static void update_speed_cb(void *arg);
    esp_timer_handle_t spdUpdateTimerhandle;
    

    public:
    QuadDecoder( const char * InName);
    ~QuadDecoder();
    void setup(MotorControl_config_t*cfg);
    ProcessStatus  ExecuteCommand () override;    // Override this method to handle custom commands
    ProcessStatus  DoPeriodic() override;         // Override this method to periodically send reports

    ProcessStatus qsetCommand();
    ProcessStatus qsckCommand();
    void setPhysParams(pulse_t pulseCnt, double diam);

    void setSpeedCheckInterval(time_t interval);
    double getPosition();
    void   resetPosition();
};

