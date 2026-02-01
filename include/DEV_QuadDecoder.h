/**
 * @file DEV_QuadDecoder.h
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
#include "SMAC/Device.h"
#include "ESP32Encoder.h"
#include "esp_timer.h"

class DEV_QuadDecoder : public Device
{
private:
    ESP32Encoder *myEncoder;
    pulse_t pulsesPerRev;
    double wheelDiam;        // diameter in mm
    pulse_t last_position;
    time_t last_timecheck;
    double last_speed;
    time_t currentSpdCheckRate;
    double pulsesToDist; // converts pulse count to engineering units
    static void update_speed_cb(void *arg);
    esp_timer_handle_t spdUpdateTimerhandle;

public:
    DEV_QuadDecoder(const char *InName);
    ~DEV_QuadDecoder();
    void setup(MotorControl_config_t *cfg);
    ProcessStatus ExecuteCommand(char *command, char *params = NULL) override; // Override this method to handle custom commands
    ProcessStatus DoPeriodic() override;                                       // Override this method to periodically send reports

    ProcessStatus qsetCommand(char *command, char *params);
    ProcessStatus qsckCommand(char *command, char *params);
    ProcessStatus statusCommand(char *command, char *params);
    
    void          setPhysParams(pulse_t pulseCnt, double diam);
    void          setSpeedCheckInterval(time_t interval);
    double        getPosition();
    double        getSpeed();
    void          resetPosition();
};
