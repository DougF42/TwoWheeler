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
#include <FreeRTOS.h>
#include <freertos/task.h>

#include "SMAC/Device.h"
#include "ESP32Encoder.h"
#include "esp_timer.h"

class DEV_QuadDecoder : public Device
{
private:
    const char *mememme;
    ESP32Encoder *myEncoder;
    pulse_t pulsesPerRev;
    double wheelDiam;        // diameter in mm
    pulse_t last_position;

    double last_speed;
    time_t currentSpdCheckms; // in ticks.   ticks =  msecs/ portTICK_PERIOD_MS

    double pulsesToDist; // converts pulse count to engineering units

    // static void updateSpeedTask(void *arg);
    // TaskHandle_t speedTaskHandle; // poitns to task that reads/updates 'last_speed'.
    // bool speedTaskRunning;

public:
    DEV_QuadDecoder(const char *InName);
    ~DEV_QuadDecoder();
    void setup(MotorControl_config_t *cfg);
    ProcessStatus DoImmediate() override;
    ProcessStatus ExecuteCommand(char *command, char *params = NULL) override; // Override this method to handle custom commands
    ProcessStatus DoPeriodic() override;                                       // Override this method to periodically send reports

    ProcessStatus qsetCommand(char *command, char *params);
    ProcessStatus qsckCommand(char *command, char *params);
    ProcessStatus statusCommand(char *command, char *params);
    
    void          setPhysParams(pulse_t pulseCnt, double diam);
    void          setSpeedCheckInterval(time_t intervalMs);
    double        getPosition();
    double        getSpeed();
    void          resetPosition();
};
