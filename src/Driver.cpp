/**
 * @file Driver.cpp
 * @author Doug Fajardo
 * @brief 
 * @version 0.1
 * @date 2025-05-01
 * 
 * @copyright Copyright (c) 2025
 * 
 * This sets up - and controls - two wheels. Feedback
 * thru the PID class is used to govern the actual 
 * power applied to each wheel.
 */
#include "config.h"
#include "Driver.h"
#include "QuadDecoder.h"
#include "stdlib.h"

// What we consider delimiters for commands 
#define COMMAND_WHITE_SPACE " |\r\n"

// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// We have a new driver
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::Driver(int devId) : Device{"Driver"}
{
    nextMotorIdx=0;   
    mySpeed=0;
    myDirect=0; 
    SetID(devId);
    Serial.print(" ");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
Driver::~Driver()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Add a motor to our list. 
// Return:  true normally, false if too many motors
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool Driver::addNewMotor(const MotorControl_config_t &configuration)
{
    if (nextMotorIdx >= MAX_MOTOR_COUNT) return(false);
    Serial.print("Driver: adding new motor at index "); Serial.println(nextMotorIdx);
    motors[nextMotorIdx] = new MotorControl();
    motors[nextMotorIdx]->setup(configuration);
    nextMotorIdx++;
    return(true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Main loop - This calls both low-level motor 
//    controllers (which uses the Position sensor and
//    a PID loop to govern the motor speed). It is 
//    separate from the 'doPeriodic' routine which 
//    processes commands.
//  
// - - - - - - - - - - - - - - - - - - - - - - - - - - 
void Driver::loop()
{
    for (int idx=0; idx<nextMotorIdx; idx++)
    {
        motors[idx]->loop();
    }
}


  // If your child Device class needs to handle custom commands, then override this method:
  //
  // The global <CommandPacket> will have the command definition.
  // First call this base class method to handle the built-in Device commands:
  //
  //   Device::ExecuteCommand ();
  //
  // If this call returns NOT_HANDLED, then your child class should handle the command.
  //
  // If your ExecuteCommand() method has data to return, it should populate the
  // global <DataPacket> and return an appropriate ProcessStatus.
  //
  // When populating the global <DataPacket>, value strings that start with a dash or a digit
  // will be interpreted by the Interface as periodic process data, say from a sensor reading.
  //
  // Commands recognized by the driver:
  //   QUAD  <pulsesPerRev>, <circum>, <units>   // configure the Quadrature encodere.
  //   PID   <Kp>,<Kd>,<Ki>                      // configure the PID controler.
  //   SPD   <rate>      // +/- 2048  heading change in mm per Millisecond. May be negative.
  //   ROT <degrees>      // +/- 2048 degrees per Millisecond. Negative is right, positive is left
  //   stop (int stopRate); // 0..100 0 means drift, 100 means emergency stop, otherwise percentage
  //

  //       
ProcessStatus  Driver::ExecuteCommand ()
{
    ProcessStatus status;
    status = Device::ExecuteCommand();
    if (status != NOT_HANDLED) return(status);

    status=FAIL_NODATA;
    char *cmdPtr;
    char *paramPtr;

    // we try to handle it...
    Serial.printf("Parsing Command device index '%d', command='%s' with paramters '%s'\n", CommandPacket.deviceIndex, CommandPacket.command, CommandPacket.params);
    // Relayed messages have a timestamp (which we ignore), but while direct commands do not (and commands do not start with a digit)...
    if ( isDigit(CommandPacket.command[0]) )
    {
        // Skip the time stamp...
        for ( cmdPtr = CommandPacket.params; (( *cmdPtr!='\0')  &&  !isalpha(*cmdPtr));  cmdPtr++) 
        { // do nothing
         };
        paramPtr= cmdPtr+5;
    } else {
        // No time stamp - use as is
        cmdPtr=CommandPacket.command;
        paramPtr=CommandPacket.params;
    }
    Serial.printf("---- cmd is  '%s' \n", cmdPtr);
    Serial.printf("      Param = %s\n", paramPtr);

    if (strncmp(cmdPtr, "QUAD",4 ) == 0)
    {   // Set/get Quad paramters        
        status=cmdQUAD(paramPtr);

    }  else if (strncmp(cmdPtr, "DPID",4) == 0)
    {  // Set PID parameters
        status=cmdPID(paramPtr);

    }  else if (strncmp(cmdPtr, "DMOV",4) == 0)
    {  // Move  at a given speed and rate of rotation
        status=cmdMOV(paramPtr);
        
    }  else if (strncmp(cmdPtr, "STOP",4) == 0)
    { // Stop all motion
        status=cmdSTOP(paramPtr);

    } else if (strncmp(cmdPtr,"SPED",4) == 0)
    {  // Set speed
        status = cmdSPEED(paramPtr);

    } else if (strncmp(cmdPtr,"ROTA",4 ) == 0)
    {  // Set rotation rate
        status = cmdROTATION(paramPtr);
    }
    Serial.print("STATUS:  "); Serial.println(status);
    return(status);
}


// - - - - - - - - - - - - - - - - - -
// set or get the quad parameters.
// quad            - with no arguments, acts as 'get'
// quad|<pulsesPerRev>|<circum>
//    pulsesPerRev -  self explanatory
//    circum       - circumfrence in mm.
// We return the new (current) pulses per rev and circum(in mm).
// NOTE: Both motors will be set to the same values.
// @pParams - pointer to the parameters
ProcessStatus Driver::cmdQUAD(char *pParams)
{
    pulse_t pulsesPerRev;
    dist_t circumfrence; // in mm
    char *ptr;
    char *endptr;

    ProcessStatus result=SUCCESS_DATA;
    Serial.println("See cmdQUAD");
    // Do we have any arguments?
    ptr = strtok(pParams, COMMAND_WHITE_SPACE);
    if (ptr != nullptr)
    { // we have arguments - this is a 'Set' command
        errno = 0;
        pulsesPerRev = strtoul(ptr, nullptr, 10);
        if ((errno != 0) || (pulsesPerRev > 5000))
        {
            // TODO: ERROR - invalid ppr
            sprintf(DataPacket.value, "Invalid pulses-per-revolution value");
            result = FAIL_DATA;
        } 

        // Decode circumfrence
        ptr = strtok(nullptr, COMMAND_WHITE_SPACE);
        Serial.print("CIRCUM ARG is "); Serial.println(ptr);

        if (ptr == nullptr)
        {
            sprintf(DataPacket.value,"Missing or invalid circumfrence value");
            result = FAIL_DATA;
        }
        circumfrence = strtod(ptr, nullptr);

        if (circumfrence > 200)
        {
            sprintf(DataPacket.value,"Invalid circumfrence");
            result = FAIL_DATA;
        }

        Serial.print("Circumfrence value is ");  Serial.println(circumfrence);

        if (result == SUCCESS_DATA)
        {
            // SET the new values
            motors[0]->setQUADcalibration(pulsesPerRev, circumfrence);
            motors[1]->setQUADcalibration(pulsesPerRev, circumfrence);
        }
    }

    // on success, return the current values (unless there was an error)
    if (result==SUCCESS_DATA)
    {
        motors[0]->getCalibration(&pulsesPerRev, &circumfrence);
        sprintf(DataPacket.value,"pulse_per_rev=%u Circum=%f ",pulsesPerRev, circumfrence);
        DataPacket.timestamp=millis();
    }
    
    Serial.print(" QUAD command returns "); Serial.println(result);
    return(result);
}


/**
 * @brief Set the paramters for the PID loop
 * Format: "PID|step_time|kp|ki|kd"
 *     Time is in steps per second
 *     kp, ki, kd are the PID parameters. 
 * 
 * @return ProcessStatus 
 * 
 * The command is processed in-place with strtok
 * to parse tokens.
 */
ProcessStatus Driver::cmdPID(char *pParams)
{
    ProcessStatus result = SUCCESS_DATA;
    Serial.println("See cmdPID");
    unsigned long tmpTime;
    float tmpkp, tmpki, tmpkd;
    char *ptr;
    errno = 0;

    ptr = strtok(pParams, COMMAND_WHITE_SPACE); // 1st pointer
    if (ptr != nullptr)
    { // yes, we have arguments... process the settng}
        
        errno = 0;
        // Kp
        tmpTime = strtol( ptr, nullptr, 10);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Kp parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        }
        tmpkp = strtof(ptr, nullptr);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Kp parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        };

        // Ki
        ptr = strtok(nullptr, COMMAND_WHITE_SPACE); // 1st pointer
        errno=0;
        tmpki = strtof(ptr, nullptr);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Kd parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        };

        // Kd
        ptr = strtok(nullptr, COMMAND_WHITE_SPACE); // 1st pointer
        errno=0;
        tmpkd = strtof(ptr, nullptr);
        if ((ptr == nullptr) || (errno != 0))
        {
            result = FAIL_DATA;
            sprintf(DataPacket.value, "Ki parameter is not a valid floating point value");
            goto endOfSetPidParameters;
        };
    }
    // SET THE PID PARAMETERS    
    motors[0]->setPIDTuning( tmpkp, tmpki, tmpkd); 
    motors[1]->setPIDTuning( tmpkp, tmpki, tmpkd);
    sprintf(DataPacket.value, "Kp=%f  ki=%f  kd=%f", tmpkp, tmpki, tmpkd);
    // SUPLLY CURRENT VALUE AS RESPONSE

 endOfSetPidParameters:
    return(result);
}


