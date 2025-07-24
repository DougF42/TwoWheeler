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

INA3221DeviceChannel::INA3221DeviceChannel(const char *inName, float *ptr) : Device(inName)
{
 datapointr = ptr;
 immediateEnabled = false;
 SetRate(1800);
}


// - - - - - - - - - - - - - - - - - - - - -
// @brief Destroy the INA3221 object
//
 // - - - - - - - - - - - - - - - - - - - - -
 INA3221DeviceChannel::~INA3221DeviceChannel()
{
}


// - - - - - - - - - - - - - - - - - - - - -
// Report the battery voltage and current readings
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus INA3221DeviceChannel::DoPeriodic()
{
    DataPacket.timestamp = millis();
    sprintf(DataPacket.value, "%f", *datapointr);
    return (SUCCESS_DATA);
}
