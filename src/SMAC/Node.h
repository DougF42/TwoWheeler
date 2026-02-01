//=========================================================
//
//     FILE : Node.h
//
//  PROJECT : SMAC Framework
//
//    NOTES : Node class:
//
//            █ A Node is an ESP32 Development board (with Devices attached) and uses
//              Espressif's ESP-NOW protocol to connect to a single Relayer module.
//              Each Node in your SMAC system will have a unique ID (0-19) which is
//              given when constructed.
//
//            █ The ESP32 Dev board has an RGB LED that is used as a "Status" LED:
//              ∙ Off   = No power/program not running -OR- Error booting (check Serial Monitor for errors)
//              ∙ Red   = Node running, waiting to connect to Relayer
//              ∙ Green = Connected to Relayer, sending data and listening for commands
//
//            █ Usually, Devices such as sensors, robots, factory/lab equipment, etc., physically
//              attach to Nodes via shielded cable with RJ-45 connectors or other means.
//
//            █ Although commands are usually targeted for Devices, Nodes can execute commands
//              by overriding the virtual ExecuteCommand() method.
//
//              ∙ ExecuteCommand() receives commands from the Relayer Module or the SMAC User Interface.
//
//              ∙ ExecuteCommand() is only called when it receives a command targeted for this Node
//                or a Device connected to this Node.
//
//              ∙ This Node base class handles the following built-in (reserved) Node commands:
//
//                SNNA = Set Node Name
//                GNOI = Get Node Info   : SMACData.values = NOINFO=name|version|macAddress|numDevices
//                GDEI = Get Device Info : SMACData.values = DEINFO=name|version|ipEnabled|ppEnabled|rate
//                PING = Check if still alive and connected; responds with "PONG"
//                WFCH = Set New ESP-NOW WiFi Channel
//                BLIN = Quickly blink the Node's status LED to indicate communication or location
//                GNVR = Get Node Firmware Version
//                RSET = Reset this Node's processor using esp_restart()
//
//            █ A child Node class can override ExecuteCommand() to handle custom commands.
//              It should first call this base class's ExecuteCommand() to handle the built-in Node commands:
//                Node::ExecuteCommand()
//
//            █ Data can be returned from ExecuteCommand() by filling the 'values' field of the
//              global <SMACData> structure and returning a ProcessStatus with data to send.
//
//  WARNING : Some Espressif ESP32 boards do not allow the use of Analog Channel 2 (ADC2) with Wifi.
//            Since Nodes use WiFi, do not use ADC2 pins as analog inputs.
//            https://github.com/espressif/arduino-esp32/issues/102
//
//            Also, do not use the strapping pins of the MCU.  You will have trouble with serial comms
//            and trouble uploading new firmware.
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2026, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//==========================================================

#ifndef NODE_H
#define NODE_H

//--- Includes ---------------------------------------------

#include "common.h"

//--- Declarations -----------------------------------------

class Device;  // Forward declaration to prevent circular dependency

//==========================================================
//  class Node
//==========================================================

class Node
{
  private:
    int  deviceIndex = 0;

  protected:
    char           nodeID[ID_SIZE+1];                                // This unique ID string ('00'-'19') is assigned at construction
    char           name[MAX_NAME_LENGTH+1] = "Node";                 // A display name to show in the SMAC Interface
    char           version[MAX_VERSION_LENGTH] = "";                 // A version number for this Node's firmware (yyyy.mm.dd<a-z>)
    char           macAddressString[MAC_STRING_SIZE+1] = "Not set";  // MAC address as a Hex string (xx:xx:xx:xx:xx:xx)
    Device         *devices[MAX_DEVICES];                            // Holds the array of Devices for this Node
    int            numDevices = 0;                                   // Number of added Devices
    char           *commandString;                                   // Command string from buffer
    unsigned long  lastPacketTime;                                   // Holds last Node communication time, used for keep alive
    ProcessStatus  pStatus;

  public:
    Node (const char *inName, int inNodeID);

    void   AddDevice   (Device *device);  // Call this method to add Devices
    void   Run         ();                // Run this Node; called from the loop() method of main.cpp
    void   SendData    (const char *sourceDeviceID, bool widgetData=true, bool broadcast=false);
    void   SendCommand (const char *targetNodeID, const char *targetDeviceID, const char *command, const char *params=NULL, bool broadcast=false);
    char * GetVersion  ();  // Return the current version of this Node

    virtual ProcessStatus  ExecuteCommand (char *command, char *params=NULL);  // Override this method in a child Node class
};

#endif
