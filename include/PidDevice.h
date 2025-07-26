/**
 * @file PidDevice.h
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-06-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "config.h"
#include "DefDevice.h"
// #include "PID_v1.h"
#include "DefPID.h"
#include "esp_timer.h"
#include "QuadDecoder.h"
#include "ln298.h"


class PidDevice : public DefDevice
{
    private:
        QuadDecoder *quad;
        LN298       *ln298;
        esp_timer_handle_t pidTimerhandle;
        time_t     mySampleTime;

    public:
        DefPID *pid;
        char *name;

        double setPoint; // the value we want
        double actual;   // the actual value
        double output;   // what to set the motor (ln298) to
  
        double kp;
        double ki;
        double kd;
    
        PidDevice(const char * _name, MotorControl_config_t *cfg, QuadDecoder *_quad, LN298 *_ln298 );
        ~PidDevice();

        ProcessStatus  DoPeriodic     () override; 
        ProcessStatus  DoImmediate    () override;
        ProcessStatus  ExecuteCommand () override;

        ProcessStatus cmdSetSpeed();
        void setSpeed(double speed);

        ProcessStatus cmdSetP();
        void setP(double _kp);

        ProcessStatus cmdSetI();
        void setI(double _kp);

        ProcessStatus cmdSetD();
        void setD(double _kp);

        ProcessStatus cmdSetMode();
        void setMode(bool modeIsAuto);

        ProcessStatus cmdSetSTime();
        void setSampleClock(time_t intervalMs);

};