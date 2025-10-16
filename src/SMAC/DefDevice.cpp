/**
 * @file DefDevice.cpp
 * @author Doug Fajardo
 * @brief Add parsing functions to the 'device' class.
 * @version 0.1
 * @date 2025-06-13
 * 
 * @copyright Copyright (c) 2025
 * 
 * This is a subclass to Device. It adds convenient parsing functions to aid in decoding
 *     the parameter string, and providing various error messages when the data does not
 *     match expectations.
 * Usage:   In devices, make your device a subclass of DefDevice (instead of Device). 
 * 
 * METHODS ADDED:
 * isCommand - compares its argument against the CommandPacket.command string.
 * 
 * scanParam - scans the CommandPacket.params arguments, parsing the list intonull-terminated
 *             tokens, using '|' as a delimiter.  ThIt returns 'argCount' (the 
 *             number of tokens found). In addition, the argcnt(number of parameters found)  
 *             and  'argList' ( an array of pointers to each token ) are available.
 * 
 * Various 'get...' ) functions to convert a string into one a number of different data types,
 *              with syntax checking specific to each data type.
 * 
 *              1st parameter is the index of the parameter (in arglist) to be scanned.
 *                     NOTE: scanParam MUST be called once before any of these 'get...' functions!!!
 *              2nd parameter is pointer to where the result should be stored. This must point
 *                  to existing memory of a suitable size for the requested data type.
 *              3d  parametr is a short 'header' string used to identify what is being
 *                  parsed in case of an error message. (see return value below)
 * 
 *              return value: 'SUCCESS_NODATA' or 'FAIL_DATA' as appropriate.
 *                 IF FAIL_DATA, a message will have been placed in the DataPacket.value indicating the
 *                 problem. The message will be in this format:
 *                     ERR|<3dParam>|<text describing the error>
 *                               
 */
#include "DefDevice.h"

/**
 * @brief Construct a new Def Device:: Def Device object
 * 
 * @param _node   - poitner to the node controlling this device.
 * @param inName  - Name of the device.
 */
DefDevice::DefDevice(const char *inName) : Device(inName)
{
    return;
}


DefDevice::~DefDevice()
{
    return;
}


/**
 * @brief Scan the parameter list
 *    The CommandPacket.params string is searched
 * for '|' separated parameters. Each parameter is 
 * null terminated, and  and a pointer to it is 
 * stored in the argument list. 
 * 
 *    IF we reach DEFDEVICE_MAX_ARGS, then we stop -
 *  the last argument will be the entire remaining
 *  line, regardless of content.
 * 
 * @return int number of parameters found. (This is
 *    the same as the argCount)
 */
int DefDevice::scanParam()
{
    argCount=0;
    char *aPtr = strtok(CommandPacket.params, "|");
    while ( (aPtr!=nullptr) && ( *aPtr != '\0') )
    {
        if (argCount >= DEFDEVICE_MAX_ARGS) break; // negative Ghost Rider, the pattern is full!
        arglist[argCount++] = aPtr;
        aPtr = strtok(nullptr, "|,");
    }
    return(argCount);
}


/**
 * @brief INTERNAL USE ONLY - verify that the argument number is valid.
 * 
 * @param argno        - the index of the argument (in 'arglist').
 * @return ProcessStatus  - Either FAIL_DATA (if error) or SUCCESS_NODATA
 */
ProcessStatus   DefDevice::argCountCheck(int argno, const char *msg)
{
    if ((argno >=  argCount) | (argno<0))
    {
        sprintf(DataPacket.value, "EROR,%s,Missing argument no %d", msg, argno);
        return(FAIL_DATA);
    }
    return(SUCCESS_NODATA);
}


/**
 * @brief Determine if we have a specific command
 * This does a caseless compare between the CommandPacket.command and a candidate command string.
 * 
 * @param cmd     - the command we are looking for. Only uip to 4 chars are used.
 * @return true   - the command matches.
 * @return false  - not a match.
 *   - TBD: Should this be a macro?
 */
bool  DefDevice::isCommand(const char *cmd)
{
    return(0==strncasecmp(CommandPacket.command, cmd, 4));
}


/**
 * @brief Get a 8bit unsigned integer
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus DefDevice::getUInt8(int arg, uint8_t *result, const char *msg)
{
   if (FAIL_DATA == argCountCheck(arg,msg)) return(FAIL_DATA);

    // Must be positive digits
    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "EROR,%s,argument %d is not an unsigned int", msg, arg);
            return (FAIL_DATA);
        }
    }

    long long tmpRes;
    tmpRes = strtoll(arglist[arg], nullptr, 10);
    if (tmpRes > (1L<<16) )
    {
        sprintf(DataPacket.value, "EROR,%s,Missing argument no %d");
        return(FAIL_DATA);
    }

    *result = (uint8_t)tmpRes;
    return (SUCCESS_NODATA); // OKAY
}


/**
 * @brief  get a long long int value
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 * @return ProcessStatus 
 */
