/**
 * @file INA3221Device.h   for 3 channel PWR monitor breakout
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-07-07
 * 
 * @copyright Copyright (c) 2025
 * 
 * Measurements:
 *     The built-in resistor is 0.05 ohms. 
 *     Max amplifier input difference is +-163.8mv, 
 *     This allows measurements up to +-3.2 amps.
 *     With 13 bit ADC, the resolution is 0.4mA.
 * 
 * Pinout info at https://learn.adafruit.com/adafruit-ina3221-breakout/pinouts
 * VCC - 2.7 to 5.5 v
 * FND - common ground for power and logic
 * VPU - connect to VCC to indicate power valid.
 * SCL is I2c clock. includes 10k pullup
 * SDA is I2C Data. includes 10k pullup
 * 
 * I2C address on bottom - 0x40 is default, 0x41 if jumper in place.
 * 
 * We do not use any interrupt pins in this driver
 * 
 * This is a front-end to the Adafruit_INA3221 library
 *    The github location is https://github.com/adafruit/Adafruit_INA3221
 */ 
#pragma once
#include "DefDevice.h"
#include "Adafruit_INA3221.h"
#include <Wire.h>

class INA3221Device : public Adafruit_INA3221, public DefDevice
{
    private:
        int i2cAddr;
        ProcessStatus enableChannelCmd();
        ProcessStatus disableChannelCmd();
        ProcessStatus reportChannelStatusCmd();
        bool channelEnaFlag[3]; // Is the channel enabled for reporting?

    public:
        INA3221Device(Node *node, const char *inName);
        ~INA3221Device();
        bool setup(int I2CAddr=0x40); // Set up the driver, initialize I2C
        ProcessStatus  DoImmediate    () override ;  // Override this method for processing your device continuously
        ProcessStatus  DoPeriodic     () override;  // Override this method for processing your device periodically
        ProcessStatus  ExecuteCommand () override;  // Override this method to handle custom commands
};
