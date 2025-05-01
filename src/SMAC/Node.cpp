//=========================================================
//
//     FILE : Node.cpp
//
//  PROJECT : SMAC Framework
//              │
//              └── Publish
//                    │
//                    └── Firmware
//                          │
//                          └── Node
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
#include "Node.h"
#include "esp_mac.h"

//--- Declarations ----------------------------------------

void onCommandReceived (const esp_now_recv_info_t * esp_now_info, const uint8_t *inCommandString, int commandlength);
// void onDataPacketSent  (const uint8_t *mac_addr, esp_now_send_status_t status);

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

  // Set the UI name, ID and version number (compile timestamp)
  strncpy (name, inName, MAX_NAME_LENGTH-1);
  name[MAX_NAME_LENGTH-1] = 0;

  sprintf (nodeID, "%02d", inNodeID);

  strcpy (version, __DATE__ " " __TIME__);


  //================================================
  //  Init ESP-NOW Communications with the Relayer
  //================================================

  Serial.println ("Starting ESP-NOW communication ...");

  // Set this device as a Wi-Fi Station
  if (!WiFi.mode (WIFI_STA))
  {
    Serial.println ("ERROR: Unable to set WiFi mode.");
    return;
  }
  delay(10); // Give WiFi a chance
  // Load this Node's MAC address
  uint8_t  macAddress[MAC_SIZE+2];
  for (int i=0; i<MAC_SIZE+2; i++) macAddress[i] = 0;  // Clear result first

  esp_efuse_mac_get_default (macAddress);  // System-programmed by Espressif, 6 bytes
  // // --OR--
  // macAddress = WiFi.macAddress ();


  // // Set WiFi rate ???  5-GHz ???
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
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  Serial.println ("Adding Node as ESP-NOW Peer ...");
  ESPNOW_Result = esp_now_add_peer (&peerInfo);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: Unable to add Relayer as ESP-NOW peer: ");
    Serial.println (ESPNOW_Result);
    return;
  }

  // Register receive event
  ESPNOW_Result = esp_now_register_recv_cb (onCommandReceived);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: Unable to register ESP-NOW Command handler: ");
    Serial.println (ESPNOW_Result);
    return;
  }


  // // Register send event
  // ESPNOW_Result = esp_now_register_send_cb (onDataPacketSent);
  // if (ESPNOW_Result != ESP_OK)
  // {
  //   Serial.print   ("ERROR: Unable to register send event handler: ");
  //   Serial.println (ESPNOW_Result);
  //   return;
  // }


  // Set this Node's MAC Address string (for the Interface to show)
  sprintf (macAddressString, "%02X:%02X:%02X:%02X:%02X:%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
  Serial.print   ("Node MAC = ");
  Serial.println (macAddressString);
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

    Serial.print   ("Added ");
    Serial.print   (device->GetName());
    Serial.println (" device");
  }
}

//--- SendDataPacket --------------------------------------

void Node::SendDataPacket ()
{
  // Convert the <DataPacket> to a Data string.
  // The <DataPacket> structure must be populated with deviceID, timestamp and value fields.
  //
  // A Data string has four fields separated with the '|' char:
  //
  //   ┌──────────────────── 2-char nodeID (00-19)
  //   │  ┌───────────────── 2-char deviceID (00-99)
  //   │  │     ┌─────────── variable length timestamp (usually millis())
  //   │  │     │        ┌── variable length value string (including NULL terminating char)
  //   │  │     │        │   this can be a numerical value or a text message
  //   │  │     │        │
  //   nn|dd|timestamp|value
  //
  // Data Strings must be NULL terminated.

  // Build the Data String
  memset (DataString + 2, '|', 4);
  memcpy (DataString, nodeID, ID_SIZE);
  memcpy (DataString + 3, DataPacket.deviceID, ID_SIZE);
  ltoa   (DataPacket.timestamp, DataString + 6, 10);
  strcat (DataString, "|");
  strcat (DataString, DataPacket.value);

  //=============================
  // Send Data String to Relayer
  //=============================                                                              ┌─ include string terminator
  ESPNOW_Result = esp_now_send (RelayerMAC, (const uint8_t *) DataString, strlen(DataString) + 1);
  if (ESPNOW_Result != ESP_OK)
  {
    Serial.print   ("ERROR: Unable to send Data String: ");
    Serial.println (ESPNOW_Result);
  }

  if (Debugging)
  {
    // Show the outgoing Data String
    Serial.print   ("Node --> Relayer : ");
    Serial.println (DataString);
  }
}

//=========================================================
//  Run:
//
//  This method is called continuously in the loop function
//  of the .ino file.
//
//  It calls the Run() method for all Devices
//  and processes any Relayer/Interface commands.
//=========================================================

