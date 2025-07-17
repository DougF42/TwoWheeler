/**
 * @file INA3221.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-07-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "INA3221Device.h"

        /** - - - - - - - - - - - - - - - - - - - - -
         * @brief Construct a new INA3221 object
         * 
         * @param _node 
         * @param InName 
         */
        INA3221Device::INA3221Device(Node *node, const char * inName): DefDevice(node, inName)
        {

        }

        /**  - - - - - - - - - - - - - - - - - - - -
         * @brief Destroy the INA3221 object
         * 
         */
        INA3221Device::~INA3221Device()
        {

        }

        /** - - - - - - - - - - - - - - - - - - - - -
         * @brief setup Set up the driver, initialize I2C
         * 
         * @param I2CAddr  - the I2C address for this device. Default 0x40, alternative 0x41
         * @return true    - normal startup okay
         * @return false   - error was detected
         */
        bool INA3221Device::setup(int _Pwr_i2CAddr, TwoWire *theWire)
        {
            i2cAddr = _Pwr_i2CAddr;
            periodicEnabled = true;  // Enable reporting for this module.
            SetRate(60);             // default report rate - 60 timers/hour is once per minute
            immediateEnabled = false; // Nothing to do 
            
            // Init communications with I2C
            if (! Adafruit_INA3221::begin(i2cAddr, theWire) ) 
            {
                Serial.println("Failed to find INA3221 chip");
                return(false);
            }

            setAveragingMode( INA3221_AVG_16_SAMPLES);

            for (uint8_t idx=0; idx<3; idx++)
            {
                setShuntResistance(idx, 0.05);
            }

            return(true);
        }

        // - - - - - - - - - - - - - - - - - - - - -
        // Override this method for processing your device continuously
        ProcessStatus INA3221Device::DoImmediate()
        {
            ProcessStatus retVal = NOT_HANDLED;
            return (retVal);
        }

        // - - - - - - - - - - - - - - - - - - - - -
        // Report the battery voltage and current readings
        // - - - - - - - - - - - - - - - - - - - - -
        ProcessStatus INA3221Device::DoPeriodic()
        {
            float busVolt[3];
            float current[3];

            for (int idx = 0; idx < 3; idx++)
            {
                busVolt[idx] = getBusVoltage(idx);
                current[idx] = getCurrentAmps(idx) * 1000; // convert to ma
            }

            sprintf(DataPacket.value, "BATX|%f|%f|%f|%f|%f|%f",
                    busVolt[1], current[1], busVolt[2], current[2], busVolt[3], current[3]);

            return (SUCCESS_DATA);
        }

        /** - - - - - - - - - - - - - - - - - - - - -
         * @brief Execute commands specific to this device
         *
         *
         * @return ProcessStatus
         */
        ProcessStatus INA3221Device::ExecuteCommand()
        {
            ProcessStatus retVal = NOT_HANDLED;
            retVal = Device::ExecuteCommand();
            if (retVal != NOT_HANDLED)
                return (retVal);

            scanParam();
            if (isCommand("PENA"))
            {  // Enable one of the channels
                retVal= enableChannelCmd();

            } else if (isCommand("PDIS"))
            {
                // Disable one of the channels
                retVal= disableChannelCmd();                

            } else {
                sprintf(DataPacket.value, "ERR|Unknown command for IAN3221 driver");
                retVal=FAIL_DATA;
            }

            return (retVal);
        }


        /** - - - - - - - - - - - - - - - - - - - - -
         * @brief Enable one of the power channels - and report the status of all
         *   FORMAT: PENA <channel> 
         *      (channel is 0, 1 or 2)
         * 
         * @return ProcessStatus 
         */
        ProcessStatus INA3221Device::enableChannelCmd()
        {
            ProcessStatus retVal=NOT_HANDLED;
            uint8_t channel;
            if (! getUInt8(0, &channel, "Channel number") )
            {
                defDevSendData(0,false);
                retVal=FAIL_DATA;

            } else  if (channel>3)
            {
                sprintf(DataPacket.value,"PENA|ERR|Channel must be 0, 1 or 2");

                retVal=FAIL_DATA;

            } else 
            {
                channelEnaFlag[channel]=true;
                retVal=SUCCESS_DATA;
            }
        
            if (retVal == SUCCESS_NODATA)
            {
                sprintf(DataPacket.value, "PENA|%d", 
                    (channelEnaFlag[0]) ? "ENA":"DIS",
                    (channelEnaFlag[1]) ? "ENA":"DIS",
                    (channelEnaFlag[2]) ? "ENA":"DIS");
                retVal=SUCCESS_DATA;
            }
            
            defDevSendData(0, false);
            return(retVal);
        }
        

        /** - - - - - - - - - - - - - - - - - - - - -
         * @brief  disableChannelCmd    command to disable a channel
         *  FORMAT: PDIS <channel> 
         *      (channel is 0, 1 or 2)
         * @return ProcessStatus 
         */
        ProcessStatus INA3221Device::disableChannelCmd()
        {
            ProcessStatus retVal=NOT_HANDLED;
            uint8_t channel;
            if (! getUInt8(0, &channel, "Channel number") )
            {
                defDevSendData(0,false);
                retVal = FAIL_DATA;

            } else if (channel>3)
            {
                sprintf(DataPacket.value,"PDIS|ERR|Channel must be 0, 1 or 2");

                retVal=FAIL_DATA;

            } else {
                channelEnaFlag[channel]=true;
                retVal=SUCCESS_NODATA;
            }

            if (retVal == SUCCESS_NODATA)
            {
                sprintf(DataPacket.value, "PENA|%d", 
                    (channelEnaFlag[0]) ? "ENA":"DIS",
                    (channelEnaFlag[1]) ? "ENA":"DIS",
                    (channelEnaFlag[2]) ? "ENA":"DIS");
            }

            defDevSendData(0, false);
            return(retVal);
        }

        