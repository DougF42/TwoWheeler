// - - - - - - - - - - - - - - - - - - - - -
// @file INA3221.cpp
// @author Doug Fajardo
// @brief SMAC driver for INA3221 tripple Power monitoring device
// @version 1.01
// @date 2025-07-07
//
// Written by Doug Fajardo Jully, 2025
//              This code was developed as part of the SMAC project by Bill Daniels,
//              and all rights and copyrights are hereby conveyed to that project.
//
//              The SMAC project is Copyright 2021-2025, D+S Tech Labs, Inc.
//              All Rights Reserved
//
//  See INA3221Device.h for implementation and usage notes
// - - - - - - - - - - - - - - - - - - - - -

#include "INA3221Device.h"
#include "INA3221DeviceChannel.h"
#include "cmath"
#define  IAN3221Version "20250720a"

// - - - - - - - - - - - - - - - - - - - - -
// @brief Construct a new INA3221Device object
//
// @param inName   - name of this device
// @param _i2CAddr - adress on the I2C bus of the IAN3221
// @param theWire  - pointer to the 'Wire' class instance to use for I2C communication.
// - - - - - - - - - - - - - - - - - - - - -
INA3221Device::INA3221Device(const char *inName, int _i2CAddr, Node *myNode, TwoWire *theWire) : Device(inName)
{    
    periodicEnabled = true;
    immediateEnabled = false;
    strncpy(version, IAN3221Version, MAX_VERSION_LENGTH);
    version[MAX_VERSION_LENGTH-1]=0x00;
    SetRate(3600);
    setAveragingMode(INA3221_AVG_16_SAMPLES);
    i2cAddr = _i2CAddr;
    initStatusOk = true;

    // Init communications with I2C
    if (!Adafruit_INA3221::begin(i2cAddr, theWire))
    {
        Serial.println("Failed to find INA3221 chip");
        initStatusOk = false;
        return;
    }

    for (uint8_t idx = 0; idx < 3; idx++)
    {
        setShuntResistance(idx, 0.05);
    }
    initStatusOk = true;
    myNode->AddDevice(new INA3221DeviceChannel("Current0", &current[0]));
    myNode->AddDevice(new INA3221DeviceChannel("Current1", &current[1]));
    myNode->AddDevice(new INA3221DeviceChannel("Current2", &current[2]));
    myNode->AddDevice(new INA3221DeviceChannel("Volt0", &busVolt[0]));
    myNode->AddDevice(new INA3221DeviceChannel("Volt1", &busVolt[1]));
    myNode->AddDevice(new INA3221DeviceChannel("Volt2", &busVolt[2]));
}


// - - - - - - - - - - - - - - - - - - - - -
// @brief Destroy the INA3221 object
//
 // - - - - - - - - - - - - - - - - - - - - -
 INA3221Device::~INA3221Device()
{
    return;
}


// - - - - - - - - - - - - - - - - - - - - -
// Report the battery voltage and current readings
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus INA3221Device::DoPeriodic()
{
    readCurrentValues();

   // sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
   //         busVolt[0], current[0], busVolt[1], current[1], busVolt[2], current[2]);

    return (SUCCESS_NODATA);
}

// - - - - - - - - - - - - - - - - - - - - -
// Read the voltage and values from the INA3221 for all three channels
// - - - - - - - - - - - - - - - - - - - - -
void INA3221Device::readCurrentValues()
{
    for (int idx = 0; idx < 3; idx++)
    {
        busVolt[idx] = getBusVoltage(idx);
        current[idx] = getCurrentAmps(idx) * 1000; // convert to ma
    }
}

// - - - - - - - - - - - - - - - - - - - - -
// @brief Get the Channel Voltage for a given channel
//
//  NOTE: This does NOT re-read the voltage from the channel - it uses the last
//        value read.
//
//     If the channel number is invalid, return INFINITY.
//
// @param chhannel number (0 thru 2)
// @return double  - the last read current on the requested channel, or INFINITY if the
// - - - - - - - - - - - - - - - - - - - - -
double INA3221Device::getChannelVoltage(int channel)
{
    if ((channel < 0) || (channel > 2))
        return (INFINITY);
    return (busVolt[channel]);
}


// - - - - - - - - - - - - - - - - - - - - -
// @brief Get the Current of a given channel
//
// NOTE: This does NOT re-read the voltage from the channel - it uses the last
//        value read.
//
//     If the channel number is invalid, return 0.
// @param chhannel number (0 thru 2)
// @return double  - the last read current on the requested channel, or INFINITY if the
// - - - - - - - - - - - - - - - - - - - - -
double INA3221Device::getChannelCurrent(int channel)
{
    if ((channel < 0) || (channel > 2))
        return (INFINITY);
    return (current[channel]);
}
