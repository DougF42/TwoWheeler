//=========================================================
//
//     FILE : LightSensor.cpp
//
//  PROJECT : SMAC Framework - Example 1
//
//    NOTES : Analog light sensor using a photoresistor.
//            Derived from Device base class
//
//                             ~180Ω..1MΩ
//                    5KΩ           ░
//               ┌───█████───┬─────(Θ)─────┐
//               │           │             │
//               │           │             │
//               ^           ▼             ▼
//             3.3V         GPIO          GND
//                          pin
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

//--- Includes --------------------------------------------

#include <Arduino.h>
#include "LightSensor.h"

//--- Constructor -----------------------------------------

LightSensor::LightSensor (const char *inName, int inSensorPin)
            :Device (inName)
{
  // Init GPIO pin
  sensorPin = inSensorPin;
  pinMode (sensorPin, INPUT);

  // Default to 10 samples per second
  SetRate (10 * 3600);
}

//--- DoPeriodic (override) -------------------------------

ProcessStatus LightSensor::DoPeriodic ()
{
  // The "Periodic Process" for this device is simply to measure a sample
  int sample = 4095 - analogRead (sensorPin);

  // All data is returned by populating the global <DataPacket> structure
  // and returning an appropriate ProcessStatus.
  //
  // The <DataPacket> structure has three fields:
  //
  //   deviceID  : 2-char string (00-99)
  //   timestamp : timestamp of when the data was aquired - usually millis() converted to a string
  //   value     : variable length string (including NULL terminating char)
  //               (this can be a numerical value or a text message)

  // For the DoPeriodic() method, we only need to populate the timestamp
  // and value fields.  The deviceID is filled for us by the Node base class.
  DataPacket.timestamp = millis ();
  itoa (sample, DataPacket.value, 10);  // DataPacket.value must be a terminated string

  // DoPeriodic() must return one of four possible "ProcessStatus" values:
  //
  //   SUCCESS_DATA    - Process performed successfully, send DataPacket to Relayer (such as a sensor reading)
  //   SUCCESS_NODATA  - Process performed successfully, no data to send to Relayer
  //   FAIL_DATA       - Process failed, send DataPacket to Relayer (such as error code or message)
  //   FAIL_NODATA     - Process failed, no data to send to Relayer

  // For this example, we indicate a successful reading with data to send
  return SUCCESS_DATA;
}
