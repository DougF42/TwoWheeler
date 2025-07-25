/**
 * @file DEF_Pid.h
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
#include "PIDX.h"
#include "esp_timer.h"
#include "DEV_QuadDecoder.h"
#include "DEV_ln298.h"


class DEV_Pid : public DefDevice
{
    private:
        DEV_QuadDecoder *quad;
        DEV_LN298       *ln298;
        esp_timer_handle_t pidTimerhandle;
        time_t     mySampleTime;

    public:
        PIDX *pid;
        char *name;

        double setPoint; // the value we want
        double actual;   // the actual value
        double output;   // what to set the motor (ln298) to
  
        double kp;
        double ki;
        double kd;
    
        DEV_Pid(const char * _name, MotorControl_config_t *cfg, DEV_QuadDecoder *_quad, DEV_LN298 *_ln298 );
        ~DEV_Pid();

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