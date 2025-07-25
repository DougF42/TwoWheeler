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
 *      Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
 *      myIna3221Device = new INA3221Device("Power", I2C_INA3221_ADDR, &Wire);
 *      ThisNode->AddDevice(myIna3221Device);
 *          ...
 *
 * Compilation note:
 *      There is an anoying warning from the Adafruit_INA3221 library that "legacy pcnt driver is deprecated, please migrate to use driver/pulse_cnt.h"
 * This warning can be safely ignored.
 *
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
 *     VCC - 2.7 to 5.5 volts.
 *     GND - common ground for power and logic
 *     VPU - connect to VCC. drives the pullups for various logic pins.
 *     SCL is I2c clock. includes 10k pullup
 *     SDA is I2C Data. includes 10k pullup
 *
 * I2C address on bottom - 0x40 is default, 0x41 if jumper in place.
 *
 * Future: Read status pins (available on the breakout board), possibly
 *         with interrupt-driven callback if voltage limits are exceeded.
 *
 * This is a front-end to the Adafruit_INA3221 library
 *    The github location is https://github.com/adafruit/Adafruit_INA3221
 *
 * Change Log:
 *  7/18/2025 DEF Ver 1.01
 *  7/81/2025 DEF Ver 2.00 - Refactor after review
 *       Moved setup operations into initializer. added initStatusOk variable.
 *       Default: periodic updates are enabled, occur once per 1minute.
 *       Remove unneeded functions - (setup, DoImmediate, ExecuteCommand)
 *       Add user code callable getChannelVoltage and getChannelCurrent to allow programatic access to lastr available readings.
 *       Add user code callable readValues to allow user code to force a read of current values.
 */
#pragma once
#include "Node.h"
#include "DefDevice.h"
#include "Adafruit_INA3221.h"
#include <Wire.h>
#define INA3221Version 3.0.0


class INA3221Device : public Adafruit_INA3221, public DefDevice
{
private:
    int i2cAddr;
    float busVolt[3];
    float current[3];

    // - - - - - -
    // This subclass is used to instantiate separate classes
    //  for reporting voltages and currents in separate messages.
    //
    class INA3221DeviceChannel : public DefDevice
    {
    private:
        float *datapointr;

    public:
        INA3221DeviceChannel(const char *inName, float *data);
        ~INA3221DeviceChannel();
        ProcessStatus DoPeriodic() override; // Override this method for processing your device periodically
    };

public:
    INA3221Device(const char *inName, int _i2CAddr, Node *myNode, TwoWire *theWire);
    ~INA3221Device();
    bool initStatusOk;                   // True if init was okay. false if any error
    ProcessStatus DoPeriodic() override; // Override this method for processing your device periodically
    ProcessStatus ExecuteCommand() override;
    ProcessStatus gpowerCommand();
    ProcessStatus setAveragingModeCommand();
    ProcessStatus setConvTime();

    double getChannelVoltage(int channel);
    double getChannelCurrent(int current);
};
