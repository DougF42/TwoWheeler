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

#include "DEV_INA3221.h"
#include "cmath"
#include "esp_log.h"
#include "config.h"

// - - - - - - - - - - - - - - - - - - - - -
// @brief Construct a new INA3221Device object
//
// @param inName   - name of this device
// @param _i2CAddr - adress on the I2C bus of the IAN3221
// @param theWire  - pointer to the 'Wire' class instance to use for I2C communication.
// - - - - - - - - - - - - - - - - - - - - -
DEV_INA3221::INA3221DeviceChannel::INA3221DeviceChannel(const char *inName, DEV_INA3221 *_me, int _dataPtNo) :
     DefDevice(inName)
{
    me = _me;
    dataPointNo = _dataPtNo;
    immediateEnabled = false;
    SetRate(900);
}

// - - - - - - - - - - - - - - - - - - - - -
// @brief Destroy the INA3221 object
//
 // - - - - - - - - - - - - - - - - - - - - -
 DEV_INA3221::INA3221DeviceChannel::~INA3221DeviceChannel()
{    
}


// - - - - - - - - - - - - - - - - - - - - -
// Report the battery voltage and current readings
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus DEV_INA3221::INA3221DeviceChannel::DoPeriodic()
{
    
    float val=0;
    me->getDataReading(dataPointNo, &val, &DataPacket.timestamp);
    sprintf(DataPacket.value, "%f",val,  &DataPacket.timestamp);
    return (SUCCESS_DATA);
}


// - - - - - - - - - - - - - - - - - - - - -
// @brief Construct a new INA3221Device object
//
// @param inName   - name of this device
// @param _i2CAddr - adress on the I2C bus of the IAN3221
// @param theWire  - pointer to the 'Wire' class instance to use for I2C communication.
// - - - - - - - - - - - - - - - - - - - - -
DEV_INA3221::DEV_INA3221(const char *inName, int _i2CAddr, Node *myNode, TwoWire *theWire) : DefDevice(inName)
{    
    // Device default condition
    strncpy(version, INA3221Version, MAX_VERSION_LENGTH);
    version[MAX_VERSION_LENGTH-1]=0x00;
    periodicEnabled = false;
    immediateEnabled = false;
    SetRate(1800);           // default reporting rate
    initStatusOk = true;     // flag - did we initialze properly?

    // Initialize the readings to 0.
    for (int i=0; i<6; i++) {
       dataReadings[i]=0.0;
    }

    // Init communications with I2C
    i2cAddr = _i2CAddr;    // Remember our address
    if (!Adafruit_INA3221::begin(i2cAddr, theWire))
    {
        Serial.println("Failed to find INA3221 chip");
        initStatusOk = false;
        return;
    }

    // CONFIGRE THE INA3221 device, and initialize the subtasks
    for (uint8_t idx = 0; idx < 3; idx++)
    {
        setShuntResistance(idx, 0.05);
    }
    initStatusOk = true;
    myNode->AddDevice(new INA3221DeviceChannel("CVolt0",   this, 0));
    myNode->AddDevice(new INA3221DeviceChannel("CVolt0",   this, 1));
    myNode->AddDevice(new INA3221DeviceChannel("CVolt0",   this, 2));
    myNode->AddDevice(new INA3221DeviceChannel("Current0", this, 3));
    myNode->AddDevice(new INA3221DeviceChannel("Current1", this, 4));
    myNode->AddDevice(new INA3221DeviceChannel("Current2", this, 5));

    setAveragingMode(INA3221_AVG_16_SAMPLES);
    noOfSamplesPerReading=16;
    sampleTimeUs = 1000;  
 
    ESP_ERROR_CHECK(xTaskCreate(readDataTask, "ReadINA3221", 1024, NULL, 3, &readtask));
}


// - - - - - - - - - - - - - - - - - - - - -
// @brief Destroy the INA3221 object
//
 // - - - - - - - - - - - - - - - - - - - - -
 DEV_INA3221::~DEV_INA3221()
{
    return;
}


// - - - - - - - - - - - - - - - - - - - - -
// This is called to re-calculate the time
// between reading the 6 data channels.
//
// The interval is converted into 'ticks'
// @param forceNewInterval (default: true)
//     this is used if a parameter was changed
//     by a command.  It forces the subtask to 
//     do a fresh read (using the new values)
//
// - - - - - - - - - - - - - - - - - - - - -
time_t DEV_INA3221::updateSampleReadInterval(bool forceNewInterval)
{
    // What the new time interval should be
    taskENTER_CRITICAL(&dataReading_spinlock);
    sampleTimeUs = (noOfSamplesPerReading * sampleTimeUs * 6 * 1000) / portTICK_PERIOD_MS;
    taskEXIT_CRITICAL(&dataReading_spinlock);
    xTaskAbortDelay(readtask);

    return (sampleTimeUs);
}

