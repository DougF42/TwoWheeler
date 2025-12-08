//=========================================================
//
//     FILE : Node.cpp
//
//  PROJECT : SMAC Framework
//
//    NOTES : Node base class:
//            Derive your custom Node from this class.
//
//   AUTHOR : Bill Daniels
//            Copyright 2021-2025, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

//--- Includes --------------------------------------------

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Node.h"
#include "Device.h"

//--- Declarations ----------------------------------------

void ESPNOW_Receiver (const esp_now_recv_info_t *info, const uint8_t *espnowString, int stringLength);
// void onESPNOWSent     (const uint8_t *mac_addr, esp_now_send_status_t status);  // Not used in SMAC

extern bool  WaitingForRelayer;

//--- Constructor -----------------------------------------

Node::Node (const char *inName, int inNodeID)
{
  // Check name and nodeID range
  if ((strlen (inName) < 1) || (inNodeID < 0) || (inNodeID >= MAX_NODES))
  {
    Serial.println ("ERROR: Invalid Node construction");
    return;
  }

  // Set the UI name, ID and version number
  strncpy (name, inName, MAX_NAME_LENGTH-1);
  name[MAX_NAME_LENGTH-1] = 0;

  // Names cannot have commas
  for (int i=0; i<strlen(name); i++)
    if (name[i] == ',') name[i] = '.';

  sprintf (nodeID, "%02d", inNodeID);

  strcpy (version, "3.0.0");  // no more than 9 chars


  //================================================
  //  Init ESP-NOW Communications with the Relayer
  //================================================

  Serial.println ("Starting ESP-NOW communication ...");


  // Set this device as both an Access Point and a Wi-Fi Station
  if (!WiFi.mode (WIFI_AP_STA))
  {
    Serial.println ("ERROR: Unable to set WiFi mode");
    return;
  }
  delay (100);

  Serial.print ("WiFi Channel is "); Serial.println (WiFi.channel());

  // Load this Node's MAC address (ESP-NOW v2)
  WiFi.macAddress().toCharArray (macAddressString, sizeof(macAddressString));



  // // Set ESP-NOW rate to 5-GHz for faster comms ???
  // esp_now_rate_config_t  rateConfig = { ... };
  // esp_now_set_peer_rate_config (macAddress, &rateConfig);



  // Init ESP-NOW protocol
  ESPNOW_Result = esp_now_init ();
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: Unable to initialize ESP-NOW protocol: ");
    Serial.println (ESPNOW_Result);
    return;
  }

  // Load Relayer's MAC Address and register as peer
  // <RelayerMAC> was loaded from non-volatile memory and set in <main.cpp>
  esp_now_peer_info_t  peerInfo = {};
  memcpy (peerInfo.peer_addr, RelayerMAC, MAC_SIZE);
  peerInfo.channel = 0;  // 0 = Auto-select channel
  peerInfo.encrypt = false;

  Serial.println ("Adding Node as ESP-NOW Peer ...");
  ESPNOW_Result = esp_now_add_peer (&peerInfo);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: Unable to add this Node as ESP-NOW peer: ");
    Serial.println (ESPNOW_Result);
    return;
  }

  // Register receive event
  ESPNOW_Result = esp_now_register_recv_cb (ESPNOW_Receiver);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: Unable to register ESP-NOW Command handler: ");
    Serial.println (ESPNOW_Result);
  }
}

//--- AddDevice -------------------------------------------

void Node::AddDevice (Device *device)
{
  if (numDevices < MAX_DEVICES)
  {
    // Assign the next DeviceID to the Device (00-99)
    device->SetID (numDevices);

    // Add Device to the devices array
    devices[numDevices++] = device;

    // Set the Node object for the device
    device->SetNode (this);

    Serial.print   ("Added ");
    Serial.print   (device->GetName());
    Serial.println (" device");
  }
}

//--- SendData --------------------------------------------