ProcessStatus DefDevice::getLLint(int arg, long long *result, const char *msg)
{
    if (FAIL_DATA == argCountCheck(arg,msg)) return(FAIL_DATA);

    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "EROR,%s,argument %d is not an unsigned int", msg, arg);
            return (FAIL_DATA);
        }
    }

    *result = strtoll(arglist[arg], nullptr, 10);
    return (SUCCESS_NODATA);
}

/**
 * @brief get a plain integer value
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus   DefDevice::getUint16(int arg, uint16_t *result, const char *msg)
{
   if (FAIL_DATA == argCountCheck(arg, msg))
        return (FAIL_DATA);
    
    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "EROR,%s,Argument %d is not an unsigned int", msg, arg);
            return (FAIL_DATA);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return (SUCCESS_NODATA);
}



/**
 * @brief Get an unsigned int (32 bits)
 *    Must be all digits!
 * If an error is detected, a diagnostic
 * is queued in DataPacket.value
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus DefDevice::getUint32(int arg, uint32_t *result, const char *msg)
{
    if (FAIL_DATA == argCountCheck(arg, msg))
        return (FAIL_DATA);

    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr))
        {
            sprintf(DataPacket.value, "EROR,%s,argument %d is not an unsigned int", msg, arg);
            //Serial.printf("ERR|%s|Bad integer value for argument %d: '%s'", msg, arg, arglist[arg]);
            return (FAIL_DATA);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return (SUCCESS_NODATA);
}



/**
 * @brief get a plain integer value
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus  DefDevice::getInt16  (int arg,  int16_t   *result,  const char *msg)
{
   if (FAIL_DATA == argCountCheck(arg, msg))
        return (FAIL_DATA);
    
    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr) && (*ptr != '+') && (*ptr != '-'))
        {
             sprintf(DataPacket.value, "EROR,%s,argument %d is not an unsigned int", msg, arg);
            return (FAIL_DATA);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return (SUCCESS_NODATA);
}





/**
 * @brief Get a signed integer (32 bit)
 *
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus DefDevice::getInt32(int arg, int32_t *result, const char *msg)
{
    if (FAIL_DATA == argCountCheck(arg, msg))
        return (FAIL_DATA);
    
    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr) && (*ptr != '+') && (*ptr != '-'))
        {
            sprintf(DataPacket.value, "EROR,%s,Argument %d is not a int", msg, arg);
            return (FAIL_DATA);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return (SUCCESS_NODATA);
}

/**
 * @brief get a plain integer value
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus  DefDevice::getInt  (int arg,  int   *result,  const char *msg)
{
   if (FAIL_DATA == argCountCheck(arg, msg))
        return (FAIL_DATA);
    
    for (char *ptr = arglist[arg]; *ptr != '\0'; ptr++)
    {
        if (!isDigit(*ptr) && (*ptr != '+') && (*ptr != '-'))
        {
           sprintf(DataPacket.value, "EROR,%s,Argument %d is not a int", msg, arg);
            return (FAIL_DATA);
        }
    }

    *result = strtol(arglist[arg], nullptr, 10);
    return (SUCCESS_NODATA);
}


/**
 * @brief Get a 'double' value
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus DefDevice::getDouble(int arg, double *result, const char *msg)
{
   if (FAIL_DATA == argCountCheck(arg,msg)) return(FAIL_DATA);

    errno = 0;
    double tmpVal =strtod(arglist[arg], nullptr);
    if (errno!=0)
        {
            sprintf(DataPacket.value, "EROR,%s,Invalid double for argument no %d", msg, arg);
            return(FAIL_DATA);
        }

    *result=tmpVal;
    return(SUCCESS_NODATA);
}


/**
 * @brief Get the Bool value
 *     values recognized - 0, 1, Y(es) N(o), T(rue), F(alse)
 * 
 * @param arg        index of the argument to scan
 * @param result     where to store the result (if no error)
 * @param msg        Header for error messages
 * @return    Either SUCCCESS_NODATA or FAIL_DATA
 */
ProcessStatus DefDevice::getBool(int arg, bool *result, const char *msg)
{
    ProcessStatus retval = SUCCESS_NODATA;
    retval = argCountCheck(arg, msg);
    if (retval == FAIL_DATA)
    {
       // Serial.println("Arg count check failed!");
        retval = FAIL_DATA;

    } else {
        char firstChar = toupper(arglist[arg][0]); // Just use 1st char
        if ((firstChar == '0') || (firstChar == 'N') || (firstChar == 'F'))
        {
            *result = false;
        }
        else if ((firstChar == '1') || (firstChar == 'Y') || (firstChar == 'T'))
        {
            *result = true;
        }
        else
        {
            sprintf(DataPacket.value, "EROR,%s,Unknown boolean value for argument %d",
                    arg, msg);
            retval = FAIL_DATA;
        }
    }
    return (retval);
}
