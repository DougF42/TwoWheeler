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
DEV_INA3221::INA3221DeviceChannel::INA3221DeviceChannel(const char *inName, DEV_INA3221 *_me, int _dataPtNo) :
     DefDevice(inName)
{
    me = _me;
    dataPointNo = _dataPtNo;
    immediateEnabled = false;
    periodicEnabled = false;
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
    initStatusOk=false;
    strncpy(version, INA3221Version, MAX_VERSION_LENGTH);
    version[MAX_VERSION_LENGTH-1]=0x00;
    immediateEnabled = false;
    periodicEnabled = false;
    SetRate(900);     // default reporting rate (every 4 secs)for this device (note periodicEnabled=false!)

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

    myNode->AddDevice(new INA3221DeviceChannel("Volt0",   this, 0));
    myNode->AddDevice(new INA3221DeviceChannel("Volt1",   this, 1));
    myNode->AddDevice(new INA3221DeviceChannel("Volt2",   this, 2));
    myNode->AddDevice(new INA3221DeviceChannel("Current0", this, 3));
    myNode->AddDevice(new INA3221DeviceChannel("Current1", this, 4));
    myNode->AddDevice(new INA3221DeviceChannel("Current2", this, 5));

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
    updateSampleReadInterval(1000); // Default (for now) 1 Second.
    Serial.printf("INITIAL SAMPLE TIME IS %d ticks\r\n", sampleReadIntervalTicks);
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
    sampleReadIntervalTicks = pdMS_TO_TICKS( timeInMsecs);    
    taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);


    Serial.printf ("***In updateSampleReadInterval - new update interval is %d ticks\r\n", sampleReadIntervalTicks);

    xTaskAbortDelay(readtask);  // tell our subtask to use the new time period

    return (sampleReadIntervalTicks);
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
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();
    float tmpValues[6]= {};
    uint8_t idx=0;

    while (true)
    {   // do forever
        #ifdef DEBUG_DEV_INA3221
        Serial.println("***READ NEW DATA VALUES");
        #endif
        me->readCounter++;
        // we are ready to read - do it!
        TAKE_I2C;  // Using I2C - this can take a while...
        for (idx = 0; idx < 3; idx++)
        {           
            tmpValues[idx]   = me->getBusVoltage(idx);
            tmpValues[idx+3] = me->getCurrentAmps(idx) * 1000; // convert to ma
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
        #ifdef DEBUG_DEV_INA3221
         Serial.print("***In readDataTask: ticks to wait = ");
         Serial.println(me->sampleReadIntervalTicks);
        #endif
        xTaskDelayUntil( &xLastWakeTime, me->sampleReadIntervalTicks );
    }
}

/**
 * @brief show all the current data values, and the count of samples
 * 
 *  FORMAT:   INAX|<readCount>|volt[0], volt[1], volt[2], current[0], current[1], current[2]
 */
ProcessStatus DEV_INA3221::DoPeriodic()
{
    float tmp[6];
    unsigned long long tmpCount;
    taskENTER_CRITICAL(&INA3221_Data_Access_Spinlock);
    for (int i=0; i<6; i++)
    {
        tmp[i] = dataReadings[i];
    }
    tmpCount = readCounter;
    taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);

    sprintf(DataPacket.value, "INAX|%d|%f|%f|%f|%f|%f|%f",tmpCount,
         tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);     
    return(SUCCESS_DATA);
}


