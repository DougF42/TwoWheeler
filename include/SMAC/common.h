//=========================================================
//
//     FILE : common.h
//
//  PROJECT : SMAC Framework
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

#ifndef COMMON_H
#define COMMON_H

//--- Includes --------------------------------------------

#include <Arduino.h>
#include <esp_now.h>
#include "RingBuffer.h"

//--- Defines ----------------------------------------------

#define SERIAL_BAUDRATE      115200
#define MAX_VERSION_LENGTH       22
#define MAX_NODES                20  // Maximum number of ESP-NOW peers
#define MAX_DEVICES             100  // Maximum number of Devices that can connect to a Node
#define ID_SIZE                   2  // Size of nodeID and deviceID
#define MAX_NAME_LENGTH          32
#define MAX_MESSAGE_LENGTH      250  // Max message size for ESP-NOW protocol
#define MAC_SIZE                  6  // Size of ESP32 MAC Address
#define COMMAND_SIZE              4
#define MAX_VALUE_LENGTH        240
#define MAX_PARAMS_LENGTH       240
#define MIN_COMMAND_LENGTH        7  // Minimum Input Command String: dd|cccc
#define MIN_DATA_LENGTH          10  // Minimum Output Data String: nn|dd|t|d
#define TEMP_BUFFER_LENGTH      250

// For the Espressif ESP32-S3-DevKitC-1 board, the built-in LED
// is a 1-element addressable string of RGB LEDs of type WS2812.
#define STATUS_LED_PIN           38  // GPIO-48 for v1.0 boards, GPIO-38 for v1.1 boards
#define STATUS_LED_BRIGHTNESS    20  // Not recommended above 64

//--- Types -----------------------------------------------

typedef struct DPacket
{
  char           deviceID[ID_SIZE + 1];
  unsigned long  timestamp;
  char           value[MAX_VALUE_LENGTH + 1];
} DPacket;

typedef struct CPacket
{
  int   deviceIndex;
  char  command[COMMAND_SIZE + 1];
  char  params[MAX_PARAMS_LENGTH + 1];
} CPacket;

enum ProcessStatus
{
  SUCCESS_DATA,    // Process performed successfully, send <DataPacket> to Relayer
  SUCCESS_NODATA,  // Process performed successfully, no data to send to Relayer
  FAIL_DATA,       // Process failed, error code or message stored in value field of <DataPacket>, send <DataPacket> to Relayer
  FAIL_NODATA,     // Process failed, no data to send to Relayer
  NOT_HANDLED      // Command was not handled by base class ExecuteCommand(), child Node or Device class should handle the command
};

//--- Externs ---------------------------------------------

extern bool        Debugging;
extern esp_err_t   ESPNOW_Result;
extern uint8_t     RelayerMAC[];
extern RingBuffer  *CommandBuffer;
extern DPacket     DataPacket;
extern CPacket     CommandPacket;
extern char        DataString[];

#endif
