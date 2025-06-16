/**
 * @file DefDevice.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-06-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "DefDevice.h"

DefDevice::DefDevice( Node *_node, const char *inName) : Device(inName)
{
    myNode = _node;
}


DefDevice::~DefDevice()
{
}


/**
 * @brief Scan the parameter list
 *    The CommandPacket.params string is searched
 * for '|' separated parameters. Each parameter is 
 * null terminated, and  and a pointer to it is 
 * stored in the argument list. 
 * 
 * @return int number of parameters found. 
 */
int DefDevice::scanParam()
{
    char *aPtr = strtok(CommandPacket.params, "|");
    while (aPtr!=nullptr)
    {
        arglist[argCount++] = aPtr;
    }
    return(argCount);
}


/**
 * @brief Send a data packet Using the node's 'SendDataPacket'
 *    This causes a SMAC message to be sent using
 * the global 'DataString' and the deviceID to send the packet.
 * 
 *   Timestamp is set - either to 'millis()' (if 0)
 *       or to the given value.
 * 
 * @param timeStamp if not zero, set the timestamp of the message.
 *                  if zero, use the value returned by millis().
 * 
 * @param sendNowFlag - if true, the packet is sent immediatly.
 *                    set false if the packet is being sent from 
 *                    the termination of a Execute, doImmediate or
 *                    doPeriodica route.
 *                      default false.
 */
void DefDevice::defDevSendData(time_t timeStamp, bool sendNowFlag)
{
    memcpy( DataPacket.deviceID, deviceID, ID_SIZE+1);
    DataPacket.timestamp = (timeStamp==0) ? millis() : timeStamp;
    if (sendNowFlag)     myNode->SendDataPacket();
}

int DefDevice::getLLint(int arg, long long *result, const char *msg)
{
    if (arg > argCount)
    {
        // Missing argumen t
        sprintf(DataPacket.value, "%s:Missing argument no %d", msg, arg);
        return (1);
    }

    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "%s:argument %d is not an unsigned int\n", msg, arg);
            return (2);
        }
    }

    *result = strtoll(arglist[arg], nullptr, 10);
    return (0);
}

/**
 * @brief Get an unsigned int (32 bits)
 *    Must be all digits!
 * If an error is detected, a diagnostic
 * is queued in DataPacket.value
 * 
 * @param arg   - the argument number
 * @param msg   - msg header for error messages
 * @param result - pointer to the resulting value
 * @return error code (0 normal)
 */
int DefDevice::getUint32(int arg, uint32_t *result, const char *msg)
{
    if (arg>argCount)
    {
        // Missing argumen t
        sprintf(DataPacket.value, "%s:Missing argument no %d",msg, arg );
        return(1);
    }

    for (char *ptr = arglist[arg]; *ptr!='\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "%s:argument %d is not an unsigned int\n", msg, arg);
            return (2);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return(0);
}


/**
 * @brief Get a signed integer (32 bit)
 * 
 * @param arg 
 * @param result 
 * @param msg 
 * @return int 
 */
int DefDevice::getInt32(int arg, int32_t *result, const char *msg)
{
    if (arg > argCount)
    {
        // Missing argumen t
        sprintf(DataPacket.value, "%s:Missing argument no %d", msg, arg);
        return (1);
    }

    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr) && (*ptr != '+') && (*ptr != '-'))
        {
            sprintf(DataPacket.value, "%s:Argument %d is not a int\n", msg, arg);
            return (2);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return(0);
}


/**
 * @brief Get a 'double' value
 * 
 * @param arg 
 * @param result 
 * @param msg 
 * @return int 
 */
int DefDevice::getDouble(int arg, double *result, const char *msg)
{
    if (arg > argCount)
    {
        // Missing argumen t
        sprintf(DataPacket.value, "%s:Missing argument no %d", msg, arg);
        return (1);
    }

    errno = 0;
    double tmpVal =strtod(arglist[arg], nullptr);
    if (errno!=0)
        {
            sprintf(DataPacket.value, "%s: Invalid double for argument no %d", msg, arg);
            return(2);
        }

    *result=tmpVal;
    return(0);
}

/**
 * @brief Determine if we have a specific command
 * 
 * @param cmd   - the command we are looking for
 * @return true  - the command matches
 * @return false  - not a match
 */
bool  DefDevice::isCommand(const char *cmd)
{
    return(0==strncasecmp(CommandPacket.command, cmd, 4));
}