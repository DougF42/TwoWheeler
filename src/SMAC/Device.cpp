//=========================================================
//
//     FILE : Device.cpp
//
//  PROJECT : SMAC Framework
//
//    NOTES : Device base class:
//            Derive custom Devices from this class.
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2026, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

#include <Arduino.h>
#include "Device.h"
#include "Node.h"

//--- Constructor -----------------------------------------

Device::Device (const char *inName)
{
  // Set the UI name for this device
  strncpy (name, inName, MAX_NAME_LENGTH-1);
  name[MAX_NAME_LENGTH-1] = 0;

  // Names cannot have commas
  for (int i=0; i<strlen(name); i++)
    if (name[i] == ',') name[i] = '.';

  // Set version
  strcpy (version, "3.1");  // no more than 9 chars
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

char * Device::GetID ()
{
  return deviceID;
}

//--- SetNode ---------------------------------------------

void Device::SetNode (Node *parentNode)
{
  // Set the Node object for this Device
  node = parentNode;
}

//--- GetName ---------------------------------------------

char * Device::GetName ()
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

//--- GetVersion ------------------------------------------

char * Device::GetVersion ()
{
  return version;
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

  return NODATA;
}

//--- DoImmediate -----------------------------------------

IRAM_ATTR ProcessStatus Device::DoImmediate ()
{
  // Override this method in your child class to perform
  // a continuous (as fast as possible) process.
  //
  // If there is data to return, then this method should populate this Device's
  // values string of the global <SMACData> structure and return a ProcessStatus.

  return NODATA;
}

//--- DoPeriodic ------------------------------------------

IRAM_ATTR ProcessStatus Device::DoPeriodic ()
{
  // Override this method in your child class to perform
  // a timed periodic process.
  //
  // If there is data to return, then this method should populate this Device's
  // values string of the global <SMACData> structure and return a ProcessStatus.

  return NODATA;
}

//--- ExecuteCommand --------------------------------------

ProcessStatus Device::ExecuteCommand (char *command, char *params)
{
  // If your child Device class needs to handle custom commands, then override this method:
  //
  // First call this base class method to handle the built-in Device commands:
  //
  //   Device::ExecuteCommand (command, params);
  //
  // If this call returns NOT_HANDLED, then your child class should handle the command.
  //
  // If your ExecuteCommand() method has data to return, it should populate the
  // global SMACData.values field and return an appropriate ProcessStatus.
  //
  // If SMACData.values starts with a dash or a digit, then the Interface will
  // interpret it as data, say from a sensor reading.
  //
  // Multiple values can be separated with commas.

  // Initial Process Status
  pStatus = NOT_HANDLED;

  //--- Get Device Name (GDNA) ------------------
  if (strcmp (command, "GDNA") == 0)
  {
    // Return Device's name
    strcpy (SMACData.values, "DENAME=");
    strcat (SMACData.values, name);

    pStatus = SYSTEM_DATA;
  }

  //--- Set Device Name (SDNA) ------------------
  else if (strcmp (command, "SDNA") == 0)
  {
    // Set this Device's name
    strncpy (name, params, MAX_NAME_LENGTH-1);
    name[MAX_NAME_LENGTH-1] = 0;

    // Acknowledge new name
    strcpy (SMACData.values, "DENAME=");
    strcat (SMACData.values, name);

    pStatus = SYSTEM_DATA;
  }

  //--- Enable Immediate Processing (ENIP) ------
  else if (strcmp (command, "ENIP") == 0)
  {
    immediateEnabled = true;

    // Acknowledge
    strcpy (SMACData.values, "IP Enabled");

    pStatus = SYSTEM_DATA;
  }

  //--- Disable Immediate Processing (DIIP) -----
  else if (strcmp (command, "DIIP") == 0)
  {
    immediateEnabled = false;

    // Acknowledge
    strcpy (SMACData.values, "IP Disabled");

    pStatus = SYSTEM_DATA;
  }

  //--- Do Immediate Process one time (DOIP) ----
  else if (strcmp (command, "DOIP") == 0)
    return DoImmediate ();

  //--- Enable Periodic Processing (ENPP) -------
  else if (strcmp (command, "ENPP") == 0)
  {
    periodicEnabled = true;
    nextPeriodicTime = millis();

    // Acknowledge
    strcpy (SMACData.values, "PP Enabled");

    pStatus = SYSTEM_DATA;
  }

  //--- Disable Periodic Processing (DIPP) ------
  else if (strcmp (command, "DIPP") == 0)
  {
    periodicEnabled = false;

    // Acknowledge
    strcpy (SMACData.values, "PP Disabled");

    pStatus = SYSTEM_DATA;
  }

  //--- Do Periodic Process one time (DOPP) -----
  else if (strcmp (command, "DOPP") == 0)
    return DoPeriodic ();

  //--- Get Rate (GRAT) -------------------------
  else if (strcmp (command, "GRAT") == 0)
  {
    // Return this Device's current periodic process rate (calls per hour)
    strcpy (SMACData.values, "RATE=");
    ltoa (GetRate(), SMACData.values + 5, 10);

    pStatus = SYSTEM_DATA;
  }

  //--- Set Rate (SRAT) -------------------------
  else if (strcmp (command, "SRAT") == 0)
  {
    // Set this Device's periodic process rate (calls per hour)
    double newRate = atof (params);
    SetRate (newRate);

    // Acknowledge new periodic rate
    strcpy (SMACData.values, "RATE=");
    ltoa (GetRate(), SMACData.values + 5, 10);

    nextPeriodicTime = millis();  // start new rate now
    pStatus = SYSTEM_DATA;
  }

  //--- Get Version (GDVR) ----------------------
  else if (strcmp (command, "GDVR") == 0)
  {
    strcpy (SMACData.values, "DVER=");
    strcat (SMACData.values, version);
    pStatus = SYSTEM_DATA;
  }

  // Return the resulting ProcessStatus
  return pStatus;
}
