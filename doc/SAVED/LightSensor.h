//=========================================================
//
//     FILE : LightSensor.h
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

#ifndef LIGHTSENSOR_H
#define LIGHTSENSOR_H

//--- Includes --------------------------------------------

#include "Device.h"


//=========================================================
//  class LightSensor
//=========================================================

class LightSensor : public Device
{
  protected:
    int  sensorPin = 1;  // default pin

  public:
    LightSensor (const char *inName, int inSensorPin);

    ProcessStatus DoPeriodic ();  // override default processing
};

#endif
