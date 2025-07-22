//=========================================================
//
//     FILE : Node.h
//
//  PROJECT : SMAC Framework
//              │
//              └── Publish
//                    │
//                    └── Firmware
//                          │
//                          └── Node
//
//    NOTES : Node class:
//
//            █ A Node is an ESP32 Development board (with Devices attached) and use
//              Espressif's ESP-NOW protocol to connect to a single Relayer module.
//              Each Node in your SMAC system will have a unique ID (0-19) which is
//              given when constructed.
//
//            █ The ESP32 Dev board has an RGB LED that is used as a "Status" LED:
//              ∙ Off   = No power/program not running -OR- Error booting (check Serial Monitor for errors)
//              ∙ Red   = Node running, waiting to connect to Relayer
//              ∙ Green = Connected to Relayer, sending data and listening for commands
//
//            █ Usually, Devices such as sensors, factory/lab equipment, etc., physically attach
//              to Nodes via shielded cable with RJ-45 connectors or other means.
//
//            █ Nodes can execute commands by overriding the virtual ExecuteCommand() method.
//              Although this is rare.  Usually commands are targeted for Devices.
//
//              ∙ ExecuteCommand() receives commands from the Relayer Module or the User Interface.
//
//              ∙ ExecuteCommand() is only called when it receives a command targeted for this Node
//                or a Device connected to this Node.
//
//                The global <CommandPacket> holds incoming command data and has the following three fields:
//
//                  int  deviceIndex  : deviceID as an integer (0-99)
//                  char command[..]  : 4-char command (usually capital letters)
//                  char params[..]   : variable length parameter string
//
//              ∙ This Node base class handles the following built-in (reserved) Node commands:
//
//                SNNA = Set Node Name
//                GNOI = Get Node Info   : <DataPacket> value = name|version|macAddress|numDevices
//                GDEI = Get Device Info : <DataPacket> value = name|version|ipEnabled|ppEnabled|rate
//                PING = Check if still alive and connected; responds with "PONG"
//                BLIN = Quickly blink the Node's status LED to indicate communication or location
//                GNVR = Get Node Firmware Version
//                RSET = Reset this Node's processor using esp_restart()
//
//            █ A child Node class can override ExecuteCommand() to handle custom commands.
//              It should first call this base class's ExecuteCommand() to handle the built-in Node commands:
//                Node::ExecuteCommand()
//
//  WARNING : Some Espressif ESP32 boards do not allow the use of Analog Channel 2 (ADC2) with Wifi.
//            Since Nodes use WiFi, do not use ADC2 pins as analog inputs.
//            https://github.com/espressif/arduino-esp32/issues/102
//
//            Also, do not use the strapping pins of the MCU.  You will have trouble with serial comms
//            and trouble uploading new firmware.
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//==========================================================

#ifndef NODE_H
#define NODE_H

//--- Includes ---------------------------------------------

#include "Device.h"


//==========================================================
//  class Node
//==========================================================

class Node
{
  private:
    int  deviceIndex = 0;

  protected:
    char           nodeID[ID_SIZE+1];                 // This unique ID (00-19) is assigned at construction
    char           name[MAX_NAME_LENGTH+1] = "Node";  // A display name to show in the SMAC Interface
    char           version[MAX_VERSION_LENGTH] = "";  // A version number for this Node's firmware (yyyy.mm.dd<a-z>)
    char           macAddressString[18] = "Not set";  // MAC address as a Hex string (xx:xx:xx:xx:xx:xx)
    Device         *devices[MAX_DEVICES];             // Holds the array of Devices for this Node
    int            numDevices = 0;                    // Number of added Devices
    char           *commandString;                    // Command string from buffer
    ProcessStatus  pStatus;

  public:
    Node (const char *inName, int inNodeID);

    void          AddDevice (Device *device);  // Call this method to add Devices
    void          SendDataPacket ();           // Send the global <DataPacket> structure to the Relayer Module
    void          Run ();                      // Run this Node; called from the loop() method of main.cpp
    const char *  GetVersion ();               // Return the current version of this Node

    virtual ProcessStatus  ExecuteCommand ();  // Override this method in a child Node class
};

#endif
