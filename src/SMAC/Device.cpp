//=========================================================
//
//     FILE : Device.cpp
//
//  PROJECT : SMAC Framework
//              │
//              └── Firmware
//                    │
//                    └── Node
//
//    NOTES : Device base class:
//            Derive custom Devices from this class.
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

#include <Arduino.h>
#include "Device.h"

//--- Constructor -----------------------------------------

Device::Device (const char *inName)
{
  // Set the UI name for this device
  strncpy (name, inName, MAX_NAME_LENGTH-1);
  name[MAX_NAME_LENGTH-1] = 0;
}

//--- SetID -----------------------------------------------

void Device::SetID (int inDeviceID)
{
  // Set the ID for this Device (00 - 99)
  if (inDeviceID > 99)
    inDeviceID = 99;

  sprintf (deviceID, "%02d", inDeviceID);
}

//--- GetID -----------------------------------------------

const char * Device::GetID ()
{
  return deviceID;
}

//--- GetName ---------------------------------------------

const char * Device::GetName ()
{
  return name;
}

//--- IsIPEnabled -----------------------------------------

bool Device::IsIPEnabled ()
{
  return immediateEnabled;
}

//--- IsPPEnabled -----------------------------------------

bool Device::IsPPEnabled ()
{
  return periodicEnabled;
}

//--- GetRate ---------------------------------------------

unsigned long Device::GetRate ()
{
  return 3600000L / processPeriod;  // Calls to DoPeriodic() per hour
}

//--- SetRate ---------------------------------------------

void Device::SetRate (double newRate)
{
  // Do not allow zero rate
  if (newRate < 1.0)
    newRate = 1.0;

  processPeriod = (unsigned long)(3600000.0/newRate + 0.5);
}

//--- RunPeriodic -----------------------------------------

ProcessStatus Device::RunPeriodic ()
{
  // Do not override this method.
  // It is continually called by the Node to operate periodic processing.

  // Is it time to do the periodic process?
  now = millis();
  if (now >= nextPeriodicTime)
  {
    nextPeriodicTime = now + processPeriod;
    return DoPeriodic ();
  }

  return SUCCESS_NODATA;
}

//--- DoImmediate -----------------------------------------

ProcessStatus Device::DoImmediate ()
{
  // Override this method in your child class to perform
  // a continuous (as fast as possible) process.
  //
  // If there is data to return, then this method should populate this Device's
  // timestamp and value strings of the global <DataPacket> structure
  // and return SUCCESS_DATA or FAIL_DATA.

  return SUCCESS_NODATA;
}

//--- DoPeriodic ------------------------------------------

ProcessStatus Device::DoPeriodic ()
{
  // Override this method in your child class to perform
  // a timed periodic process.
  //
  // If there is data to return, then this method should populate this Device's
  // timestamp and value strings of the global <DataPacket> structure
  // and return SUCCESS_DATA or FAIL_DATA.

  return SUCCESS_NODATA;
}

//--- ExecuteCommand --------------------------------------

ProcessStatus Device::ExecuteCommand ()
{
  // If your child Device class needs to handle custom commands, then override this method:
  //
  // The global <CommandPacket> will have the command definition.
  // First call this base class method to handle the built-in Device commands:
  //
  //   Device::ExecuteCommand ();
  //
  // If this call returns NOT_HANDLED, then your child class should handle the command.
  //
  // If your ExecuteCommand() method has data to return, it should populate the
  // global <DataPacket> and return an appropriate ProcessStatus.
  //
  // When populating the global <DataPacket>, value strings that start with a dash or a digit
  // will be interpreted by the Interface as periodic process data, say from a sensor reading.

  // Initial Process Status
  pStatus = NOT_HANDLED;

  //--- Get Device Name (GDNA) ------------------
  if (strcmp (CommandPacket.command, "GDNA") == 0)
  {
    // Return Device's name
    strcpy (DataPacket.value, "DENAME=");
    strcat (DataPacket.value, name);

    pStatus = SUCCESS_DATA;
  }

  //--- Set Device Name (SDNA) ------------------
  else if (strcmp (CommandPacket.command, "SDNA") == 0)
  {
    // Set this Device's name
    strncpy (name, CommandPacket.params, MAX_NAME_LENGTH-1);
    name[MAX_NAME_LENGTH-1] = 0;

    // Acknowledge new name
    strcpy (DataPacket.value, "DENAME=");
    strcat (DataPacket.value, name);

    pStatus = SUCCESS_DATA;
  }

  //--- Enable Immediate Processing (ENIP) ------
  else if (strcmp (CommandPacket.command, "ENIP") == 0)
  {
    immediateEnabled = true;

    // Acknowledge
    strcpy (DataPacket.value, "IP Enabled");

    pStatus = SUCCESS_DATA;
  }

  //--- Disable Immediate Processing (DIIP) -----
  else if (strcmp (CommandPacket.command, "DIIP") == 0)
  {
    immediateEnabled = false;

    // Acknowledge
    strcpy (DataPacket.value, "IP Disabled");

    pStatus = SUCCESS_DATA;
  }

  //--- Do Immediate Process one time (DOIP) ----
  else if (strcmp (CommandPacket.command, "DOIP") == 0)
    return DoImmediate ();

  //--- Enable Periodic Processing (ENPP) -------
  else if (strcmp (CommandPacket.command, "ENPP") == 0)
  {
    periodicEnabled = true;
    nextPeriodicTime = millis();

    // Acknowledge
    strcpy (DataPacket.value, "PP Enabled");

    pStatus = SUCCESS_DATA;
  }

  //--- Disable Periodic Processing (DIPP) ------
  else if (strcmp (CommandPacket.command, "DIPP") == 0)
  {
    periodicEnabled = false;

    // Acknowledge
    strcpy (DataPacket.value, "PP Disabled");

    pStatus = SUCCESS_DATA;
  }

  //--- Do Periodic Process one time (DOPP) -----
  else if (strcmp (CommandPacket.command, "DOPP") == 0)
    return DoPeriodic ();

  //--- Get Rate (GRAT) -------------------------
  else if (strcmp (CommandPacket.command, "GRAT") == 0)
  {
    // Return this Device's current periodic process rate (calls per hour)
    strcpy (DataPacket.value, "RATE=");
    ltoa (GetRate(), DataPacket.value + 5, 10);

    pStatus = SUCCESS_DATA;
  }

  //--- Set Rate (SRAT) -------------------------
  else if (strcmp (CommandPacket.command, "SRAT") == 0)
  {
    // Set this Device's periodic process rate (calls per hour)
    double newRate = atof (CommandPacket.params);
    SetRate (newRate);

    // Acknowledge new periodic rate
    strcpy (DataPacket.value, "RATE=");
    ltoa (GetRate(), DataPacket.value + 5, 10);

    nextPeriodicTime = millis();  // start new rate now
    pStatus = SUCCESS_DATA;
  }

  // Set timestamp if data to return
  if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
    DataPacket.timestamp = millis();

  // Return the resulting ProcessStatus
  return pStatus;
}