/**
 * @brief Internal - Set the speed for the two motors.
 *   speed is +/- 2048,  rotation is +/- 2048.
 * This handles all normalization and limits.
 * (Note: If the speed and rotation haven't changed, then 
 *  the motor speeds are not changed. )
 * 
 * It also puts the current speed/rotation response in the Datapacket.
 * @param speed     - the desired speed (0 +/-2048).
 * @param rotation  - the desired rotation (0 +/- 2048)
 */
void Driver::setMotion(int speed, int rotation)
{
    Serial.printf("** In setMotion. Speed=%d  rotation=%d\n", speed, rotation);
    int tmpSpeed, tmpDirection = 0;
    dist_t m1, m2 = 0.0;

    tmpSpeed = constrain(speed, -2048, 2048);
    tmpDirection = constrain(rotation, -2048, 2048);

    // calculate motor 1 speed. limit to +/-2048
    m1 = mySpeed + myDirect;
    m1 = constrain(m1, -2048, 2048);

    // Limit Direction to +/- 2048
    m2 = mySpeed - myDirect;
    m2 = constrain(m2, -2048, 2048);
    if ((tmpSpeed != mySpeed) || (tmpDirection != myDirect))
    {
        motors[0]->setSpeed(m1);
        motors[1]->setSpeed(m2);
    }
    // Report current speed and rotation rate
    DataPacket.timestamp = millis();
    sprintf(DataPacket.value, "Speed|%d| dir|%d| m1|%d| M2|%d", mySpeed, myDirect, m1, m2);
}

