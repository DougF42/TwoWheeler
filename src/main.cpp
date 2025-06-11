//
// MAIN copied from SMAC, just 'added' a driver. 

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
// NOTE: Python server:   'python -m http.server
//
//=============================================================================

//--- Includes --------------------------------------------

#include <Arduino.h>
#include <Preferences.h>
#include "common.h"
#include "Node.h"
#include "Driver.h"

//--- Globals ---------------------------------------------

bool         Debugging = true;  // ((( Set to false for production builds )))
char         Serial_Message[SERIAL_MAX_LENGTH];
char         Serial_NextChar;
int          Serial_Length = 0;
esp_err_t    ESPNOW_Result;
Preferences  MCUPreferences;  // Non-volatile memory
uint8_t      RelayerMAC[MAC_SIZE];  // MAC Address of the Relayer Module stored in non-volatile memory.
                                    // This is set using the <SetMAC.html> tool in the SMAC_Interface folder.
                                    // { 0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 }
bool         WaitingForRelayer = true;
RingBuffer   *CommandBuffer;
DPacket      DataPacket;
CPacket      CommandPacket;
char         DataString[MAX_MESSAGE_LENGTH];

Node         *ThisNode;  // The Node for this example
Driver       *myDriver;
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

  Serial.println("Starting the Node ...");

  //=======================================================
  // Create an instance of a Node here.
  // The 1st param is a name for your Node (to show in the SMAC Interface).
  // The 2nd param is the Node's unique ID (0-19).
  // Node ID's cannot be duplicated in your SMAC System.
  // --- Do not use the same ID for other Nodes ---
  //=======================================================
  ThisNode = new Node("TwoWheeler", 1);
  myDriver = new Driver(1);  // device ID 1
  //=======================================================
  // Add all Devices to the Node (one driver with two motors, 
  //      each has pid, ln298 and MotorControl))
  //=======================================================
  MotorControl_config_t left_mtr_cfg =
      {
          .chnlNo = LEDC_CHANNEL_1,
          .ena_pin = MOTOR_1_EN,
          .dir_pin_a = MOTOR_1_DRIVE_A,
          .dir_pin_b = MOTOR_1_DRIVE_B,
          .quad_pin_a = MOTOR_1_QUAD_A,
          .quad_pin_b = MOTOR_1_QUAD_B,
          .kp = 0,
          .ki = 0,
          .kd = 0,
      };
  Serial.println("Adding LEFT motor");
  myDriver->addNewMotor(left_mtr_cfg);

  MotorControl_config_t right_mtr_cfg =
      {
          .chnlNo = LEDC_CHANNEL_1,
          .ena_pin = MOTOR_2_EN,
          .dir_pin_a = MOTOR_2_DRIVE_A,
          .dir_pin_b = MOTOR_2_DRIVE_B,
          .quad_pin_a = MOTOR_2_QUAD_A,
          .quad_pin_b = MOTOR_2_QUAD_B,
          .kp = 0,
          .ki = 0,
          .kd = 0,
      };
  Serial.println("Adding RIGHT motor");
  myDriver->addNewMotor(right_mtr_cfg);
  ThisNode->AddDevice(myDriver);

  // PING the Relayer once per second until it responds with PONG
  Serial.println ("PINGing Relayer ...");
  strcpy (DataPacket.deviceID, "00");
  strcpy (DataPacket.value, "PING");
  unsigned long  nowSec, lastSec = 0L;
  WaitingForRelayer = true;
  while (WaitingForRelayer)
  {
    DataPacket.timestamp = millis ();
    nowSec = DataPacket.timestamp / 1000L;

    if (nowSec > lastSec)
    {
      ThisNode->SendDataPacket ();
      lastSec = nowSec;
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
    sprintf (DataString, "CurrentMAC=%02x:%02x:%02x:%02x:%02x:%02x", RelayerMAC[0], RelayerMAC[1], RelayerMAC[2], RelayerMAC[3], RelayerMAC[4], RelayerMAC[5]);
    Serial.println (DataString);
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
        strncpy (DataString, Serial_Message+i, 2);  DataString[2] = 0;
        sscanf  (DataString, "%02x", RelayerMAC+j);
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
