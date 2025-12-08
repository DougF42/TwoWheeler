//=============================================================================
//
//       FILE : main.cpp
//
//    PROJECT : SMAC Framework - Example 1
//
//      NOTES : This is the PIO firmware for the SMAC Node of Example 1.
//
//              About this template:
//              - The SMAC System uses Espressif's ESP-NOW protocol between Node Modules and the Relayer Module.
//              - Device is the base class from which your custom Devices are derived.
//              - This "template" creates a Node with a single Device, a LightSensor.
//              - Node Modules first attempt to connect to the Relayer Module.
//              - Once connected, the LightSensor Device "measures" a value and outputs a "Data String" with its value.
//              - The above operation is performed periodically to maintain continuous data.
//              - All Device data can be visualized with gauges and graphs using the SMAC Interface (a Chrome browser app).
//              - The SMAC System is bidirectional. You can send commands to both Nodes and individual Devices.
//              - Commands can be sent directly from the SMAC Interface using buttons, dials, sliders, etc.
//              - The Node and Device base classes handle standard commands and child classes can handle custom commands.
//
//              Classes in this example:
//
//              ∙ Node   -- at least one Node is required for any SMAC system
//              ∙ Device
//                  │
//                  └── LightSensor -- Demo to show how sensor data can be sent to the SMAC Interface
//
//  DEBUGGING : Set the global <Debugging> to true to see debugging info in Serial Monitor.
//              Be sure to set <Debugging> to false for production builds!
//
//     AUTHOR : Bill Daniels
//              Copyright 2021-2025, D+S Tech Labs, Inc.
//              All Rights Reserved
//
// CHANGES:
// 12/6/2025 DEF remove #include lightsensor.h and Lightsensor device (not used this project)
             
//=============================================================================

//--- Includes --------------------------------------------

#include <Arduino.h>
#include <Preferences.h>
#include "common.h"
#include "SMAC/Node.h"

//--- Globals ---------------------------------------------

bool            Debugging = false;  // ((( Set to false for production builds )))
char            Serial_Message[SERIAL_MAX_LENGTH];
char            Serial_NextChar;
int             Serial_Length = 0;
esp_err_t       ESPNOW_Result;
Preferences     MCUPreferences;  // Non-volatile memory
uint8_t         RelayerMAC[MAC_SIZE];  // MAC Address of the Relayer Module stored in non-volatile memory.
                                       // This is set using the <SetMAC.html> tool in the SMAC_Interface folder.
                                       // e.g. { 0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 }
bool            WaitingForRelayer = true;
RingBuffer      *CommandBuffer;
const int       CommandOffset = MIN_COMMAND_LENGTH - COMMAND_SIZE;
const int       ParamsOffset  = MIN_COMMAND_LENGTH + 1;
SMACDataPacket  SMACData;
char            ESPNOW_String[MAX_ESPNOW_LENGTH];
Node            *ThisNode;   // The global Node object

#include "user_global.h"
#ifndef user_global_once
// SMAC Systems can have up to 20 Nodes.
// Set the Name and NodeID for this ESP32 module (0-19).
// The NodeID's for a SMAC Systems with multiple Nodes
// must be unique and cannot be duplicated.
char  ThisNodeName[] = "My First Node";  // Name for this node (max 32 chars)
int   ThisNodeID     = 0;                // NodeID (0-19)
#endif
//--- Declarations ----------------------------------------

void Serial_CheckInput     ();
void Serial_ProcessMessage ();


//=========================================================
//  setup
//=========================================================

