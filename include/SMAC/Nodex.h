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
        uint8_t RelayerMAC[MAC_SIZE];
    public:
        Nodex(const char *inName, int inNodeID, const uint8_t *macAddr);
        void setRelayMacAddr(const uint8_t *macAddr);
        void getRelayMacAddr(uint8_t *macAddr);
};