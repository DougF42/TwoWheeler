//=========================================================
//
//     FILE : Device.h
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
//            █ Devices are peripherals connected to a Node Module.
//              They can be sensors, lab or industrial equipment, digital/analog electronics, motor drivers, etc.
//
//            █ All Devices added to your Node will be assigned a unique 1-byte ID (0-99).
//              The Device ID is assigned automatically when your Device is added to a Node.
//
//            █ Devices can perform two kinds of operations: an "Immediate Process" and a "Periodic Process".
//
//              ∙ An Immediate Process is a quick operation you want your device to do continuously with
//                as little delay as possible between operations.
//
//              ∙ A Periodic Process is an operation to be performed at a periodic rate such as reading a
//                sensor once per second or minute.  Minimum rate is 1/hr.  Maximum rate is 20/sec.
//
//              ∙ You define your Immediate Process by overriding the virtual DoImmediate() method.
//
//              ∙ You define your Periodic Process by overriding the virtual DoPeriodic() method.
//
//              ∙ The rate of calls to DoPeriodic() is set in "calls per hour" (1 - 72,000)
//                It defaults to 3600 (one call per second).
//
//                      1 = one sample per hour   (the slowest data rate)
//                     60 = one sample per minute (for example, a temperature plot for a day)
//                   3600 = one sample per second (default)
//                  72000 = 20 samples per second (recommended fastest data rate)
//
//                Use the SRAT command in ExecuteCommand() to set the periodic rate.
//
//              ∙ If either process has data to return, it should populate the global <DataPacket> timestamp
//                and value fields, then return one of the <ProcessStatus> enums, usually SUCCESS_DATA.
//
//                The global <DataPacket> holds outgoing node/device data and has the following three fields:
//
//                  char           *deviceID   : pointer to the 2-char deviceID (00-99)
//                  unsigned long  timestamp   : timestamp when value was aquired (usually millis())
//                  char           value[..]   : variable length value string (including NULL terminating char)
//                                               this can be a numerical value or a text message
//
//                <DataPacket.value> must be NULL terminated!
//
//            █ All Devices can execute custom commands by overriding the virtual ExecuteCommand() method.
//
//              ∙ Both Nodes and Devices can receive commands from the User Interface (SMAC Interface)
//
//              ∙ ExecuteCommand() is the method called when a command is received targeted for this Device.
//                A command string holds the incoming command data and has the following four fields
//                separated with the '|' char:
//
//                  ┌─────────────── 2-char nodeID (00-19)
//                  │  ┌──────────── 2-char deviceID (00-99)
//                  │  │   ┌──────── 4-char command (usually capital letters)
//                  │  │   │     ┌── Optional variable length parameter string
//                  │  │   │     │
//                  nn|dd|CCCC|params
//
//              ∙ This Device base class handles the following built-in (reserved) Device commands:
//
//                GDNA = Get Device Name              : Returns this device's name
//                SDNA = Set Device Name              : Sets this device's name (a User-Friendly name for the SMAC Interface)
//                ENIP = Enable Immediate Processing  : Start executing the immediate process for this device
//                DIIP = Disable Immediate Processing : Stop executing the immediate process for this device
//                DOIP = Do Immediate Process         : Perform the immediate process one time
//                ENPP = Enable Periodic Processing   : Start executing the periodic process for this device (start sending data) default
//                DIPP = Disable Periodic Processing  : Stop executing the periodic process for this device (stop sending data)
//                DOPP = Do Periodic Process          : Perform the periodic process one time, returns true or false
//                GRAT = Get Rate                     : Get the current periodic process rate for this device in calls per hour:
//                SRAT = Set Rate                     : Set the periodic process rate for this device in procs per hour
//                GDVR = Get Device Version           : Get the current version of this Device's firmware
//
//              ∙ Your child Device class can override ExecuteCommand() to handle custom commands,
//                for example, CALI for a calibrate function.
//
//                Child Device classes should first call this base class's ExecuteCommand() to handle the built-in Device commands:
//                  Device::ExecuteCommand(...)
//                If the command is not handled by this base class, you can handle the command in your derived class.
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

#ifndef DEVICE_H
#define DEVICE_H

//--- Includes --------------------------------------------

#include "common.h"


//=========================================================
//  class Device
//=========================================================

class Device
{
  protected:
    char           deviceID[ID_SIZE+1];                 // Assigned by the parent Node when "added" using addDevice()
    char           name[MAX_NAME_LENGTH+1] = "Device";  // Display name for the SMAC Interface
    char           version[MAX_VERSION_LENGTH] = "";    // A version number for this Node's firmware (yyyy.mm.dd<a-z>)
    bool           immediateEnabled = true;             // true to have DoImmediate called continuously (as fast as possible)
    bool           periodicEnabled  = true;             // true to have DoPeriodic called at the process period
    unsigned long  processPeriod    = 1000L;            // milliseconds; default is 1 process per second
    unsigned long  nextPeriodicTime = 0L;               // Next time to do the periodic process
    unsigned long  timestamp;                           // Timestamp of last data sample
    unsigned long  now;
    ProcessStatus  pStatus;

  public:
    Device (const char *inName);

    void           SetID       (int inDeviceID);  // No need to use this method. It is called by the Node.
    const char *   GetID       ();                // Return the deviceID
    const char *   GetName     ();                // Return the name of this Device
    bool           IsIPEnabled ();                // Is Immediate Processing enabled?
    bool           IsPPEnabled ();                // Is Periodic Processing enabled?
    unsigned long  GetRate     ();                // Return the periodic data rate of this Device
    void           SetRate     (double newRate);  // Set the periodic process rate (# per hour)
    const char *   GetVersion  ();                // Return the current version of this Device

    ProcessStatus  RunPeriodic ();  // No need to use this method. It is called by the Node.

    virtual ProcessStatus  DoImmediate    ();  // Override this method for processing your device continuously
    virtual ProcessStatus  DoPeriodic     ();  // Override this method for processing your device periodically
    virtual ProcessStatus  ExecuteCommand ();  // Override this method to handle custom commands
};

#endif
