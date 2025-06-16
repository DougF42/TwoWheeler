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
#include "DefDevice.h"
#include "PID_v1.h"
#include "esp_timer.h"


class PidDevice : public DefDevice
{
    private:
        PID *pid;
        char *name;
        double input;
        double output;
        double setPoint;
        double kp;
        double ki;
        double kd;

    public:
        PidDevice(Node *_node, const char * _name);
        ~PidDevice();
        void setPID(); 

        ProcessStatus  DoPeriodic     () override; 
        ProcessStatus  ExecuteCommand () override;
};