//
// MAIN copied from SMAC, just 'added' a driver. 

//=============================================================================
//
//       FILE : main.cpp
//
//
//     AUTHOR : Doug Fajardo
//
//--- Includes --------------------------------------------

#include <Arduino.h>
#include <Preferences.h>
#include "common.h"
#include "driver/gpio.h"
#include "DefNode.h"
#include "Driver.h"
#include "INA3221Device.h"

#define USE_INA3221

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

DefNode      *ThisNode;  // The Node for this example
Driver       *myDriver;

#ifdef USE_INA3221
#include "Wire.h"
INA3221Device      *myIna3221Device;
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
  Serial.print("Loading Relayer MAC Address ...");
  MCUPreferences.begin("RelayerMAC", false);

  if (not MCUPreferences.isKey("RelayerMAC"))
  {
    // NOTHING IN Prefrences - use default
    RelayerMAC[0] = 0xE4;
    RelayerMAC[1] = 0x65;
    RelayerMAC[2] = 0xb8;
    RelayerMAC[3] = 0x58;
    RelayerMAC[4] = 0x62;
    RelayerMAC[5] = 0x78;
    Serial.println(" From DEFAULT");

  }  else  {

    MCUPreferences.getBytes("RelayerMAC", RelayerMAC, sizeof(RelayerMAC));
    Serial.print("FROM FLASH");
  }
  MCUPreferences.end      ();
  Serial.printf(" Relayer addr: %02x:%02X:%02X:%02X:%02X:%02X\n\r", 
    RelayerMAC[0],RelayerMAC[1], RelayerMAC[2],
    RelayerMAC[3],RelayerMAC[4], RelayerMAC[5]);

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
  ThisNode = new DefNode("TwoWheeler", 1);


  //=======================================================
  //Set up the DRIVER device
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

  MotorControl_config_t right_mtr_cfg =
      {
          .chnlNo = LEDC_CHANNEL_2,
          .ena_pin = MOTOR_2_EN,
          .dir_pin_a = MOTOR_2_DRIVE_A,
          .dir_pin_b = MOTOR_2_DRIVE_B,
          .quad_pin_a = MOTOR_2_QUAD_A,
          .quad_pin_b = MOTOR_2_QUAD_B,
          .kp = 0,
          .ki = 0,
          .kd = 0,
      };

  // CREATE DRIVER device
  myDriver = new Driver(ThisNode, "Driver");
  myDriver->setup(&left_mtr_cfg, &right_mtr_cfg);
  ThisNode->AddDevice(myDriver);


  #ifdef USE_INA3221
  // CREATE Power Monitor device
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    myIna3221Device = new INA3221Device("Power", I2C_INA3221_ADDR, &Wire);
    ThisNode->AddDevice(myIna3221Device);
  #endif

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