void setup()
{
  // Init built-in LED, start off bad
  pinMode (STATUS_LED_PIN, OUTPUT);
  STATUS_LED_BAD;

  // Init serial comms
  Serial_Message[0] = 0;
  Serial.begin (SERIAL_BAUDRATE);

  Serial.println ("--- Program Start ----------------------");

  // Load the Relayer Module's MAC Address from non-volatile memory
  Serial.println ("Loading Relayer MAC Address ...");
  MCUPreferences.begin    ("RelayerMAC", false);
  MCUPreferences.getBytes ("RelayerMAC", RelayerMAC, sizeof(RelayerMAC));
  MCUPreferences.end      ();

  // Init Command buffer (a circular FIFO buffer)
  CommandBuffer = new RingBuffer (FIFO);

  Serial.println ("Starting the Node ...");

  //--- Create Node Instance ---
  ThisNode = new Node (ThisNodeName, ThisNodeID);


  //=======================================================
  // Create or Start/Begin any infrastructure your Devices
  // may need. Then, if necessary, pass those references
  // to your Devices' constructors.
  //=======================================================
#include "user_setup.h"

  //=======================================================
  // Add all Devices to this Node
  //=======================================================

  // PING the Relayer once per second until it responds with PONG
  Serial.println ("PINGing Relayer ...");
  strcpy (SMACData.values, "PING");
  unsigned long  nowMillis, lastMillis = 0L;
  WaitingForRelayer = true;
  while (WaitingForRelayer)
  {
    nowMillis = millis ();
    if (nowMillis - lastMillis > 1000L)
    {
      lastMillis = nowMillis;
      ThisNode->SendData ("--");
    }

    // Check for Set MAC Tool
    Serial_CheckInput ();
  }

  // Relayer responded, All good, Go green
  Serial.println ("Relayer responded to PING");
  STATUS_LED_GOOD;

  Serial.println ("Node running ...");
}


//=========================================================
//  loop
//  NO NEED TO CHANGE THIS CODE
//=========================================================

void loop()
{
  // Keep the Node running
  ThisNode->Run ();

  // Check for serial chars
  Serial_CheckInput ();
  #include "user_loop.h"
}


//=========================================================
//  Serial_CheckInput
//  NO NEED TO CHANGE THIS CODE
//=========================================================

void Serial_CheckInput ()
{
  // Check serial port for characters
  while (Serial.available ())
  {
    Serial_NextChar = (char) Serial.read ();
    if (Serial_NextChar != '\r')  // ignore CR's
    {
      if (Serial_NextChar == '\n')
      {
        // Message is ready, terminate string and process it
        Serial_Message[Serial_Length] = 0;
        Serial_ProcessMessage ();

        // Start new message
        Serial_Length = 0;
      }
      else
      {
        // Add char to end of buffer
        if (Serial_Length < SERIAL_MAX_LENGTH - 1)
          Serial_Message[Serial_Length++] = Serial_NextChar;
        else  // too long
        {
          Serial.println ("ERROR: Serial message is too long.");

          // Ignore and start new message
          Serial_Length = 0;
        }
      }
    }
  }
}


//=========================================================
//  Serial_ProcessMessage
//  NO NEED TO CHANGE THIS CODE
//=========================================================

void Serial_ProcessMessage ()
{
  // Check if using <Set MAC> tool
  if (strcmp (Serial_Message, "SetRelayerMAC") == 0)
  {
    // Send current setting
    sprintf (ESPNOW_String, "CurrentMAC=%02x:%02x:%02x:%02x:%02x:%02x", RelayerMAC[0], RelayerMAC[1], RelayerMAC[2], RelayerMAC[3], RelayerMAC[4], RelayerMAC[5]);
    Serial.println (ESPNOW_String);
  }
  else if (strncmp (Serial_Message, "NewMAC=", 7) == 0)
  {
    if (strlen (Serial_Message) < 24)  // NewMAC=xx:xx:xx:xx:xx:xx
    {
      Serial.print   ("Invalid MAC Address: ");
      Serial.println (Serial_Message);
    }
    else
    {
      // Parse and set new MAC Address (xx:xx:xx:xx:xx:xx)
      for (int i=7, j=0; j<sizeof(RelayerMAC); i+=3, j++)
      {
        strncpy (ESPNOW_String, Serial_Message+i, 2);  ESPNOW_String[2] = 0;
        sscanf  (ESPNOW_String, "%02x", RelayerMAC+j);
      }

      // Store new network credentials in non-volatile <preferences.h>
      MCUPreferences.begin    ("RelayerMAC", false);
      MCUPreferences.putBytes ("RelayerMAC", RelayerMAC, sizeof(RelayerMAC));
      MCUPreferences.end      ();

      // Respond
      Serial.println ("SetRelayerMAC-Success");

      // Blink the Status LED
      for (int i=0; i<10; i++)
      {
        STATUS_LED_BAD;
        delay (80);

        STATUS_LED_GOOD;
        delay (20);
      }

      // Reset the board
      esp_restart();
    }
  }
}
