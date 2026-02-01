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
//
// - - - - - - - - - - - - - - - - - - - - -
#include "SMAC/Node.h"
#include "DEV_INA3221.h"
#include "cmath"
#include "esp_log.h"
#include "config.h"
#include "strings.h"
#include "Util.h"

// #define DEBUG_DEV_INA3221

// Static initializer
portMUX_TYPE DEV_INA3221::INA3221_Data_Access_Spinlock = portMUX_INITIALIZER_UNLOCKED;


// - - - - - - - - - - - - - - - - - - - - -
// @brief Construct a new INA3221Device object
//
// @param inName   - name of this device
// @param _i2CAddr - adress on the I2C bus of the IAN3221
// @param theWire  - pointer to the 'Wire' class instance to use for I2C communication.
// - - - - - - - - - - - - - - - - - - - - -
DEV_INA3221::DEV_INA3221(const char *inName, int _i2CAddr,  TwoWire *theWire) : Device(inName)
{    
    // Device default condition
    initStatusOk=false;
    strncpy(version, INA3221Version, MAX_VERSION_LENGTH);
    immediateEnabled = false;
    periodicEnabled = false;
    readCounter = 0;
    SetRate(900);     // default reporting rate (every 4 secs)for this device

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


 // Start the read task, configure the INA3221
    ESP_ERROR_CHECK(xTaskCreate(readDataTask, "ReadINA3221", 4096, this, 3, &readtask));
    for (uint8_t idx = 0; idx < 3; idx++)
    {
        TAKE_I2C;
        setShuntResistance(idx, 0.05);
        GIVE_I2C;
    }
    noOfSamplesPerReading=16;
    sampleTimeUs=5000;
    updateSampleReadInterval(5000); // Default 5000 msecs (5 Second).
    Serial.printf("INITIAL SAMPLE TIME IS %d ticks\r\n", sampleReadIntervalMs);
    initStatusOk = true;
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
// @brief Set a new read interval.
//   This notifies the read task that an update
// happened
// - - - - - - - - - - - - - - - - - - - - -
time_t DEV_INA3221::updateSampleReadInterval( time_t timeInMsecs)
{
    taskENTER_CRITICAL(&INA3221_Data_Access_Spinlock);
    sampleReadIntervalMs = timeInMsecs;  
    taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);


    Serial.printf ("***In updateSampleReadInterval - new update interval is %d Msecs\r\n", sampleReadIntervalMs);

    xTaskAbortDelay(readtask);  // tell our subtask to use the new time period

    return (sampleReadIntervalMs);
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
    taskENTER_CRITICAL(&INA3221_Data_Access_Spinlock);
    *timeStamp = (unsigned long) dts_msec;
    *dta = dataReadings[idx];
    taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);
}

// - - - - - - - - - - - - - - - - - - - - -
// @brief This is run as a separate task (It is Static!)
//   this is where we get the voltage and current   
// readings from the INA3221.
// - - - - - - - - - - - - - - - - - - - - -
void DEV_INA3221::readDataTask(void *arg)
{
    DEV_INA3221 *me=(DEV_INA3221 *) arg;

    float tmpValues[6]= {};
    uint8_t idx=0;
    time_t xLastWakeTime;  // when we last woke up
    bool wasDelayedFlag=false; 

    while (true)
    {   // do forever
        #ifdef DEBUG_DEV_INA3221
        Serial.println("***READ NEW DATA VALUES");
        #endif
        xLastWakeTime = esp_timer_get_time();
        me->readCounter++;

        // we are ready to read - do it!
        TAKE_I2C;  // Using I2C - this can take a while...
        for (idx = 0; idx < 3; idx++)
        {           
            tmpValues[idx]   = me->getBusVoltage(idx);  // In volts
            tmpValues[idx+3] = me->getCurrentAmps(idx); // In amps
        }
        GIVE_I2C;

        // Now update our internal memory with the new values
        taskENTER_CRITICAL(&INA3221_Data_Access_Spinlock);
        for (idx=0; idx<6; idx++)
        {
            me->dataReadings[idx] = tmpValues[idx];
        }
        me->dts_msec = esp_timer_get_time()/1000;
        taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);

        // wait a while for the next reading
        TickType_t waitTimeMs = me->sampleReadIntervalMs - ((esp_timer_get_time() - xLastWakeTime) / 1000);
        vTaskDelay(portTICK_PERIOD_MS * waitTimeMs);
    }
}

/**
 * @brief show all the current data values, and the count of samples
 * 
 *  FORMAT:   <readCount>|volt[0], volt[1], volt[2], current[0], current[1], current[2]
 */
