/**
 * @file DefDevice.h
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-06-13
 * 
 * @copyright Copyright (c) 2025
 * 
 * This superclass adds the following to the SMAC 'Device'
 * class -
 *  (1) Capture a pointer to 'Node', which allows us to 
 * call sendData
 *  (2) A 'sendData' routine that sets the timestamp and 
 * (optionally) calls the node's senddata.
 *  (3) Functions for parsing and decoding the parameter 
 * string.
 */
#pragma once
#include "common.h"
#include "Node.h"
#include "Device.h"

class DefDevice : public Device
{
    protected:
        char *arglist[6];
        int argCount;
        Node *myNode;
        void  defDevSendData(time_t timeStamp, bool sendNowFlag=false);
        int   scanParam();  // scan the parameter list
        int   getLLint(int arg, long long  *result, const char *msg);
        int   getUint32(int arg, uint32_t *result, const char *msg);
        int   getInt32(int arg,  int32_t *result, const char *msg);
        int   getDouble(int arg, double *result, const char *msg);
        bool  isCommand(const char *cmd);

    public:
        DefDevice( Node *_node, const char * InName);
        ~DefDevice();
        
};