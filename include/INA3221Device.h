/**
 * @file INA3221Device.h   for 3 channel PWR monitor breakout
 * @author Doug Fajardo
  * @brief SMAC driver for INA3221 tripple Power monitoring device
 * @version  1.0
 * @date 2025-07-07
 * 
 * Written by Doug Fajardo Jully, 2025
 *              This code was developed as part of the SMAC project by Bill Daniels,
 *              and all rights and copyrights are hereby conveyed to that project.
 * 
 *              The SMAC project is Copyright 2021-2025, D+S Tech Labs, Inc.
 *              All Rights Reserved
 * 
 * - - - -  SETUP and USAGE:
 *  This is a 'driver' for SMAC to utilize the INA3221 tripple power monitoring 
 *      chip, as implemented on Adafruit's breakout board (product no 6062) to 
 *      measure the voltage (relative to ground) and Current on 3 circuits.
 * 
 *      It periodically sends a single packet containing the voltage and current for 
 *         each of three channels as follows:
 * 
 *           dd|nn|<timestamp> BATX|<volt_ch1>|<cur_ch1>|<volt_ch2>|<cur_ch2>|<volt_ch3>|<cur_ch3>
 *                 <timestamp> - supplied by SMAC device class.
 *                 Voltage is floating point, in volts.
 *                 Current is floating point, in milliAmps.
 *
 *
 * SETUP and USAGE
 * This requires the Adafruit_INA3221 library.
 *     The Wire library is used for I2C communications (The INA3221 is on port 0x40
 *     by default, adding a jumper can move it to port 0x41).  This library
 *     should be configured before calling the INA3221_device::setup() method.
 * 
 *    TYPICAL EXAMPLE:
 *         #include "Wire.h"
 *         #include "INA3221Device.h"
 *         #define I2C_INA3221_ADDR 0x40
 *          ...
 *         INA3221Device myIna3221Device = INA3221Device("Power");
 *         Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 
 *         myIna3221Device->setup(I2C_INA3221_ADDR, &Wire); 
 *         ThisNode->AddDevice(myIna3221Device);
 *          ...
 * 
 * Compilation note:
 *      There is an anoying warning from the Adafruit_INA3221 library that "legacy pcnt driver is deprecated, please migrate to use driver/pulse_cnt.h"
 * This warning can be safely ignored.
//=============================================================================
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
 * 
 * I2C address on bottom - 0x40 is default, 0x41 if jumper in place.
 * 
 * We do not use any interrupt pins in this driver
 * 
 * This is a front-end to the Adafruit_INA3221 library
 *    The github location is https://github.com/adafruit/Adafruit_INA3221
 */ 
#pragma once
#include "Device.h"
#include "Adafruit_INA3221.h"
#include <Wire.h>

class INA3221Device : public Adafruit_INA3221, public Device
{
    private:
        int i2cAddr;

    public:
        INA3221Device(const char *inName);
        ~INA3221Device();
        bool setup(int I2CAddr,  TwoWire *theWire); // Set up the driver, initialize I2C
        ProcessStatus  DoImmediate    () override ;  // Override this method for processing your device continuously
        ProcessStatus  DoPeriodic     () override;  // Override this method for processing your device periodically
        ProcessStatus  ExecuteCommand () override;  // Override this method to handle custom commands
};
