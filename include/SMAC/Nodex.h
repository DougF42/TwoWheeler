/**
 * @file Nodex.h
 * @author Doug Fajardo
 * @brief Extensions/changes to the Node class
 * @version 0.1
 * @date 2025-04-30
 * 
 * @copyright Copyright (c) 2025
 * 
 * Changes from the original 'Node': 
 *    (1) The relayMacAddress is now part of the node class,
 *        NOT a global variable.
 */
#pragma once
#include "config.h"
#include "RingBuffer.h"
#include "Node.h"

class Nodex:public Node
{
    private:
    protected:
        uint8_t RelayerMAC[MAC_SIZE]; // MAC Address of the Relayer Module stored in non-volatile memory.
                                // This is set using the <SetMAC.html> tool in the SMAC_Interface folder.
                                // { 0x7C, 0xDF, 0xA1, 0xE0, 0x92, 0x98 }
    public:
        Nodex(const char *inName, int inNodeID, const uint8_t *macAddr);
        void setRelayMacAddr(const uint8_t *macAddr);
        void getRelayMacAddr(uint8_t *macAddr);
        bool ping(uint32_t timeoutMs);
};