ProcessStatus DEV_INA3221::DoPeriodic()
{
    float tmp[6];
    unsigned long long tmpCount;
    time_t timeStamp;
    taskENTER_CRITICAL(&INA3221_Data_Access_Spinlock);
    for (int i=0; i<6; i++)
    {
        tmp[i] = dataReadings[i];
    }
    tmpCount = readCounter;
    timeStamp = dts_msec;
    taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);

    sprintf(SMACData.values, "%llu|%f|%f|%f|%f|%f|%f",tmpCount,
         tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
    return(WIDGET_DATA);
}


// - - - - - - - - - - - - - - - - - - - - -
// Handle any SMAC commands 
// FORMAT: GPOW   ( get all 6 current values)
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus  DEV_INA3221::ExecuteCommand (char *command, char *params) 
{
    ProcessStatus retVal=NOT_HANDLED;

    retVal = Device::ExecuteCommand(command, params);
    if (retVal == NOT_HANDLED)
    {
        if (0 == strcasecmp(command, "STIM"))
        { // Set the time per sample (ms)
            retVal = setTimePerSampleCommand(command, params);
        }
        else if (0 == strcasecmp(command, "SAVG"))
        {
            retVal = setAveragingModeCommand(command, params);
        }
        else if (0 == strcasecmp(command, "RATE"))
        {
            retVal = setSampleRateCommand(command, params);
        }
    }
    return (retVal);
}

/**
 * @brief Set the Averaging Mode (how many to average?)
 *   FORMAT: SAVG|<code>
 *      code is one of the following:
 *         1, 4, 16, 64, 128, 256, 512, 1024
 * @return ProcessStatus 
 */
ProcessStatus DEV_INA3221::setAveragingModeCommand(char *command, char *params)
{
    ProcessStatus retVal = NOT_HANDLED;
    int notoaverage = -1;

    if ((params == nullptr) || (WIDGET_DATA == Util::getint_t(params, &notoaverage, "Average mode")))
    {
        retVal = SYSTEM_DATA;
        sprintf(SMACData.values, "EROR - missing arguments (or argument not a valid integer) to SAVG command");
    }
    else
    {
        retVal = setAvgCount(notoaverage);
    }
    return (retVal);
}

/*
 * @brief how many samples to average?

 *     Value MUST be is one of the following:
 *         1, 4, 16, 64, 128, 256, 512, 1024
 *  return: The update interval is re-calculated.
 */
ProcessStatus DEV_INA3221::setAvgCount(int val)
{
    ProcessStatus retVal=NODATA;

    #ifdef DEBUG_DEV_INA3221
    Serial.printf("***setAvgCount - argument is %d\r\n", val);
    #endif
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
        sprintf(SMACData.values, "ERROR: Count Must be one of 1,4,16,64,128,256,512,1024. arg=%d", val);
        retVal = SYSTEM_DATA;
    }
    GIVE_I2C;

    return(retVal);
}


/**
 * @brief Set (or get) the Time Per Sample.
 *   FORMAT:  STIM <timeInMs>
 *  total time for each sample (in uSecs). This is an INA3221
 *  value, and is limited to specific values (see setConvTime)
 *
 * @return ProcessStatus
 */
ProcessStatus DEV_INA3221::setTimePerSampleCommand(char *commaand, char *params)
{
    ProcessStatus retVal = NODATA;
    int time_val = 0;
    if ((params == nullptr))
    {
        sprintf(SMACData.values, "ERROR- Missing argument to STIM command");
        retVal = SYSTEM_DATA;
    }
    else if (NODATA == (retVal = Util::getint_t(params, &time_val, "time value")))
    {
        retVal = setConvTime(time_val);
        retVal = NODATA;
    }

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
ProcessStatus DEV_INA3221::setConvTime(int val)
{
    ProcessStatus retVal = NODATA;
    
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
        retVal = SYSTEM_DATA;
        sprintf(SMACData.values, "ERROR: Convert time must be 140, 204, 332, 588, 1, 2, 4, 8");
        #ifdef DEBUG_DEV_INA3221
        Serial.printf( "ERROR: Convert time must be 140, 204, 332, 588, 1, 2, 4, 8. value seen = %d\r\n",val);
        #endif
    }
    GIVE_I2C;
    
    return (retVal);
}


/**
 * @brief command to set how often samples are taken?
 *  FORMAT:  <SRAT>|<time>
 *     <time> is in milliseconds (limit 32767)
 */
ProcessStatus DEV_INA3221::setSampleRateCommand(char *command, char *params)
{
    ProcessStatus retVal = NODATA;

    time_t newRate = 0;
    if (params == nullptr)
    {   
        sprintf(SMACData.values, "ERROR - missing argument for SRAT command");
        retVal = SYSTEM_DATA;

    } else if (NODATA ==  ( retVal = Util::getLL_t(0, &newRate, "Sample Rate:") ) )
    {
        updateSampleReadInterval(newRate);
    }

    return(retVal);
}