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
 *  (1) scanParam - parses a string into a list of '|' separated 
 *      tokens (null-temrinated strings).  The result is available 
 *      in the 'arglist[n]', where n is the index of each token (0...5).
 *  
 *  (2) Functions for parsing and decoding a token into various numeric
 *      types.
 */
#pragma once
#include "common.h"
#include "Device.h"

// This is the max number of arguments allowed. It defines the size of 
//   the arglist array, so should be kept to a rasonable size.
#define  DEFDEVICE_MAX_ARGS  5
class DefDevice : public Device
{
    private:
        ProcessStatus   argCountCheck(int argno, const char* msg);
        
    protected:
        char *arglist[DEFDEVICE_MAX_ARGS];
        int argCount;
        int   scanParam();  // scan the parameter list
        bool  isCommand(const char *cmd);

        ProcessStatus   getUInt8(int arg, uint8_t *result, const char *msg);
        ProcessStatus   getLLint(int arg, long long  *result, const char *msg);
        ProcessStatus   getUint32(int arg, uint32_t *result, const char *msg);
        ProcessStatus   getInt32(int arg,  int32_t *result, const char *msg);
        ProcessStatus   getDouble(int arg, double *result, const char *msg);
        ProcessStatus   getBool(int arg, bool *result, const char *msg);

    public:
        DefDevice(const char * InName);
        ~DefDevice();
        
};