// - - - - - - - - - - - - - - - - - - - - -
// @Brief - get the requested data value
//
// @param idx - the index of the data requested
// @param dta - pointer to where to store the data (format: float)
// @param timeStamp - pointer where to store the time stamp.
//         Note: Although SMAC uses a long timestamp, internally 
//         the RTOS time_t is defined as long long. We handle
//         this conversion internally
// - - - - - - - - - - - - - - - - - - - - -
void DEV_INA3221::getDataReading(int idx, float *dta, unsigned long int *timeStamp)
{
    taskENTER_CRITICAL(&dataReading_spinlock);
    *timeStamp = (unsigned long) dts_msec;
    *dta = dataReadings[idx];
    taskEXIT_CRITICAL(&dataReading_spinlock);
}

// - - - - - - - - - - - - - - - - - - - - -
// @brief This is run as a separate task.
//   this is where we get the voltage and current
// readings from the INA3221
// - - - - - - - - - - - - - - - - - - - - -
void DEV_INA3221::readDataTask(void *arg)
{
    DEV_INA3221 *me=(DEV_INA3221 *) arg;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();
    float tmpValues[6];

    while (true)
    {   // do forever

        // we are ready to read - do it!
        TAKE_I2C;
        for (int idx = 0; idx < 3; idx++)
        {
            tmpValues[idx]   = me->getBusVoltage(idx);
            tmpValues[idx+3] = me->getCurrentAmps(idx+3) * 1000; // convert to ma
        }
        GIVE_I2C;

        taskENTER_CRITICAL(&dataReading_spinlock);
        me->timestamp = esp_timer_get_time()/1000;
        taskEXIT_CRITICAL(&dataReading_spinlock);

        // wait a while for the next reading
        TickType_t ticksToWait= pdMS_TO_TICKS(me->sampleReadInterval/1000);
        xTaskDelayUntil( &xLastWakeTime, ticksToWait );
    }
}

// - - - - - - - - - - - - - - - - - - - - -
// Handle any SMAC commands 
// FORMAT: GPOW   ( get all 6 current values)
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus  DEV_INA3221::ExecuteCommand () 
{
    ProcessStatus retVal=SUCCESS_NODATA;
    DataPacket.timestamp=millis();
    retVal = ExecuteCommand();
    if (retVal == NOT_HANDLED)
    {
        scanParam();
        if (isCommand("SPOW"))
        {    // get all 6 values
            retVal=gpowerCommand();

        } else if (isCommand("STIM"))
        {  // Set the time per sample
            retVal=setTimePerSampleCommand();

        } else if (isCommand("SAVG"))
        {
            retVal=setAveragingModeCommand();
        } else 
        { 
            sprintf(DataPacket.value, "ERROR: Unknown command");
            retVal=FAIL_DATA;
        }
    }
 
    if (retVal==SUCCESS_NODATA)
    {        
        sprintf(DataPacket.value, "OK");
        retVal=SUCCESS_DATA;
    }
    if (DataPacket.timestamp == 0) DataPacket.timestamp = millis();
    return(retVal);
}


/**
 * Report the current values of all three channels
 * Format: 
 *       sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
 *         busVolt[0], current[0], busVolt[1], current[1], busVolt[2], current[2]);

 */
ProcessStatus DEV_INA3221::gpowerCommand()
{
    DataPacket.timestamp = millis();
    taskENTER_CRITICAL(&dataReading_spinlock);
    sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
        dataReadings[0], dataReadings[1], dataReadings[2], dataReadings[3], 
        dataReadings[4], dataReadings[5]);
    taskEXIT_CRITICAL(&dataReading_spinlock);
    return(SUCCESS_DATA);
}


/**
 * @brief Set the Averaging Mode (how many to average?)
 *   FORMAT: SAVG|<code>
 *      code is one of the following:
 *         1, 4, 16, 64, 128, 256, 512, 1024
 * @return ProcessStatus 
 */
ProcessStatus DEV_INA3221::setAveragingModeCommand()
{
    ProcessStatus retVal = SUCCESS_NODATA;
    uint8_t timeCode = 0;
    if (argCount != 1)
    {
        sprintf(DataPacket.value, "ERROR: Missing (or too many) arguments to SAVG command");
        retVal = FAIL_DATA;
    }
    else
    {
        retVal = getUInt8(0, &timeCode, "Number to Average:");
        if (retVal == SUCCESS_NODATA)
            retVal = setAvgCount(timeCode);
        if (retVal == SUCCESS_NODATA)
        {
            sprintf(DataPacket.value, "OK");
            retVal = SUCCESS_DATA;
        }
    }

    DataPacket.timestamp = millis();
    return (retVal);
}

/*
 * @brief how many samples to average?
 * FORMAT:   SAVR|<value>
 *     Value is one of the following:
 *         1, 4, 16, 64, 128, 256, 512, 1024
 */
