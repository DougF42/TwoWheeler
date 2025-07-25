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
#include "cmath"
#define  IAN3221Version "20250720a"


// - - - - - - - - - - - - - - - - - - - - -
// @brief Construct a new INA3221Device object
//
// @param inName   - name of this device
// @param _i2CAddr - adress on the I2C bus of the IAN3221
// @param theWire  - pointer to the 'Wire' class instance to use for I2C communication.
// - - - - - - - - - - - - - - - - - - - - -

INA3221Device::INA3221DeviceChannel::INA3221DeviceChannel(const char *inName, float *ptr) : DefDevice(inName)
{
 datapointr = ptr;
 immediateEnabled = false;
 SetRate(1800);
}


// - - - - - - - - - - - - - - - - - - - - -
// @brief Destroy the INA3221 object
//
 // - - - - - - - - - - - - - - - - - - - - -
 INA3221Device::INA3221DeviceChannel::~INA3221DeviceChannel()
{
}


// - - - - - - - - - - - - - - - - - - - - -
// Report the battery voltage and current readings
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus INA3221Device::INA3221DeviceChannel::DoPeriodic()
{
    DataPacket.timestamp = millis();
    sprintf(DataPacket.value, "%f", *datapointr);
    return (SUCCESS_DATA);
}



// - - - - - - - - - - - - - - - - - - - - -
// @brief Construct a new INA3221Device object
//
// @param inName   - name of this device
// @param _i2CAddr - adress on the I2C bus of the IAN3221
// @param theWire  - pointer to the 'Wire' class instance to use for I2C communication.
// - - - - - - - - - - - - - - - - - - - - -
INA3221Device::INA3221Device(const char *inName, int _i2CAddr, Node *myNode, TwoWire *theWire) : DefDevice(inName)
{    
    periodicEnabled = true;
    immediateEnabled = false;
    strncpy(version, IAN3221Version, MAX_VERSION_LENGTH);
    version[MAX_VERSION_LENGTH-1]=0x00;
    SetRate(3600);
    setAveragingMode(INA3221_AVG_16_SAMPLES);
    setBusVoltageConvTime(INA3221_CONVTIME_1MS);
    setShuntVoltageConvTime(INA3221_CONVTIME_1MS);

    i2cAddr = _i2CAddr;
    initStatusOk = true;
    for (int i=0; i<3; i++) {
        current[i]=0.0;
        busVolt[i]=0.0;
    }

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
// Handle any SMAC commands 
// FORMAT: GPOW   ( get all 6 current values)
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus  INA3221Device::ExecuteCommand () 
{
    ProcessStatus retVal=SUCCESS_NODATA;
    DataPacket.timestamp=millis();
    retVal = ExecuteCommand();
    if (retVal == NOT_HANDLED)
    {
        scanParam();
        if (isCommand("SPOW"))
        {
            retVal=gpowerCommand();
        } else if (isCommand("SAVG"))
            retVal=setConvTime();
         else {
            retVal=FAIL_DATA;
            sprintf(DataPacket.value, "ERROR: Uknown command");
        }
    }
 
    if (retVal==SUCCESS_NODATA)
    {        
        sprintf(DataPacket.value, "OK");
        retVal=SUCCESS_DATA;
    }

    return(retVal);
}


/**
 * Report the current values of all three channels
 * Format: 
 *       sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
 *         busVolt[0], current[0], busVolt[1], current[1], busVolt[2], current[2]);

 */
ProcessStatus INA3221Device::gpowerCommand()
{
    DataPacket.timestamp = millis();
    sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
        busVolt[0], current[0], busVolt[1], current[1], 
        busVolt[2], current[2]);
    return(SUCCESS_DATA);
}


/*
 * @brief how many samples to average?
 * FORMAT:   SAVR|<value>
 *     Value is one of the following:
 *         1, 4, 16, 64, 128, 256, 512, 1024
 */
