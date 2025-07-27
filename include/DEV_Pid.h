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
                                  // Commands (setpoint) are 
        DEV_QuadDecoder *quad;    // pointer to the quad device we get 'actual; from
        DEV_LN298       *ln298;   // pointer to the devuce we sebd the output to.
        esp_timer_handle_t pidTimerhandle;
        time_t          mySampleTime;

    public:
        PIDX *pid;
        char *name;

        // The PID device requires we have our own storage for these...
        double setPoint; // the value we want
        double actual;   // the actual value
        double output;   // what to set the motor (ln298) to
  
        double kp;
        double ki;
        double kd;

        DEV_Pid(const char *_name, MotorControl_config_t *cfg,
             DEV_QuadDecoder *_quad, DEV_LN298 *_ln298);
        ~DEV_Pid();
        static void timer_callback(void *arg);
        ProcessStatus DoPeriodic() override;
        // ProcessStatus  DoImmediate    () override;
        ProcessStatus ExecuteCommand() override;

        ProcessStatus cmdSetSpeed(); // external command to directly 
                                     // set the 'setpoint' or 
                                     // desired speed. Same units as
                                     // used by QUAD.

        void setSpeed(double speed); // Call this to set the 'setpoint'
                                     // or desired speed. Same units as
                                     // used by QUAD.

        ProcessStatus cmdSetP();     // set the P Parameter
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