ProcessStatus DEV_INA3221::setAvgCount(int noOfSamples)
{
    ProcessStatus retVal=SUCCESS_NODATA;
    uint8_t val;
    // bool setAveragingMode(ina3221_avgmode mode);
    TAKE_I2C;
    if (val == 1)
    {
        setAveragingMode(INA3221_AVG_1_SAMPLE);
        noOfSamplesPerReading = 1;
    }
    else if (val == 4)
    {
        setAveragingMode(INA3221_AVG_4_SAMPLES);
        noOfSamplesPerReading = 4;
    }
    else if (val == 16)
    {
        setAveragingMode(INA3221_AVG_16_SAMPLES);
        noOfSamplesPerReading = 16;
    }
    else if (val == 64)
    {
        setAveragingMode(INA3221_AVG_64_SAMPLES);
        noOfSamplesPerReading = 64;
    }
    else if (val == 128)
    {
        setAveragingMode(INA3221_AVG_128_SAMPLES);
        noOfSamplesPerReading = 128;
    }
    else if (val == 256)
    {
        setAveragingMode(INA3221_AVG_256_SAMPLES);
        noOfSamplesPerReading = 256;
    }
    else if (val == 512)
    {
        setAveragingMode(INA3221_AVG_512_SAMPLES);
        noOfSamplesPerReading = 512;
    }
    else if (val == 1024)
    {
        setAveragingMode(INA3221_AVG_1024_SAMPLES);
        noOfSamplesPerReading = 1024;
    }
    else
    {
        DataPacket.timestamp = millis();
        sprintf(DataPacket.value, "ERROR:  Must be one of 1,4,16,64,128,256,512,1024");
        retVal = FAIL_DATA;
    }
    GIVE_I2C;
    if (retVal == SUCCESS_NODATA)
    {
        updateSampleReadInterval();
        DataPacket.timestamp = millis();
        sprintf(DataPacket.value, "OK");
        retVal = SUCCESS_DATA;
    }
    return(retVal);
}


/**
 * @brief Set the Time Per Sample command
 *    
 * @return ProcessStatus 
 */
ProcessStatus DEV_INA3221::setTimePerSampleCommand()
{
  ProcessStatus retVal = SUCCESS_NODATA;
    uint8_t timePerSampleCode = 0;
    if (argCount != 1)
    {
        sprintf(DataPacket.value, "ERROR: Missing (or too many) arguments");
        retVal = FAIL_DATA;
    }
    else
    {
        retVal = getUInt8(0, &timePerSampleCode, "Code for timePerSample:");
        if (retVal == SUCCESS_NODATA)
            retVal = setConvTime(timePerSampleCode);
        if (retVal == SUCCESS_NODATA)
        {
            sprintf(DataPacket.value, "OK");
            retVal = SUCCESS_DATA;
        }
    }

    DataPacket.timestamp = millis();
    return (retVal);
}

/**
 * @brief how long should conversion time take?
 * 
 * Format: SBUS|<time>
 *    time is one of the following:
 *        140 (uSecs)   204 (uSecs) 332 (uSecs)
 *          1 (mSec)      2 (mSecs)  4 (mSecs) 8 (secs)
 *
 */
ProcessStatus DEV_INA3221::setConvTime(int _time)
{
    ProcessStatus retVal = SUCCESS_NODATA;
    uint8_t val = 0;

    TAKE_I2C;
    if (val == 140)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_140US);
        setShuntVoltageConvTime(INA3221_CONVTIME_140US);
        sampleTimeUs = 140;
    }
    else if (val == 204)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_204US);
        setShuntVoltageConvTime(INA3221_CONVTIME_204US);
        sampleTimeUs = 204;
    }
    else if (val == 332)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_332US);
        setShuntVoltageConvTime(INA3221_CONVTIME_332US);
        sampleTimeUs = 332;
    }
    else if (val == 588)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_588US);
        setShuntVoltageConvTime(INA3221_CONVTIME_588US);
        sampleTimeUs = 588;
    }
    else if (val == 1)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_1MS);
        setShuntVoltageConvTime(INA3221_CONVTIME_1MS);
        sampleTimeUs = 1000;
    }
    else if (val == 2)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_2MS);
        setShuntVoltageConvTime(INA3221_CONVTIME_2MS);
        sampleTimeUs = 2000;
    }
    else if (val == 4)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_4MS);
        setShuntVoltageConvTime(INA3221_CONVTIME_4MS);
        sampleTimeUs = 4000;
    }
    else if (val == 8)
    {
        setBusVoltageConvTime(INA3221_CONVTIME_8MS);
        setShuntVoltageConvTime(INA3221_CONVTIME_8MS);
        sampleTimeUs = 8000;
    }
    else
    {
        retVal = FAIL_DATA;
        sprintf(DataPacket.value, "ERROR: Convert time must be 140, 204, 332, 588, 1, 2, 4, 8");
    }
    GIVE_I2C;
    
    if (retVal == SUCCESS_NODATA)
    {
        updateSampleReadInterval();
        retVal = SUCCESS_DATA;
        sprintf(DataPacket.value, "OK");
    }
    DataPacket.timestamp = millis();
    return (retVal);
}

