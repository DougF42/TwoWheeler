//=========================================================
//
//     FILE : common.h
//
//  PROJECT : SMAC Framework
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2026, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

#ifndef COMMON_H
#define COMMON_H

//--- Includes --------------------------------------------

#include <Arduino.h>
#include <esp_now.h>
#include "RingBuffer.h"

//--- Defines ---------------------------------------------

#define SERIAL_BAUDRATE      115200
#define SERIAL_MAX_LENGTH        80
#define MAX_NODES                20  // Maximum number of ESP-NOW peers
#define MAX_DEVICES             100  // Maximum number of Devices that can connect to a Node
#define MAC_SIZE                  6  // Size of ESP32 MAC Address
#define ID_SIZE                   2  // Number of chars in nodeID and deviceID
#define COMMAND_SIZE              4
#define MAC_STRING_SIZE          17  // Size of MAC Address string including colons (XX:XX:XX:XX:XX:XX)
#define MIN_COMMAND_LENGTH       12  // Minimum Input Command String: C|nn|dd|cccc
#define MAX_VERSION_LENGTH       10  // Suggested format: MM.mm.pp (Major.minor.patch)
#define MAX_NAME_LENGTH          32
#define MAX_ESPNOW_LENGTH       250  // Max message size for ESP-NOW protocol
#define MAX_VALUES_LENGTH       230  // Need to leave room for appended timestamp
#define MAX_SILENT_DURATION   30000  // Maximum millis of silence while testing for dead Node

//--- Types -----------------------------------------------

struct SMACDataPacket
{
  char  nodeID   [ID_SIZE+1];
  char  deviceID [ID_SIZE+1];
  char  values   [MAX_VALUES_LENGTH+1];  // Can hold multiple data values separated by commas
};

// ProcessStatus is used by DoImmediate(), DoPeriodic() and ExecuteCommand() methods of both the Node and Devices
enum ProcessStatus
{
  WIDGET_DATA,  // Process has Widget Data to send to Interface
  SYSTEM_DATA,  // Process has System Data to send to Interface
  NODATA,       // Process has no data to send
  NOT_HANDLED   // Command was not handled
};

//--- Externs ---------------------------------------------

extern bool            Debugging;
extern uint8_t         RelayerMAC[];
extern RingBuffer      *CommandBuffer;
extern const int       CommandOffset;
extern const int       ParamsOffset;
extern SMACDataPacket  SMACData;
extern char            ESPNOW_String[];
extern esp_err_t       ESPNOW_Result;

#endif