/**
 * @brief Set the forward motion to a given speed
 *  Format:  "FWD|speed|turnRate"
 *      The speed is 0 +/-2048,  dir is 0 +/-2048
 *  dir = +/-2048, then rotate in place.
 *  If no arguments, then just report the current motion. 
 *  If no turnRate, assume straight ahead
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdMOV(char *pParams)
{
    int tmpval = 0;
    char *pSpd = nullptr;
    char *pRot = nullptr;


    Serial.println("See cmdMOV");
    ProcessStatus result = SUCCESS_DATA;
    pSpd = strtok(pParams, COMMAND_WHITE_SPACE); // 1st argument - speed
    if (pSpd != nullptr)
    {   // We have a speed...
        errno = 0;
        tmpval = strtol(pSpd, nullptr, 10);
        if (errno != 0)
        { // bad value (overflow/underflow)
            result = FAIL_DATA;
            sprintf(DataPacket.value, "speed parameter is not a valid value");
            goto endCmdMOV;
        }
        else
        {
            mySpeed = tmpval;
        }

        char *pRot = strtok(nullptr,COMMAND_WHITE_SPACE); // 2nd argument is optional (direction)
        if (pRot != nullptr)
        { // if we have one, it must be valid...
            errno = 0;
            tmpval = strtol(pRot, nullptr, 10);
            if (errno != 0)
            {
                result = FAIL_DATA;                
                sprintf(DataPacket.value, "speed parameter is not a valid value");
                goto endCmdMOV;
            }
            myDirect = tmpval;

        }  else
        { // No direction, must be straight ahead
            myDirect = 0;
        }
    }
    // Using actual values, set the motors and report settings
    setMotion(mySpeed, myDirect);

endCmdMOV:
    return (result);
}


/**
 * @brief Stop driving the motors
 *  Format: STOP|stoprate
 *     Stoprate is 0 (drift, motors not engaged) to 100 (panic stop)
 *
 * @return ProcessStatus
 */
ProcessStatus Driver::cmdSTOP(char *pParams)
{
    // TODO:
    Serial.println("See cmdSTOP - not implemented");
    return (NOT_HANDLED);
}


/**
 * @brief SMAC command handler - set speed
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdSPEED(char *pParams)
{
    ProcessStatus result;
    Serial.printf("*** IN cmdSPEED. Parameters string is '%s'\n", pParams);
    char *pSpd;
    int tmpval;
    errno = 0;
    tmpval = strtol(pParams, nullptr,10);
    if (errno != 0)
    { // bad value (overflow/underflow)
        Serial.printf("ERROR: INVALID SPEED VALUE: %d\n", tmpval);
        sprintf(DataPacket.value, "speed parameter is not a valid value");
        result = FAIL_DATA;
        goto cmdSPEEDend;
    } else {
        setMotion(tmpval, myDirect);
        result = SUCCESS_DATA;
    }

cmdSPEEDend:
    return (result);
}

/**
 * @brief SMAC command handler - set rotation rate
 * 
 * @return ProcessStatus 
 */
ProcessStatus Driver::cmdROTATION(char *pParams)
{
    ProcessStatus result = SUCCESS_DATA;

    char *pRot;
    int tmpval;
    pRot = strtok(pParams, COMMAND_WHITE_SPACE); // 1st argument - speed
    errno = 0;
    if (pRot != nullptr)
    {
        tmpval = strtol(pRot, nullptr, 10);
        if (errno != 0)
        { // bad value (overflow/underflow)
            result = FAIL_DATA;
            sprintf(DataPacket.value, "rotation rate parameter is not a valid value");
            goto cmdSPEEDend;
        }
        else
        {
            setMotion(mySpeed, tmpval);
            result=SUCCESS_DATA;
        }
    }

cmdSPEEDend:
    return(result);
}