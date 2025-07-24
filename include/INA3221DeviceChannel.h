/**
 * @file INA3221Device.h   for 3 channel PWR monitor breakout
 * @author Doug Fajardo
  * @brief SMAC driver for INA3221 tripple Power monitoring device
 * @version  2.0.0
 * @date 2025-07-07
 * 
 * Written by Doug Fajardo Jully, 2025
 *              This code was developed as part of the SMAC project by Bill Daniels,
 *              and all rights and copyrights are hereby conveyed to that project.
 * 
 *              The SMAC project is Copyright 2021-2025, D+S Tech Labs, Inc.
 *              All Rights Reserved
 * 
  *
 */ 
#pragma once
#include "Device.h"

class INA3221DeviceChannel :public Device
{
    private:
        float *datapointr;

    public:
        INA3221DeviceChannel(const char *inName, float *data);
        ~INA3221DeviceChannel();
        ProcessStatus  DoPeriodic() override;  // Override this method for processing your device periodically
};

