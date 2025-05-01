/**
 * @file Nodex.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-04-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Nodex.h"

//=================================================
// Some 'Global' things that Node expects
// (TBD: shouldn't these be inside node?)
//=================================================
bool         Debugging=false;
bool         WaitingForRelayer = true;
RingBuffer   *CommandBuffer;
DPacket      DataPacket;
CPacket      CommandPacket;
char         DataString[MAX_MESSAGE_LENGTH];
esp_err_t    ESPNOW_Result;


//=================================================
// Initialize.  
// @param inName - name of this node
// @param inNodeId - identifier number assigned to this node
// @param relayMacAddr  - The mac address of the relayer. If null,
//                         then we assign addr of all zeros.
//                         Otherwise, This must be defined to 
//                         be MAC_SIZE bytes.
//                         Example: 
//                             uint8_t macAddr[MAC_SIZE]= {
//                                  0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 };
//=================================================
Nodex::Nodex(const char *inName, int inNodeID, const uint8_t *relayMacAddr):Node(inName, inNodeID)
{
    if (relayMacAddr == nullptr)
        memset( RelayerMAC, 0, MAC_SIZE);
    else
        setRelayMacAddr(relayMacAddr);

}

//=================================================
// @brief Set a new relay mac address.
// This sets a new relay address.
// @param macAddr - The mac address of the relayer. If null,
//                  then we assign addr of all zeros.
//                  Otherwise, This must be defined to 
//                  be MAC_SIZE bytes.
//                  Example: uint8_t macAddr[MAC_SIZE]= {
//                                   0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 };
//=================================================
 void Nodex::setRelayMacAddr(const uint8_t *macAddr)
 {
    memcpy(RelayerMAC, macAddr, MAC_SIZE);
 }


//=================================================
// @brief get the current mac address for the relay
// @param macAddr - Point to memory where we will 
//                  store the current  mac address. 
//                  Must be at least MAC_SIZE bytes.
//=================================================
 void Nodex::getRelayMacAddr(uint8_t *macAddr)
 {
    memcpy(macAddr, RelayerMAC, MAC_SIZE);
 }