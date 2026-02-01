/**
 *
 */
#include "Util.h"
#include <climits>
/**
 * @brief: This checks that the argument exists, AND strips
 * trailing characters from it.
 *
 *  The ProcessStatus is WIDGET_DATA normally, SYSTEM_DATA
 * if its empty
 *
 * tmpPtr is a pointer to a duplicate of the argument, terminated
 * by null.  It is truncated at the first comma or line terminator.
 *  The caller MUST free this!
 *
 *
 */
ProcessStatus Util::sanityCheck(char *arg, char *refName, const char **tmpPtr)
{

    ProcessStatus retStatus = NOT_HANDLED;
    if ((arg == nullptr) || (arg == '\0'))
    {
        // Sanity check - is the arg empty?
        sprintf(SMACData.values, "EROR decoding %s: No argument ", refName);
        retStatus = SYSTEM_DATA;
    }
    else
    {
        *tmpPtr = strdup(arg);
        strtok(*tmpPtr, ",\r\n");
        retStatus = WIDGET_DATA;
    }
    return (retStatus);
}

/**
 * @brief - decode a boolean value
 *
 *  Boolean values may be any of the following:
 *    0
 *    false (any case)
 *    no    (any case)
 *    1
 *    true  (any case)
 *    yes   (any case)
 *
 */
ProcessStatus Util::getbool(char *arg, bool *res, const char *refName)
{

    ProcessStatus retStatus = NOT_HANDLED;
    char *tmp = nullptr;

    retStatus = sanityCheck(arg, refName, &tmp);

    if (retStatus == SYSTEM_DATA)
    {
        if (0 == strcmp("0", tmp) || (0 == strcmp("false", tmp)) || (0 == strcmp("no", tmp)))
        {
            *res = false;
            retStatus = SYSTEM_DATA;
        }
        else if ((0 == strcmp("1", tmp)) || (0 == strcmp("true", tmp)) || (0 == strcmp("yes", tmp)))
        {
            *res = true;
            retStatus = WIDGET_DATA;
        }
        else
        {
            // OOPS!
            sprintf(SMACData.values, "EROR decoding %s: Not a valid boolean value", refName);
        }
    }
    free(tmp);
    return (retStatus);
}


/**
 * Decode unsigned int
 */
ProcessStatus Util::getuint8(char *arg, uint8_t *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long val = strtol(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<uint8_t>::min())) | (val > std::numeric_limits<uint8_t>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}


ProcessStatus Util::getint8(char *arg, int8_t *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long val = strtol(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<int8_t>::min())) | (val > std::numeric_limits<int8_t>::max()))
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return (retStatus);
}


ProcessStatus Util::getuint16_t(char *arg, uint16_t *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long val = strtol(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<uint16_t>::min())) | (val > std::numeric_limits<uint16_t>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint16_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}

ProcessStatus Util::getint16_t(char *arg, int16_t *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long val = strtol(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
    else if (((val < std::numeric_limits<int16_t>::min())) | (val > std::numeric_limits<int16_t>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a int16_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}


ProcessStatus Util::getint_t(char *arg, int *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long val = strtol(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<int>::min())) | (val > std::numeric_limits<int>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}


ProcessStatus Util::getUint_t(char *arg, unsigned int *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long val = strtol(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<unsigned int>::min())) | (val > std::numeric_limits<unsigned int>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}


ProcessStatus Util::getUint32_t(char *arg, uint32_t *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long long val = strtoll(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<uint32_t>::min())) | (val > std::numeric_limits<uint32_t>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}

ProcessStatus Util::getint32_t(char *arg, int32_t *res, const char *refName)
{
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long long val = strtoll(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<int32_t>::min())) | (val > std::numeric_limits<int32_t>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}


    ProcessStatus Util::getLL_t(char *arg, long long *res, const char *refName)
    {
        {
    ProcessStatus retStatus = NOT_HANDLED;
    char *astring = nullptr;

    retStatus = sanityCheck(arg, refName, &astring);

    if (retStatus == WIDGET_DATA)
    {
        char **endptr;
        errno = 0;
        long long val = strtoll(astring, endptr, 0); // allow 0x (hex), O888 (octal) or nnn (decimal)
        if (errno != 0)
            if (*endptr != '\0')
            {
                sprintf(SMACData.values, "EROR decoding %s - bad value", refName);
                retStatus = SYSTEM_DATA;
            }
            else if (((val < std::numeric_limits<int64_t>::min())) | (val > std::numeric_limits<int64_t>::max()) )
            {
                sprintf(SMACData.values, "EROR decoding %s - out of range for a uint8_t ", refName);
                retStatus = SYSTEM_DATA;
            }
            else
            { // valid !!!
                *res = val;
                retStatus = WIDGET_DATA;
            }
    }
    free(astring);
    return(retStatus);
}
    }