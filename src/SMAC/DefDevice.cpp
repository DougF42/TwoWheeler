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
    argCount=0;
    char *aPtr = strtok(CommandPacket.params, "|");
    while ( (aPtr!=nullptr) && ( *aPtr != '\0') )
    {
        arglist[argCount++] = aPtr;
        aPtr = strtok(nullptr, "|");
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
 * @param arg    - argument number (index of arglist)
 * @param result - where to store the result 
 * @param msg    - header for error messages
 * @return int   - 0 if okay. non-zero if bad value - a 'fail' message was generated
 */
int DefDevice::getUint32(int arg, uint32_t *result, const char *msg)
{
    if (arg>argCount)
    {
        // Missing argumen t
        sprintf(DataPacket.value, "%s:Missing argument no %d",msg, arg );
        Serial.printf("For %s: Missing argument %d - argcount is %d\r\n", msg, arg, argCount);
        return(1);
    }

    for (char *ptr = arglist[arg]; *ptr!='\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "%s:argument %d is not an unsigned int\n", msg, arg);
            Serial.printf("For %s: Bad integer value for argument %d: '%s'\r\n", msg, arg, arglist[arg]);
            return (2);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return(0);
}


/**
 * @brief Get a signed integer (32 bit)
 * 
 * @param arg    - argument number (index of arglist)
 * @param result - where to store the result 
 * @param msg    - header for error messages
 * @return int   - 0 if okay. non-zero if bad value - a 'fail' message was generated
 */
int DefDevice::getInt32(int arg, int32_t *result, const char *msg)
{
    if ((arg > argCount) || (arg<0))
    {
        // Missing argumen t
        sprintf(DataPacket.value, "%s:Invalid or Missing argument no %d", msg, arg);
        return (1);
    }
    Serial.printf("getInt32: argument is %s\r\n", arglist[arg]);
    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr) && (*ptr != '+') && (*ptr != '-'))
        {
            sprintf(DataPacket.value, "%s:Argument %d is not a int\n", msg, arg);
            return (2);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    Serial.printf("getInt32: Result is %d\n", *result);
    return(0);
}


/**
 * @brief Get a 'double' value
 * 
 * @param arg    - argument number (index of arglist)
 * @param result - where to store the result 
 * @param msg    - header for error messages
 * @return int   - 0 if okay. non-zero if bad value - a 'fail' message was generated
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
 * @brief Get the Bool value
 *     values recognized - 0, 1, Y(es) N(o), T(rue), F(alse)
 * 
 * @param arg    - argument number (index of arglist)
 * @param result - where to store the result 
 * @param msg    - header for error messages
 * @return int   - 0 if okay. non-zero if bad value - a 'fail' message was generated
 */
int DefDevice::getBool(int arg, bool *result, const char *msg)
{
    int retval=0;
    if (arg>=argCount)
    {
        sprintf( DataPacket.value, "%s: %s %d", msg, "Missing argument %d");
        defDevSendData(0, false);
        retval=1;
    }
    char firstChar = toupper(arglist[arg][0]); // Just use 1st char
    if ( (firstChar == '0') || (firstChar=='N') || (firstChar=='F') )
    {
        *result=false;

    }  else if ( (firstChar == '1') || (firstChar=='Y') || (firstChar=='T') )
    {
        *result=true;

    }  else 
        sprintf(DataPacket.value, "%s:%s %s", msg, "Unknown boolean value for argument ", arg);
        defDevSendData(0, false);
        retval= 2;
    return(retval);
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