void Node::Run ()
{
  //===================================
  //   Run all Devices
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
      {
        // Populate DeviceID and send it
        memcpy (DataPacket.deviceID, devices[deviceIndex]->GetID(), ID_SIZE);
        SendDataPacket ();
      }
    }

    //--- Periodic Processing ---
    if (devices[deviceIndex]->IsPPEnabled ())
    {
      // Perform Periodic Processing
      pStatus = devices[deviceIndex]->RunPeriodic ();

      // Any data to send?
      if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
      {
        // Populate DeviceID and send it
        memcpy (DataPacket.deviceID, devices[deviceIndex]->GetID(), ID_SIZE);
        SendDataPacket ();
      }
    }
  }

  //===================================
  //   Check for any commands
  //===================================
  if (CommandBuffer->GetNumElements() < 1)
    return;

  //--- Process next command ---
  commandString = CommandBuffer->PopString ();

  if (commandString != NULL)
  {
    int cLength = strlen (commandString);

    // Check length
    if (cLength < MIN_COMMAND_LENGTH)
      Serial.println ("ERROR: Invalid Command.");
    else
    {
      // Populate the global <CommandPacket>
      CommandPacket.deviceIndex = 10*(commandString[0]-48) + (commandString[1]-48);

      memcpy (CommandPacket.command, commandString + 3, COMMAND_SIZE);
      CommandPacket.command[COMMAND_SIZE] = 0;

      if (cLength > MIN_COMMAND_LENGTH + 1)
        strcpy (CommandPacket.params, commandString + 8);
      else
        CommandPacket.params[0] = 0;

      // Execute the command
      pStatus = ExecuteCommand ();

      // Check if command is still not handled
      if (pStatus == NOT_HANDLED)
      {
        //=================================================
        // Not a Node command, so pass to Device to handle
        //=================================================
        pStatus = devices[CommandPacket.deviceIndex]->ExecuteCommand ();
      }

      // Any data to send?
      if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
      {
        // Populate deviceID
        memcpy (DataPacket.deviceID, commandString, ID_SIZE);

        SendDataPacket ();
      }
    }

    //====================================
    // Free the Popped string from buffer
    //====================================
    free (commandString);
  }
}

//=========================================================
//  ExecuteCommand:
//
//  This method handles the built-in Node commands and
//  sets a ProcessStatus.  Override this method in your
//  child Node class to handle custom Node commands.
//  Your method should first call this base class method,
//  then handle custom commands.
//  If the command is not handled by the Node, then pass
//  it to the Device's ExecuteCommand() method.
//=========================================================

ProcessStatus Node::ExecuteCommand ()
{
  // Init ProcessStatus
  pStatus = NOT_HANDLED;

  //--- Set Node Name (SNNA) ------------------------------
  if (strncmp (CommandPacket.command, "SNNA", COMMAND_SIZE) == 0)
  {
    // Set this Node's name
    strncpy (name, CommandPacket.params, MAX_NAME_LENGTH-1);
    name[MAX_NAME_LENGTH-1] = 0;

    // Acknowledge new name
    strcpy (DataPacket.value, "NONAME=");
    strcat (DataPacket.value, name);

    pStatus = SUCCESS_DATA;
  }

  //--- Get Node Info (GNOI) ----------------------------
  else if (strncmp (CommandPacket.command, "GNOI", COMMAND_SIZE) == 0)
  {
    // Send Node info
    sprintf (DataPacket.value, "NOINFO=%s|%s|%s|%d", name, version, macAddressString, numDevices);

    pStatus = SUCCESS_DATA;
  }

  //--- Get Device Info (GDEI) ----------------------------
  else if (strncmp (CommandPacket.command, "GDEI", COMMAND_SIZE) == 0)
  {
    // For each Device, send a Device Data Packet with value = name|ipEnabled(Y/N)|ppEnabled(Y/N)|periodic data rate
    for (int i=0; i<numDevices; i++)
    {
      sprintf (DataPacket.deviceID, "%02d", i);
      DataPacket.timestamp = millis ();
      sprintf (DataPacket.value, "DEINFO=%s|%c|%c|%lu|", devices[i]->GetName(), devices[i]->IsIPEnabled() ? 'Y':'N', devices[i]->IsPPEnabled() ? 'Y':'N', devices[i]->GetRate());
      SendDataPacket ();
    }

    // All Device data has been sent, no need to send anything else
    pStatus = SUCCESS_NODATA;
  }

  //--- Ping (PING) ---------------------------------------
  else if (strncmp (CommandPacket.command, "PING", COMMAND_SIZE) == 0)
  {
    // Got PINGed from Interface, Respond with PONG
    strcpy (DataPacket.value, "PONG");

    pStatus = SUCCESS_DATA;
  }

  //--- Blink (BLIN) --------------------------------------
  else if (strncmp (CommandPacket.command, "BLIN", COMMAND_SIZE) == 0)
  {
    // Blink the Status LED white
    for (int i=0; i<10; i++)
    {
      rgbLedWrite (STATUS_LED_PIN, STATUS_LED_BRIGHTNESS, STATUS_LED_BRIGHTNESS, STATUS_LED_BRIGHTNESS);
      delay (20);

      rgbLedWrite (STATUS_LED_PIN, 0, 0, 0);
      delay (80);
    }

    // Return to green
    rgbLedWrite (STATUS_LED_PIN, 0, STATUS_LED_BRIGHTNESS, 0);

    pStatus = SUCCESS_NODATA;
  }

  //--- Reset (RSET) --------------------------------------
  else if (strncmp (CommandPacket.command, "RSET", COMMAND_SIZE) == 0)
  {
    // Acknowledge Reset
    Serial.println ("Resetting Node ... ");

    // Reset this Node
    esp_restart();
    // x x
    //  o
  }

  // Populate timestamp if data to return
  if (pStatus == SUCCESS_DATA || pStatus == FAIL_DATA)
    DataPacket.timestamp = millis();

  // Return the resulting ProcessStatus
  return pStatus;
}



//=========================================================
// External ESP-NOW "C" Functions
//=========================================================

//--- onCommandReceived -----------------------------------

void onCommandReceived (const esp_now_recv_info_t * esp_now_info, const uint8_t *commandString, int commandLength)
{
  if (Debugging)
  {
    // Show the incoming Command string
    Serial.print   ("Node <-- Relayer : ");
    Serial.println ((char *) commandString);
  }

  // Check if Relayer responded to initial Node PING
  if (strncmp ((char *) commandString, "PONG", 4) == 0)
    WaitingForRelayer = false;
  else
    // Add this ESP-NOW message to the command buffer
    CommandBuffer->PushString ((char *) commandString);
}


// //--- onDataPacketSent ------------------------------------
//
// void onDataPacketSent (const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//   // ...
// }

