/**
 * @file INA3221.cpp
 * @author Doug Fajardo
 * @brief SMAC driver for INA3221 tripple Power monitoring device
 * @version 1.0
 * @date 2025-07-07
 * 
 * Written by Doug Fajardo Jully, 2025
 *              This code was developed as part of the SMAC project by Bill Daniels,
 *              and all rights and copyrights are hereby conveyed to that project.
 * 
 *              The SMAC project is Copyright 2021-2025, D+S Tech Labs, Inc.
 *              All Rights Reserved
 * 
 *  See INA3221Device.h for implementation and usage notes
 */
#include "INA3221Device.h"

        /** - - - - - - - - - - - - - - - - - - - - -
         * @brief Construct a new INA3221 object
         * 
         * @param _node 
         * @param InName 
         */
        INA3221Device::INA3221Device( const char * inName): Device(inName)
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
                    busVolt[0], current[0], busVolt[1], current[1], busVolt[2], current[2]);

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
            return (retVal);
        }


        