IRAM_ATTR void Node::SendData (const char *sourceDeviceID, bool broadcast)
{
  // The global SMACData.values field should be filled.

  // Build an ESPNOW Data string.  It has four fields separated with the '|' char:
  //
  //   ┌──────────── 1-char packet type ('D' for Data)
  //   │ ┌────────── 2-char source nodeID (00-19)
  //   │ │  ┌─────── 2-char source deviceID (00-99)
  //   │ │  │    ┌── variable length string of values (multiple values are comma delimited)
  //   │ │  │    │
  //   D|nn|dd|values
  //
  // ESPNOW strings must be NULL terminated.
  // A '|' and timestamp is appended to all Data Strings by the Relayer

  memcpy (ESPNOW_String, "D|--|--|", 8);  // Start with 'D' for Data
  memcpy (ESPNOW_String + 2, nodeID, ID_SIZE);
  memcpy (ESPNOW_String + 5, sourceDeviceID, ID_SIZE);
  ESPNOW_String[8] = 0;
  strcat (ESPNOW_String, SMACData.values);

  //=============================
  // Send Data String to Relayer
  //=============================
  if (broadcast)                                                                                   //  ┌─ include string terminator
    ESPNOW_Result = esp_now_send (NULL      , (const uint8_t *) ESPNOW_String, strlen(ESPNOW_String) + 1);
  else
    ESPNOW_Result = esp_now_send (RelayerMAC, (const uint8_t *) ESPNOW_String, strlen(ESPNOW_String) + 1);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: esp_now_send() returned ");
    Serial.println (ESPNOW_Result);
  }

  // Save last send packet time (time of silence)
  lastPacketTime = millis ();

  if (Debugging)
  {
    // Show the outgoing Data String
    Serial.print   ("Node --> Relayer : ");
    Serial.println (ESPNOW_String);
  }
}

//--- SendCommand -----------------------------------------

IRAM_ATTR void Node::SendCommand (const char *targetNodeID, const char *targetDeviceID, const char *command, const char *params, bool broadcast)
{
  // Build an ESPNOW Command string.  It has four or five fields separated with the '|' char:
  //
  //   ┌───────────────── 1-char packet type ('C' for Command)
  //   │ ┌─────────────── 2-char target nodeID (00-19)
  //   │ │  ┌──────────── 2-char target deviceID (00-99)
  //   │ │  │   ┌──────── 4-char command (usually capital letters)
  //   │ │  │   │     ┌── optional variable length string of parameters (multiple params are comma delimited)
  //   │ │  │   │     │
  //   C|nn|dd|cccc|params
  //
  // ESPNOW strings must be NULL terminated.

  memcpy (ESPNOW_String, "C|--|--|CCCC|", 13);  // Start with 'C' for Command
  if (!broadcast)
  {
    memcpy (ESPNOW_String + 2, targetNodeID, ID_SIZE);
    memcpy (ESPNOW_String + 5, targetDeviceID, ID_SIZE);
  }
  memcpy (ESPNOW_String + CommandOffset, command, COMMAND_SIZE);
  if (params == NULL)
    ESPNOW_String[12] = 0;
  else
  {
    ESPNOW_String[13] = 0;
    strcat (ESPNOW_String, params);
  }

  //================================
  // Send Command String to Relayer
  //================================
  if (broadcast)                                                                                   //  ┌─ include string terminator
    ESPNOW_Result = esp_now_send (NULL      , (const uint8_t *) ESPNOW_String, strlen(ESPNOW_String) + 1);
  else
    ESPNOW_Result = esp_now_send (RelayerMAC, (const uint8_t *) ESPNOW_String, strlen(ESPNOW_String) + 1);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: esp_now_send() returned ");
    Serial.println (ESPNOW_Result);
  }

  // Save last send packet time (time of silence)
  lastPacketTime = millis ();

  if (Debugging)
  {
    // Show the outgoing Command String
    Serial.print   ("Node --> Relayer : ");
    Serial.println (ESPNOW_String);
  }
}

//=========================================================
//  Run:
//
//  This method is called continuously in the loop function
//  of the main.cpp file.
//
//  It calls the Run() method for all Devices
//  and processes any Relayer/Interface commands.
//=========================================================

