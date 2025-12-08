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

// Built-in LED on board
//-----------------------
// #define USE_MONO_LED   // Use this for boards with mono color leds
#define USE_RGB_LED       // Use this for boards with RGB leds

//--- Status LED ---
#if defined USE_MONO_LED && defined USE_RGB_LED
#error "ERROR: Only one of USE_MONO_LED or USE_RGB_LED may be defined"
#endif

#if (defined(USE_MONO_LED))
#define STATUS_LED_PIN         LED_BUILTIN
#define STATUS_LED_BAD         (digitalWrite (STATUS_LED_PIN, LOW))
#define STATUS_LED_GOOD        (digitalWrite (STATUS_LED_PIN, HIGH))

#elif (defined(USE_RGB_LED))
#define STATUS_LED_PIN         38  // GPIO-48 for v1.0 boards, GPIO-38 for v1.1 boards
#define STATUS_LED_BRIGHTNESS  20  // Not recommended above 64
#define STATUS_LED_BAD         (rgbLedWrite (STATUS_LED_PIN, STATUS_LED_BRIGHTNESS, 0, 0))
#define STATUS_LED_GOOD        (rgbLedWrite (STATUS_LED_PIN, 0, STATUS_LED_BRIGHTNESS, 0))
#endif

//--- Common Stuff ---
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
// #define MAX_PARAMS_LENGTH       236
#define MAX_SILENT_DURATION   30000  // Maximum millis of silence while testing for dead Node

//--- Types -----------------------------------------------

typedef struct SMACDataPacket
{
  char  nodeID   [ID_SIZE+1];
  char  deviceID [ID_SIZE+1];
  char  values   [MAX_VALUES_LENGTH+1];  // Can hold multiple data values separated by commas
} SMACDataPacket;

enum ProcessStatus
{
  SUCCESS_DATA,    // Process performed successfully, send SMACData to Relayer
  SUCCESS_NODATA,  // Process performed successfully, no data to send to Relayer
  FAIL_DATA,       // Process failed, error code or message stored in SMACData.values field
  FAIL_NODATA,     // Process failed, no data to send to Relayer
  NOT_HANDLED      // Command was not handled by base class ExecuteCommand(), child Node or Device class should handle the command
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