// - - - - - - - - - - - - - - - - - - - - -
// Handle any SMAC commands 
// FORMAT: GPOW   ( get all 6 current values)
// - - - - - - - - - - - - - - - - - - - - -
ProcessStatus  DEV_INA3221::ExecuteCommand () 
{
    ProcessStatus retVal=SUCCESS_NODATA;
    DataPacket.timestamp=millis();
    retVal = Device::ExecuteCommand();
    if (retVal == NOT_HANDLED)
    {
        scanParam();
        if (isCommand("SPOW"))
        {    // get all 6 values
            retVal=gpowerCommand();

        } else if (isCommand("STIM"))
        {  // Set the time per sample (ms)
            retVal=setTimePerSampleCommand();

        } else if (isCommand("SAVG"))
        {
            retVal=setAveragingModeCommand();

        } else if (isCommand("RATE"))
        {
            retVal = setSampleRateCommand();
        
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
    taskENTER_CRITICAL(&INA3221_Data_Access_Spinlock);
    sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
        dataReadings[0], dataReadings[1], dataReadings[2], dataReadings[3], 
        dataReadings[4], dataReadings[5]);
    taskEXIT_CRITICAL(&INA3221_Data_Access_Spinlock);
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
    int notoaverage = 0;

    if (argCount == 1)
    {
        retVal = getInt(0, &notoaverage, "Number to Average:");
    }
    else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERROR: Missing (or too many) arguments to SAVG command");
        retVal = FAIL_DATA;
    }

    if ((argCount == 1) && (retVal == SUCCESS_NODATA))
    {
        retVal = setAvgCount(notoaverage);
    }

    if (retVal == SUCCESS_DATA)
    {
        Serial.printf(DataPacket.value, "SAVG|%d\r\n", noOfSamplesPerReading);
        retVal = SUCCESS_DATA;
    }

    DataPacket.timestamp = millis();
    return (retVal);
}

/*
 * @brief how many samples to average?

 *     Value is one of the following:
 *         1, 4, 16, 64, 128, 256, 512, 1024
 *  return: The update interval is re-calculated.
 */
ProcessStatus DEV_INA3221::setAvgCount(int val)
{
    ProcessStatus retVal=SUCCESS_NODATA;

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
        sprintf(DataPacket.value, "ERROR: Count Must be one of 1,4,16,64,128,256,512,1024. arg=%d", val);
        #ifdef DEBUG_DEV_INA3221
        Serial.println(DataPacket.value);
        #endif
        retVal = FAIL_DATA;
    }
    GIVE_I2C;

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "OK");
        retVal = SUCCESS_DATA;
    }
    DataPacket.timestamp = millis();
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
ProcessStatus DEV_INA3221::setTimePerSampleCommand()
{
  ProcessStatus retVal = SUCCESS_NODATA;
    int time_val = 0;
    if (argCount == 1)
    {
        retVal = getInt(0, &time_val, "Code for timePerSample:");
    } else if (argCount!=0)
    {
        sprintf(DataPacket.value, "ERROR: Missing (or too many) arguments");
        retVal = FAIL_DATA;
    }

    if (  (argCount==1) && (retVal == SUCCESS_NODATA))
            retVal = setConvTime(time_val);

    if (retVal == SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "STIM|%f", sampleTimeUs);
        retVal = SUCCESS_DATA;
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
ProcessStatus DEV_INA3221::setConvTime(int val)
{
    ProcessStatus retVal = SUCCESS_NODATA;

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
        #ifdef DEBUG_DEV_INA3221
        Serial.printf( "ERROR: Convert time must be 140, 204, 332, 588, 1, 2, 4, 8. value seen = %d\r\n",val);
        #endif
    }
    GIVE_I2C;
    
    if (retVal == SUCCESS_NODATA)
    {
        retVal = SUCCESS_DATA;
        sprintf(DataPacket.value, "OK");
    }
    DataPacket.timestamp = millis();
    return (retVal);
}


/**
 * @brief command to set how often samples are taken?
 *  FORMAT:  <SRAT>|<time>
 *     <time> is in milliseconds (limit 32767)
 */
ProcessStatus DEV_INA3221::setSampleRateCommand()
{
    ProcessStatus retVal = SUCCESS_NODATA;

    time_t newRate = 0;
    if (argCount == 1)
    {
        retVal = getLLint(0, &newRate, "Sample Rate:");
    }
    else if (argCount != 0)
    {
        sprintf(DataPacket.value, "ERROR: Missing (or too many) arguments");
        retVal = FAIL_DATA;
    }

    if (argCount==1)
    {
        updateSampleReadInterval(newRate);
    }

    if (retVal==SUCCESS_NODATA)
    {
        sprintf(DataPacket.value, "OK");
        retVal=SUCCESS_DATA;
    }

    return(retVal);
}