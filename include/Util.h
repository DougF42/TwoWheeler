/*
 * Various conversion utilties for the SMAC environment.
 * The requested conversion is checked for validity, and if
 * an error is found, the error placed in SMACDataPacket.values, 
 * and FAIL_DATA status is reported.
 * 
 * In all cases:
 *    arg  is a pointer to the string to decode.
 *    res  is a pointer to the result (supplied by the caller)
 *    refName is a name or short phrase used in error messages 
 *            to identify the thing being decoded.
 *    Return value: WIDGET_DATA if found, 
 *                   SYSTEM_DATA if an error - In this case a 
 *                   descriptive error message will be in SMACData.values 
 */
#include "SMAC/common.h"
#pragma once
class Util
{
private:
    static ProcessStatus sanityCheck(char *arg, const char *refName, char **tmpPtr);

public:
    static ProcessStatus getbool    (char *arg, bool *res,         const char *refName);
    static ProcessStatus getuint8   (char *arg, uint8_t *res,      const char *refName);
    static ProcessStatus getint8    (char *arg, int8_t *res,       const char *refName);
    static ProcessStatus getchar    (char *arg, char *res,         const char *refName);

    static ProcessStatus getuint16_t(char *arg, uint16_t *res,     const char *refName);
    static ProcessStatus getint16_t (char *arg, int16_t *res,      const char *refName);

    static ProcessStatus getint_t   (char *arg, int *res,          const char *refName);
    static ProcessStatus getUint_t  (char *arg, unsigned int *res, const char *refName);
    static ProcessStatus getUint32_t(char *arg, uint32_t *res,     const char *refName);
    static ProcessStatus getint32_t (char *arg, int32_t *res,      const char *refName);

    static ProcessStatus getLL_t    (char *arg, long long *res,    const char *refName);
    static ProcessStatus getDouble_t(char *arg, double *res,       const char *refName);
};