void Node::Run ()
{
  //===================================
  //  Run all Devices
  //===================================
  for (deviceIndex=0; deviceIndex<numDevices; deviceIndex++)
  {
    //--- Immediate Processing ---
    if (devices[deviceIndex]->IsIPEnabled ())
    {
      // Perform Immediate Processing
      pStatus = devices[deviceIndex]->DoImmediate ();

      // Any data to send?
      if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
        SendData (devices[deviceIndex]->GetID());
    }

    //--- Periodic Processing ---
    if (devices[deviceIndex]->IsPPEnabled ())
    {
      // Perform Periodic Processing
      pStatus = devices[deviceIndex]->RunPeriodic ();

      // Any data to send?
      if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
        SendData (devices[deviceIndex]->GetID());
    }
  }

  //===================================
  //  Check for any Commands
  //===================================
  if (CommandBuffer->GetNumElements() > 0)
  {
    //--- Process next command ---
    commandString = CommandBuffer->PopString ();  // Remember to free() this "popped" string

    // ESPNOW Command string format:
    //
    //   C|nn|dd|cccc[|params]
    //
    // The last '|' and params are optional and may not exist

    if (commandString != NULL)
    {
      if (Debugging)
      {
        Serial.print   ("commandString=");
        Serial.println (commandString);
      }

      // Check for valid length
      int cLength = strlen (commandString);
      if (cLength < MIN_COMMAND_LENGTH)
        Serial.println ("ERROR: Invalid command");
      else
      {
        //--- Execute Node Command ---
        if (cLength == MIN_COMMAND_LENGTH)
          pStatus = ExecuteCommand (commandString + CommandOffset);  // Command only
        else
        {
          commandString[MIN_COMMAND_LENGTH] = 0;  // Terminate Command string
          pStatus = ExecuteCommand (commandString + CommandOffset, commandString + ParamsOffset);  // Command with parameters
        }

        // Start with any Data coming from the Node, not a Device
        deviceIndex = -1;

        // Check if command is still not handled
        if (pStatus == NOT_HANDLED)
        {
          //=================================================
          // Not a Node command, so pass to Device to handle
          //=================================================

          // Get the numeric value of DeviceID
          deviceIndex = 10*((int)(commandString[5])-48) + ((int)(commandString[6])-48);

          // Check deviceIndex range
          if (deviceIndex >= numDevices)
          {
            if (Debugging)
            {
              Serial.print ("Command targeted for unknown device: ");
              Serial.print ("deviceIndex="); Serial.print (deviceIndex);
              Serial.print (", numDevices="); Serial.println (numDevices);
            }

            Serial.print ("ERROR: Command targeted for unknown device: "); Serial.println (deviceIndex);
            pStatus = FAIL_NODATA;

            // sprintf (SMACData.values, "ERROR: Command targeted for unknown device '%02d'", deviceIndex);
            // pStatus = FAIL_DATA;
          }
          else
          {
            //--- Execute Device Command ---
            if (cLength == MIN_COMMAND_LENGTH)
              pStatus = devices[deviceIndex]->ExecuteCommand (commandString + CommandOffset);  // Command only
            else
            {
              commandString[MIN_COMMAND_LENGTH] = 0;  // Terminate Command string
              pStatus = devices[deviceIndex]->ExecuteCommand (commandString + CommandOffset, commandString + ParamsOffset);  // Command with parameters
            }
          }

          // Check if still not handled
          if (pStatus == NOT_HANDLED)
          {
            sprintf (SMACData.values, "ERROR: Unknown command '%s'", commandString + CommandOffset);
            pStatus = FAIL_DATA;
          }
        }

        // Any data to send?
        if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
          SendData ((deviceIndex < 0 || deviceIndex >= numDevices) ? "--" : devices[deviceIndex]->GetID());
      }

      //====================================
      // Free the Popped string from buffer
      //====================================
      free (commandString);
    }
  }

  // Check if this Node has been silent for some time.
  // If so, send a PONG to let the Interface know it is still alive.
  if (millis() - lastPacketTime > MAX_SILENT_DURATION)
  {
    strcpy (SMACData.values, "PONG");
    SendData ("--");
  }
}

//--- GetVersion ------------------------------------------

char * Node::GetVersion ()
{
  return version;
}

//=========================================================
//  ExecuteCommand:
//
//  This method handles the built-in Node commands and
//  sets a ProcessStatus.  Override this method in your
//  child Node class to handle custom Node commands.
//  Your method should first call this base class method,
//  then handle custom commands.
//  If the command is not handled by the Node, it will
//  get passed to the Device's ExecuteCommand() method.
//=========================================================