ProcessStatus INA3221Device::setAveragingModeCommand()
{
    ProcessStatus retVal;
    uint8_t val;
    // bool setAveragingMode(ina3221_avgmode mode);
    retVal=getUInt8(0, &val, "Averaging mode: ");
    if (retVal == SUCCESS_NODATA)
    {
        if (val == 1)
            setAveragingMode(INA3221_AVG_1_SAMPLE);
        else if (val == 4)
            setAveragingMode(INA3221_AVG_4_SAMPLES);
        else if (val == 16)
            setAveragingMode(INA3221_AVG_16_SAMPLES);
        else if (val == 64)
            setAveragingMode(INA3221_AVG_64_SAMPLES);
        else if (val == 128)
            setAveragingMode(INA3221_AVG_128_SAMPLES);
        else if (val == 256)
            setAveragingMode(INA3221_AVG_256_SAMPLES);
        else if (val == 512)
            setAveragingMode(INA3221_AVG_512_SAMPLES);
        else if (val == 1024)
            setAveragingMode(INA3221_AVG_1024_SAMPLES);
        else
        {
            DataPacket.timestamp = millis(); 
            sprintf(DataPacket.value,"ERROR:  Must be one of 1,4,16,64,128,256,512,1024");
            retVal = FAIL_DATA;
        }
    }

    if (retVal == SUCCESS_NODATA)
    {
        DataPacket.timestamp = millis();
        sprintf(DataPacket.value, "OK");
        retVal = SUCCESS_DATA;
    }
    return(retVal);
}


/**
 * @brief how long should conversion time take?
 * Format: SBUS|<time>
 *    time is one of the following:
 *        140 (uSecs)   204 (uSecs) 332 (uSecs)
 *          1 (mSec)      2 (mSecs)  4 (mSecs) 8 (secs)
 *
 */
ProcessStatus INA3221Device::setConvTime()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    uint8_t val = 0;
    retVal = getUInt8(0, &val, "Averaging mode: ");
    if (retVal == SUCCESS_NODATA)
    {
        retVal = getUInt8(0, &val, "Averaging mode: ");
        if (retVal == SUCCESS_NODATA)
        {
            if (val == 140)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_140US);
                setShuntVoltageConvTime(INA3221_CONVTIME_140US);
            }
            else if (val == 204)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_204US);
                setShuntVoltageConvTime(INA3221_CONVTIME_204US);
            }
            else if (val == 332)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_332US);
                setShuntVoltageConvTime(INA3221_CONVTIME_332US);
            }
            else if (val == 588)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_588US);
                setShuntVoltageConvTime(INA3221_CONVTIME_588US);
            }
            else if (val == 1)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_1MS);
                setShuntVoltageConvTime(INA3221_CONVTIME_1MS);
            }
            else if (val == 2)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_2MS);
                setShuntVoltageConvTime(INA3221_CONVTIME_2MS);
            }
            else if (val == 4)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_4MS);
                setShuntVoltageConvTime(INA3221_CONVTIME_4MS);
            }
            else if (val == 8)
            {
                setBusVoltageConvTime(INA3221_CONVTIME_8MS);
                setShuntVoltageConvTime(INA3221_CONVTIME_8MS);
            }
            else
            {
                retVal = FAIL_DATA;
                sprintf(DataPacket.value, "ERROR: Convert time must be 140, 204, 332, 588, 1, 2, 4, 8");
            }
        }
    }

    if (retVal == SUCCESS_NODATA)
    {
        retVal = SUCCESS_DATA;
        sprintf(DataPacket.value, "OK");
    }
    DataPacket.timestamp = millis();
    return (retVal);
}

// - - - - - - - - - - - - - - - - - - - - -
// get the battery voltage and current readings
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus INA3221Device::DoPeriodic()
{
    if (getFlags() | INA3221_CONV_READY)
    {
        for (int idx = 0; idx < 3; idx++)
        {
            busVolt[idx] = getBusVoltage(idx);
            current[idx] = getCurrentAmps(idx) * 1000; // convert to ma
        }
    }
    return(SUCCESS_NODATA);
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