ProcessStatus Node::ExecuteCommand (char *command, char *params)
{
  // Init ProcessStatus
  pStatus = NOT_HANDLED;

  //--- Set Node Name (SNNA) ------------------------------
  if (strncmp (command, "SNNA", COMMAND_SIZE) == 0)
  {
    // Set this Node's name
    strncpy (name, params, MAX_NAME_LENGTH-1);
    name[MAX_NAME_LENGTH-1] = 0;

    // Acknowledge new name
    strcpy (SMACData.values, "NONAME=");
    strcat (SMACData.values, name);

    pStatus = SUCCESS_DATA;
  }

  //--- Get Node Info (GNOI) ----------------------------
  else if (strncmp (command, "GNOI", COMMAND_SIZE) == 0)
  {
    // Send Node info with fields delimited with commas
    sprintf (SMACData.values, "NOINFO=%s,%s,%s,%d", name, version, macAddressString, numDevices);

    pStatus = SUCCESS_DATA;
  }

  //--- Get Device Info (GDEI) ----------------------------
  else if (strncmp (command, "GDEI", COMMAND_SIZE) == 0)
  {
    // For each Device, send a Data Packet with values = name,version,ipEnabled(Y/N),ppEnabled(Y/N),periodic data rate
    for (int i=0; i<numDevices; i++)
    {
      sprintf (SMACData.values, "DEINFO=%s,%s,%c,%c,%lu", devices[i]->GetName(), devices[i]->GetVersion(), devices[i]->IsIPEnabled() ? 'Y':'N', devices[i]->IsPPEnabled() ? 'Y':'N', devices[i]->GetRate());
      SendData (devices[i]->GetID());
    }

    // All Device data has been sent, no need to send anything else
    pStatus = SUCCESS_NODATA;
  }

  //--- Ping (PING) ---------------------------------------
  else if (strncmp (command, "PING", COMMAND_SIZE) == 0)
  {
    // Got PINGed from Interface, Respond with PONG
    strcpy (SMACData.values, "PONG");

    pStatus = SUCCESS_DATA;
  }

  //--- Change WiFi Channel (WFCH) ------------------------
  else if (strncmp (command, "WFCH", COMMAND_SIZE) == 0)
  {
    if (strlen (params) > 0)
    {
      // Get new channel
      uint8_t newChannel = (uint8_t) atoi (params);

      if (newChannel >= 0 && newChannel <= 14)
      {
        // Acknowledge change
        sprintf (SMACData.values, "New WiFi Channel rquested; Changing to channel %d", newChannel);

        // Change ESP-NOW WiFi Channel to new channel
        esp_wifi_set_channel (newChannel, WIFI_SECOND_CHAN_NONE);

        pStatus = SUCCESS_DATA;
      }
    }

    // Check for invalid channel
    if (pStatus == NOT_HANDLED)
    {
      Serial.println ("ERROR: Invalid WiFi Channel");
      pStatus = FAIL_NODATA;

      // strcpy (SMACData.values, "ERROR: Invalid WiFi Channel");
      // pStatus = FAIL_DATA;
    }
  }

  //--- Blink (BLIN) --------------------------------------
  else if (strncmp (command, "BLIN", COMMAND_SIZE) == 0)
  {
    // Blink the Status LED
    for (int i=0; i<10; i++)
    {
      STATUS_LED_BAD;
      delay (80);

      STATUS_LED_GOOD;
      delay (20);
    }

    pStatus = SUCCESS_NODATA;
  }

  //--- Get Version (GNVR) --------------------------------
  else if (strncmp (command, "GNVR", COMMAND_SIZE) == 0)
  {
    sprintf (SMACData.values, "NVER=%s", version);
    pStatus = SUCCESS_DATA;
  }

  //--- Reset (RSET) --------------------------------------
  else if (strncmp (command, "RSET", COMMAND_SIZE) == 0)
  {
    // Acknowledge Reset
    Serial.println ("Resetting Node ... ");

    // Reset this Node
    esp_restart();
    // x x
    //  o
  }

  // Return the resulting ProcessStatus
  return pStatus;
}



//=========================================================
// External "C" Functions
//=========================================================

//--- ESPNOW_Receiver -------------------------------------

void ESPNOW_Receiver (const esp_now_recv_info_t *info, const uint8_t *espnowString, int stringLength)
{
  if (Debugging)
  {
    // Show the incoming message string
    Serial.print   ("Node <-- Relayer : ");
    Serial.println ((char *) espnowString);
  }

  // Check if Relayer responded to initial PING
  if (strncmp ((char *) espnowString, "PONG", COMMAND_SIZE) == 0)
    WaitingForRelayer = false;

  else if ((char)(espnowString[0]) == 'C')  // Data messages are ignored
  {
    // Add this command to the Command buffer
    CommandBuffer->PushString ((char *) espnowString);
  }
}
